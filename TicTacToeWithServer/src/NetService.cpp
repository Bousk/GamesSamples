#include <NetService.hpp>

#include <Sockets.hpp>
#include <UDP/Protocols/ReliableOrdered.hpp>

#define FORWARD_TO_LISTENERS(ListenerMethod, ...)	\
	for (IListener* listener : mListeners)			\
	{												\
		listener->ListenerMethod(__VA_ARGS__);		\
	}

NetService::NetService()
{
	mUdpClient.registerChannel<Bousk::Network::UDP::Protocols::ReliableOrdered>();
}
bool NetService::init(const Parameters& parameters)
{
	if (isInitialized())
		return false;
	if (parameters.isNetworked())
	{
		if (!Bousk::Network::Start())
		{
			return false;
		}
		if (!mUdpClient.init(parameters.localPort))
			return false;
		if (!parameters.isHost())
		{
			// If we're not host, initialize connection right away
			mUdpClient.connect(parameters.hostAddress);
		}
	}
	mLocalAddress = Bousk::Network::Address::Loopback(Bousk::Network::Address::Type::IPv4, parameters.localPort);
	mContext = parameters;
	mState = State::Initialized;
	FORWARD_TO_LISTENERS(onServiceInitialized);
	return true;
}
void NetService::release()
{
	if (isInitialized() && isNetworked())
	{
		mUdpClient.release();
		Bousk::Network::Release();
	}
	mLoopbackData.clear();
	mState = State::Idle;
	FORWARD_TO_LISTENERS(onServiceReleased);
}

void NetService::receive()
{
	assert(isInitialized());
	if (isNetworked())
		mUdpClient.receive();
}
void NetService::process()
{
	assert(isInitialized());
	
	// Process loopback data
	// Copy them in case more are queued during callbacks
	std::vector<std::vector<Bousk::uint8>> loopbackData = std::move(mLoopbackData);
	for (auto&& data : loopbackData)
	{
		Bousk::Network::Messages::UserData msg(localAddress(), 0/*Unused with UDP*/, std::move(data));
		FORWARD_TO_LISTENERS(onMessageReceived, msg);
	}

	if (isNetworked())
	{
		auto messages = mUdpClient.poll();
		for (const auto& msg : messages)
		{
			if (msg->is<Bousk::Network::Messages::IncomingConnection>())
			{
				if (isHost())
				{
					// Only host can accept connections. Clients will silently ignore them.
					const bool acceptConnection = std::all_of(mListeners.begin(), mListeners.end(), [&](IListener* listener)
					{
						return listener->onIncomingConnection(*(msg->as<Bousk::Network::Messages::IncomingConnection>()));
					});
					if (acceptConnection)
						mUdpClient.connect(msg->emitter());
					// TODO : else, log
				}
			}
			else if (msg->is<Bousk::Network::Messages::Connection>())
			{
				mRemotePeers.insert(msg->emitter());
				FORWARD_TO_LISTENERS(onConnectionResult, *(msg->as<Bousk::Network::Messages::Connection>()));
			}
			else if (msg->is<Bousk::Network::Messages::UserData>())
			{
				FORWARD_TO_LISTENERS(onMessageReceived, *(msg->as<Bousk::Network::Messages::UserData>()));
			}
			else if (msg->is<Bousk::Network::Messages::Disconnection>())
			{
				mRemotePeers.erase(msg->emitter());
				FORWARD_TO_LISTENERS(onDisconnection, *(msg->as<Bousk::Network::Messages::Disconnection>()));
			}
		}
	}
}
void NetService::flush()
{
	assert(isInitialized());
	if (isNetworked())
		mUdpClient.processSend();
}

void NetService::sendTo(const Bousk::Network::Address& target, const Bousk::uint8* data, const size_t datasize)
{
	assert(isInitialized());
	if (target == mLocalAddress)
		mLoopbackData.emplace_back(data, data + datasize);
	else if (isNetworked())
		mUdpClient.sendTo(target, data, datasize, 0);
}
void NetService::sendToHost(const Bousk::uint8* data, const size_t datasize)
{
	assert(isInitialized());
	if (isNetworked() && !isHost())
		sendTo(mContext.hostAddress, data, datasize);
	else
		mLoopbackData.emplace_back(data, data + datasize);
}
void NetService::sendToRemotes(const Bousk::uint8* data, const size_t datasize)
{
	assert(isInitialized() && (!isNetworked() || isHost()));
	for (const auto& target : mRemotePeers)
		sendTo(target, data, datasize);
}
void NetService::sendAll(const Bousk::uint8* data, const size_t datasize)
{
	assert(isInitialized() && (!isNetworked() || isHost()));
	
	// Send to distant peer, if any
	if (isNetworked() && isHost())
		sendToRemotes(data, datasize);

	// Send locally if needed
	if (!isNetworked() || isHost())
		mLoopbackData.emplace_back(data, data + datasize);
}

#undef FORWARD_TO_LISTENERS