#include <main.hpp>

#include <Errors.hpp>
#include <Messages.hpp>
#include <Sockets.hpp>
#include <Serialization/Deserializer.hpp>
#include <Serialization/Serializer.hpp>
#include <UDP/UDPClient.hpp>

#include <NetService.hpp>

#include <iostream>

static constexpr Bousk::uint16 HostPort = 8888;

class NetListener : public NetService::IListener
{
    using OnIncomingConnectionCallback = std::function<bool(const Bousk::Network::Messages::IncomingConnection&)>;
    using OnConnectionCallback = std::function<void(const Bousk::Network::Messages::Connection&)>;
    using OnDisconnectionCallback = std::function<void(const Bousk::Network::Messages::Disconnection&)>;
    using OnDataReceivedCallback = std::function<void(const Bousk::Network::Messages::UserData&)>;
public:
    OnIncomingConnectionCallback mOnIncomingConnection;
    OnConnectionCallback mOnConnectionResult;
    OnDisconnectionCallback mOnDisconnection;
    OnDataReceivedCallback mOnDataReceived;

private:
    bool onIncomingConnection(const Bousk::Network::Messages::IncomingConnection& incomingConnection) override
    {
        return mOnIncomingConnection && mOnIncomingConnection(incomingConnection);
    }

    void onConnectionResult(const Bousk::Network::Messages::Connection& connection) override
    {
        if (mOnConnectionResult)
            mOnConnectionResult(connection);
    }

    void onDisconnection(const Bousk::Network::Messages::Disconnection& disconnection) override
    {
        if (mOnDisconnection)
            mOnDisconnection(disconnection);
    }

    void onDataReceived(const Bousk::Network::Messages::UserData& userdata) override
    {
        if (mOnDataReceived)
            mOnDataReceived(userdata);
    }
};

int main_merged(const bool isNetworked, const bool isHost = false)
{
    // Use a heap allocation to prevent stack size warning since NetService is quite big
    std::unique_ptr<NetService> netService = std::make_unique<NetService>();
    {
        NetService::Parameters netServiceParameters;
        netServiceParameters.networked = isNetworked;
        netServiceParameters.host = isNetworked && isHost;
        netServiceParameters.localPort = isHost ? HostPort : 0;
        if (!isHost)
            netServiceParameters.hostAddress = Bousk::Network::Address::Loopback(Bousk::Network::Address::Type::IPv4, HostPort);
        if (!netService->init(netServiceParameters))
        {
            std::cout << "NetService initialisation error : " << Bousk::Network::Errors::Get();
            return -1;
        }
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("TicTacToe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_OPENGL);

    constexpr std::string_view baseTitle = "TicTacToe - ";
    auto updateWindowTitle = [&](const char* suffix)
    {
        std::string title = std::string(baseTitle) + (isHost ? "Host" : "Client") + " - " + suffix;
        SDL_SetWindowTitle(window, title.c_str());
    };
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    enum class State {
        WaitingOpponent,
        WaitingConnection,
        MyTurn,
        OpponentTurn,
        Finished,
    };
    // Host wait for opponent to connect while guest connect to host right away
    State state;
    auto setState = [&](State newState)
    {
        state = newState;
        switch (state)
        {
            case State::WaitingOpponent: updateWindowTitle("Waiting opponent"); break;
            case State::WaitingConnection: updateWindowTitle("Waiting connection"); break;
            case State::MyTurn: updateWindowTitle("My turn"); break;
            case State::OpponentTurn: updateWindowTitle("Opponent turn"); break;
            case State::Finished: updateWindowTitle("Finished"); break;
        }
    };
    setState(isHost ? State::WaitingOpponent : State::WaitingConnection);

    // Load textures to display : None, X & O
    std::array<SDL_Texture*, 3> plays{ LoadTexture("Empty.bmp", renderer), LoadTexture("X.bmp", renderer), LoadTexture("O.bmp", renderer) };

    TicTacToe::Grid game;
    // Host plays X, guest plays O
    const TicTacToe::Case localPlayerSymbol = isHost ? TicTacToe::Case::X : TicTacToe::Case::O;
    // Save my opponent symbol too
    const TicTacToe::Case opponentSymbol = (localPlayerSymbol == TicTacToe::Case::X) ? TicTacToe::Case::O : TicTacToe::Case::X;
    // Will be used to store opponent address
    Bousk::Network::Address opponent;

    NetListener netListener;
    netService->addListener(&netListener);
    netListener.mOnIncomingConnection = [&](const Bousk::Network::Messages::IncomingConnection& msg)
    {
        if (netService->isHost() && state == State::WaitingOpponent)
        {
            setState(State::WaitingConnection);
            return true;
        }
        return false;
    };
    netListener.mOnConnectionResult = [&](const Bousk::Network::Messages::Connection& msg)
    {
        if (state == State::WaitingConnection)
        {
            if (msg.result == Bousk::Network::Messages::Connection::Result::Success)
            {
                // Save opponent address
                opponent = msg.emitter();
                // Host plays first
                setState(netService->isHost() ? State::MyTurn : State::OpponentTurn);
            }
            else if (netService->isHost())
            {
                // Go back to waiting an opponent
                setState(State::WaitingOpponent);
            }
        }
    };
    netListener.mOnDataReceived = [&](const Bousk::Network::Messages::UserData& msg)
    {
        Bousk::Serialization::Deserializer deserializer(msg.data.data(), msg.data.size());
        TicTacToe::Net::Play play;
        if (!play.read(deserializer))
        {
            std::cout << "Critical error : failed to deserialize play packet" << std::endl;
            assert(false);
        }
        assert(state == State::OpponentTurn);
        if (!game.play(play.x, play.y, opponentSymbol))
        {
            std::cout << "Critical error : failed to play opponent move" << std::endl;
            assert(false);
        }
        setState(State::MyTurn);
    };
    netListener.mOnDisconnection = [&](const Bousk::Network::Messages::Disconnection& msg)
    {
        updateWindowTitle("Disconnected");
    };
    while (1)
    {
        SDL_Event e;
        if (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                break;
            }
            if (e.type == SDL_MOUSEBUTTONUP && state == State::MyTurn)
            {
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    if (e.button.x >= 0 && e.button.x <= WIN_W
                        && e.button.y >= 0 && e.button.y <= WIN_H)
                    {
                        // Click released on the window : play ?
                        const unsigned int caseX = static_cast<unsigned int>(e.button.x / CASE_W);
                        const unsigned int caseY = static_cast<unsigned int>(e.button.y / CASE_H);
                        if (game.play(caseX, caseY, localPlayerSymbol))
                        {
                            TicTacToe::Net::Play msg;
                            msg.x = caseX;
                            msg.y = caseY;
                            Bousk::Serialization::Serializer serializer;
                            if (!msg.write(serializer))
                            {
                                std::cout << "Critical error : failed to serialize play packet" << std::endl;
                                assert(false);
                            }
                            netService->sendTo(opponent, serializer.buffer(), serializer.bufferSize());
                            setState(State::OpponentTurn);
                        }
                    }
                }
            }
        }
        // Receive network data
        netService->receive();
        // Process network data
        netService->process();
        // Send network data
        netService->flush();

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        // Draw cases
        for (int x = 0; x < 3; ++x)
        {
            for (int y = 0; y < 3; ++y)
            {
                const TicTacToe::Case caseStatus = game.grid()[x][y];
                const SDL_Rect position{ x * CASE_W, y * CASE_H, CASE_W, CASE_H };
                SDL_RenderCopy(renderer, plays[static_cast<unsigned int>(caseStatus)], NULL, &position);
            }
        }
        // Draw grid lines
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        // Horizontal
        SDL_RenderDrawLine(renderer, 0, CASE_H, WIN_W, CASE_H);
        SDL_RenderDrawLine(renderer, 0, CASE_H * 2, WIN_W, CASE_H * 2);
        // Vertical
        SDL_RenderDrawLine(renderer, CASE_W, 0, CASE_W, WIN_H);
        SDL_RenderDrawLine(renderer, CASE_W * 2, 0, CASE_W * 2, WIN_H);

        if (game.isFinished())
        {
            const TicTacToe::Case winner = game.winner();
            if (winner == localPlayerSymbol)
                updateWindowTitle("You win");
            else if (winner == opponentSymbol)
                updateWindowTitle("You loose");
            else
                updateWindowTitle("Draw");
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }

    for (SDL_Texture* texture : plays)
    {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}