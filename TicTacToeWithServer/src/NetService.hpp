#pragma once

#include <Address.hpp>
#include <Messages.hpp>
#include <UDP/UDPClient.hpp>

#include <Net.hpp>

#include <set>
#include <unordered_set>

class NetService
{
public:
	enum class NetworkType {
		None,
		Client,
		Host,
		DedicatedServer,
	};
	struct Parameters
	{
		Bousk::Network::Address hostAddress;
		Bousk::uint16 localPort{ 0 };
		NetworkType networkType{ NetworkType::None };

		inline bool isNetworked() const { return networkType != NetworkType::None; }
		inline bool isHost() const { return networkType == NetworkType::Host || networkType == NetworkType::DedicatedServer; }
		inline bool isDedicatedServer() const { return networkType == NetworkType::DedicatedServer; }
	};
	class IListener
	{
	public:
		virtual void onServiceInitialized() {}
		virtual void onServiceReleased() {}

		virtual bool onIncomingConnection(const Bousk::Network::Messages::IncomingConnection&) { return true; }
		virtual void onConnectionResult(const Bousk::Network::Messages::Connection&) {}
		virtual void onDisconnection(const Bousk::Network::Messages::Disconnection&) {}
		virtual void onMessageReceived(const Bousk::Network::Messages::UserData&) {}

	protected:
		virtual ~IListener() = default;
	};
public:
	NetService();
	bool init(const Parameters& parameters);
	void release();

	void receive();
	void process();
	void flush();

	const Bousk::Network::Address& localAddress() const { return mLocalAddress; }
	const std::set<Bousk::Network::Address>& remotePeers() const { return mRemotePeers; }

	inline void addListener(IListener* listener) { mListeners.insert(listener); }
	inline void removeListener(IListener* listener) { mListeners.erase(listener); }

	inline bool isInitialized() const { return mState == State::Initialized; }
	inline bool isNetworked() const { return mContext.isNetworked(); }
	inline bool isHost() const { return mContext.isHost(); }
	inline bool isDedicatedServer() const { return mContext.isDedicatedServer(); }

	void sendTo(const Bousk::Network::Address& target, const Bousk::uint8* data, const size_t datasize);
	void sendToHost(const Bousk::uint8* data, const size_t datasize);
	void sendToRemotes(const Bousk::uint8* data, const size_t datasize);
	void sendAll(const Bousk::uint8* data, const size_t datasize);

private:
	Bousk::Network::Address mLocalAddress;
	std::set<Bousk::Network::Address> mRemotePeers;
	// TODO ? save those as Message to not serialize/deserialize locally
	std::vector<std::vector<Bousk::uint8>> mLoopbackData;
	std::unordered_set<IListener*> mListeners;
	Bousk::Network::UDP::Client mUdpClient;
	Parameters mContext;
	enum class State {
		Idle,
		Initialized,
	};
	State mState{ State::Idle };
};