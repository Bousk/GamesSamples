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
    if (parameters.networked)
    {
        if (!Bousk::Network::Start())
        {
            return false;
        }
        if (!mUdpClient.init(parameters.localPort))
            return false;
        if (!parameters.host)
        {
            // If we're not host, initialize connection right away
            mUdpClient.connect(parameters.hostAddress);
        }
    }
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
    mState = State::Idle;
    FORWARD_TO_LISTENERS(onServiceReleased);
}

void NetService::receive()
{
	if (isInitialized() && isNetworked())
		mUdpClient.receive();
}
void NetService::process()
{
	if (isInitialized() && isNetworked())
	{
		auto messages = mUdpClient.poll();
        for (const auto& msg : messages)
        {
            if (msg->is<Bousk::Network::Messages::IncomingConnection>())
            {
                if (isHost())
                {
                    bool acceptConnection = true;
                    // Only host can accept connections. Clients will silently ignore them.
                    for (IListener* listener : mListeners)
                    {
                        acceptConnection &= listener->onIncomingConnection(*(msg->as<Bousk::Network::Messages::IncomingConnection>()));
                    }
                    if (acceptConnection)
                        mUdpClient.connect(msg->emitter());
                }
            }
            else if (msg->is<Bousk::Network::Messages::Connection>())
            {
                FORWARD_TO_LISTENERS(onConnectionResult, *msg->as<Bousk::Network::Messages::Connection>());
            }
            else if (msg->is<Bousk::Network::Messages::UserData>())
            {
                FORWARD_TO_LISTENERS(onDataReceived, *msg->as<Bousk::Network::Messages::UserData>());
            }
            else if (msg->is<Bousk::Network::Messages::Disconnection>())
            {
                FORWARD_TO_LISTENERS(onDisconnection, *(msg->as<Bousk::Network::Messages::Disconnection>()));
            }
        }
	}
}
void NetService::flush()
{
	if (isInitialized() && isNetworked())
		mUdpClient.processSend();
}

void NetService::sendTo(const Bousk::Network::Address& target, const Bousk::uint8* data, const size_t datasize)
{
    if (isInitialized() && isNetworked())
        mUdpClient.sendTo(target, data, datasize, 0);
}

#undef FORWARD_TO_LISTENERS