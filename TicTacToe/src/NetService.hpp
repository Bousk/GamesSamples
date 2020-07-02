#pragma once

#include <Address.hpp>
#include <Messages.hpp>
#include <UDP/UDPClient.hpp>

#include <unordered_set>

class NetService
{
public:
	struct Parameters
	{
		Bousk::Network::Address hostAddress;
		Bousk::uint16 localPort{ 0 };
		bool networked{ false };
		bool host{ false };
	};
	class IListener
	{
	public:
		virtual void onServiceInitialized() {}
		virtual void onServiceReleased() {}

		virtual bool onIncomingConnection(const Bousk::Network::Messages::IncomingConnection&) { return true; }
		virtual void onConnectionResult(const Bousk::Network::Messages::Connection&) {}
		virtual void onDisconnection(const Bousk::Network::Messages::Disconnection&) {}
		virtual void onDataReceived(const Bousk::Network::Messages::UserData&) {}

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

	inline void addListener(IListener* listener) { mListeners.insert(listener); }
	inline void removeListener(IListener* listener) { mListeners.erase(listener); }

	inline bool isInitialized() const { return mState == State::Initialized; }
	inline bool isNetworked() const { return mContext.networked; }
	inline bool isHost() const { return mContext.host; }

	//inline Bousk::uint16 port() const { return mUdpClient.}

	void sendTo(const Bousk::Network::Address& target, const Bousk::uint8* data, const size_t datasize);

private:
	std::unordered_set<IListener*> mListeners;
	Bousk::Network::UDP::Client mUdpClient;
	Parameters mContext;
	enum class State {
		Idle,
		Initialized,
	};
	State mState{ State::Idle };
};