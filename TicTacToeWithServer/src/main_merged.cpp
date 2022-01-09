#include <main.hpp>

#include <Errors.hpp>
#include <Messages.hpp>
#include <Serialization/Deserializer.hpp>
#include <Serialization/Serializer.hpp>

#include <Net.hpp>
#include <NetService.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <random>

static constexpr Bousk::uint16 HostPort = 8888;

class NetListener : public NetService::IListener
{
    using OnIncomingConnectionCallback = std::function<bool(const Bousk::Network::Messages::IncomingConnection&)>;
    using OnConnectionCallback = std::function<void(const Bousk::Network::Messages::Connection&)>;
    using OnDisconnectionCallback = std::function<void(const Bousk::Network::Messages::Disconnection&)>;
    using OnMessageReceivedCallback = std::function<void(const TicTacToe::Net::Message&)>;
public:
    OnIncomingConnectionCallback mOnIncomingConnection;
    OnConnectionCallback mOnConnectionResult;
    OnDisconnectionCallback mOnDisconnection;
    OnMessageReceivedCallback mOnMessageReceived;

private:
    bool onIncomingConnection(const Bousk::Network::Messages::IncomingConnection& incomingConnection) override
    {
        return !mOnIncomingConnection || mOnIncomingConnection(incomingConnection);
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

    void onMessageReceived(const Bousk::Network::Messages::UserData& userdata) override
    {
        if (mOnMessageReceived)
        {
            if (auto msg = TicTacToe::Net::Message::Deserialize(userdata.data.data(), userdata.data.size()))
                mOnMessageReceived(*msg);
        }
    }
};

int main_game(const NetService::NetworkType networkType)
{
    // Use a heap allocation to prevent stack size warning since NetService is quite big
    std::unique_ptr<NetService> netService = std::make_unique<NetService>();
    {
        NetService::Parameters netServiceParameters;
        netServiceParameters.networkType = networkType;
        netServiceParameters.localPort = netServiceParameters.isHost() ? HostPort : 0;
        netServiceParameters.hostAddress = Bousk::Network::Address::Loopback(Bousk::Network::Address::Type::IPv4, HostPort);
        if (!netService->init(netServiceParameters))
        {
            std::cout << "NetService initialization error : " << Bousk::Network::Errors::Get();
            return -1;
        }
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("TicTacToe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_OPENGL);

    constexpr std::string_view baseTitle = "TicTacToe - ";
    auto updateWindowTitle = [&](const char* suffix)
    {
        std::string title(baseTitle);
        if (netService->isNetworked())
        {
            if (netService->isDedicatedServer())
                title += "Server";
            else if (netService->isHost())
                title += "Host";
            else
                title += "Client";
        }
        else
            title += "Offline";
        title += " - ";
        title += suffix;
        SDL_SetWindowTitle(window, title.c_str());
    };
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    enum class State {
        WaitingOpponent,
        WaitingConnection,
        WaitingGameStart,
        Playing,
        PlayRequested,
        Finished,
    };
    State state;
    TicTacToe::Game game;
    TicTacToe::Case localPlayerSymbol = TicTacToe::Case::Empty;
    // Server specific data : symbol & ip address of each player
    struct {
        struct Player {
            TicTacToe::Case symbol{ TicTacToe::Case::Empty };
            Bousk::Network::Address address;
        };
        std::vector<Player> players;
    } serverData;
    // If we're host but not dedicated server, we locally play
    if (netService->isHost() && !netService->isDedicatedServer())
    {
        serverData.players.push_back({ TicTacToe::Case::Empty, netService->localAddress() });
    }
    auto isMyTurn = [&]() { assert(!netService->isDedicatedServer()); return game.currentPlayer() == localPlayerSymbol; };
    auto setState = [&](State newState)
    {
        auto updatePlayingState = [&]()
        {
            if (netService->isNetworked())
            {
                if (netService->isDedicatedServer())
                {
                    if (game.currentPlayer() == TicTacToe::Case::X)
                    {
                        updateWindowTitle("Player X turn");
                    }
                    else
                    {
                        updateWindowTitle("Player O turn");
                    }
                }
                else if (isMyTurn())
                {
                    if (localPlayerSymbol == TicTacToe::Case::X)
                        updateWindowTitle("Your turn (X)");
                    else
                        updateWindowTitle("Your turn (O)");
                }
                else
                {
                    updateWindowTitle("Opponent turn");
                }
            }
            else if (game.currentPlayer() == TicTacToe::Case::X)
            {
                updateWindowTitle("Player X turn");
            }
            else
            {
                updateWindowTitle("Player O turn");
            }
        };
        auto updateStateFinished = [&]()
        {
            const TicTacToe::Case winner = game.grid().winner();
            if (winner == TicTacToe::Case::Empty)
                updateWindowTitle("Draw");
            else
            {
                if (!netService->isNetworked() || netService->isDedicatedServer())
                {
                    updateWindowTitle((winner == TicTacToe::Case::X) ? "X win" : "O win");
                }
                else
                {
                    updateWindowTitle((winner == localPlayerSymbol) ? "You win" : "You loose");
                }
            }
        };
        state = newState;
        switch (state)
        {
            case State::WaitingOpponent: updateWindowTitle("Waiting opponent"); break;
            case State::WaitingConnection: updateWindowTitle("Waiting connection"); break;
            case State::WaitingGameStart: updateWindowTitle("Waiting game start"); break;
            case State::Playing: updatePlayingState(); break;
            case State::PlayRequested: updateWindowTitle("Waiting for server validation"); break;
            case State::Finished: updateStateFinished(); break;
        }
    };
    if (netService->isNetworked())
        setState(netService->isHost() ? State::WaitingOpponent : State::WaitingConnection);
    else
        setState(State::Playing);

    // Load textures to display : None, X & O
    const std::array<SDL_Texture*, 3> plays{ LoadTexture("Empty.bmp", renderer), LoadTexture("X.bmp", renderer), LoadTexture("O.bmp", renderer) };

    // This is only to play the validated play locally
    auto playCurrentTurnLocally = [&](unsigned int x, unsigned int y)
    {
        return game.play(x, y);
    };
    auto requestPlay = [&](unsigned int x, unsigned int y)
    {
        TicTacToe::Net::PlayRequest msg;
        msg.x = x;
        msg.y = y;
        Bousk::Serialization::Serializer serializer;
        if (!msg.write(serializer))
        {
            std::cout << "Critical error : failed to serialize play packet" << std::endl;
            assert(false);
        }
        netService->sendToHost(serializer.buffer(), serializer.bufferSize());
        setState(State::PlayRequested);
    };

    NetListener netListener;
    netService->addListener(&netListener);
    netListener.mOnIncomingConnection = [&](const Bousk::Network::Messages::IncomingConnection& msg)
    {
        return (netService->isHost() && serverData.players.size() < 2);
    };
    netListener.mOnConnectionResult = [&](const Bousk::Network::Messages::Connection& msg)
    {
        if (netService->isHost() && msg.result == Bousk::Network::Messages::Connection::Result::Success)
        {
            serverData.players.push_back({ TicTacToe::Case::Empty, msg.emitter() });

            // Enough player to start a game
            if (serverData.players.size() == 2)
            {
                setState(State::WaitingGameStart);
                std::array<TicTacToe::Case, 2> playerSymbols{ TicTacToe::Case::X, TicTacToe::Case::O };
                {
                    std::random_device rd;
                    std::mt19937 g(rd());
                    std::shuffle(playerSymbols.begin(), playerSymbols.end(), g);
                }
                // Assign random sign to each player
                for (size_t i = 0; i < 2; ++i)
                {
                    serverData.players[i].symbol = playerSymbols[i];

                    TicTacToe::Net::Setup msgSetup;
                    msgSetup.symbol = serverData.players[i].symbol;
                    Bousk::Serialization::Serializer serializer;
                    msgSetup.write(serializer);
                    netService->sendTo(serverData.players[i].address, serializer.buffer(), serializer.bufferSize());
                }
                // Send start game message
                {
                    TicTacToe::Net::Start msgStart;
                    msgStart.symbol = playerSymbols[0];
                    Bousk::Serialization::Serializer serializer;
                    msgStart.write(serializer);
                    netService->sendAll(serializer.buffer(), serializer.bufferSize());
                }
            }
        }
        else if (msg.result == Bousk::Network::Messages::Connection::Result::Success)
        {
            setState(State::WaitingGameStart);
        }
        else
        {
            std::cout << "Critical error : connection & status failure" << std::endl;
            assert(false);
        }
    };
    netListener.mOnMessageReceived = [&](const TicTacToe::Net::Message& msg)
    {
        switch (msg.type())
        {
            case TicTacToe::Net::Message::Type::Setup:
            {
                assert(state == State::WaitingGameStart);
                const TicTacToe::Net::Setup& setupMsg = static_cast<const TicTacToe::Net::Setup&>(msg);
                localPlayerSymbol = setupMsg.symbol;
            } break;
            case TicTacToe::Net::Message::Type::Start:
            {
                assert(state == State::WaitingGameStart);
                const TicTacToe::Net::Start& startMsg = static_cast<const TicTacToe::Net::Start&>(msg);
                game.start(startMsg.symbol);
                setState(State::Playing);
            } break;
            case TicTacToe::Net::Message::Type::PlayRequest:
            {
                assert(!netService->isNetworked() || netService->isHost());
                const TicTacToe::Net::PlayRequest& request = static_cast<const TicTacToe::Net::PlayRequest&>(msg);

                TicTacToe::Net::PlayResult result;
                result.valid = game.grid().canPlay(request.x, request.y, game.currentPlayer());
                result.x = request.x;
                result.y = request.y;
                Bousk::Serialization::Serializer serializer;
                result.write(serializer);
                
                if (!result.valid)
                {
                    // Play is invalid, send back error to player
                    // TODO
                }
                else
                {
                    // Play is valid, broadcast result to everyone
                    netService->sendAll(serializer.buffer(), serializer.bufferSize());
                }
            } break;
            case TicTacToe::Net::Message::Type::PlayResult:
            {
                const TicTacToe::Net::PlayResult& result = static_cast<const TicTacToe::Net::PlayResult&>(msg);
                if (result.valid)
                {
                    if (!playCurrentTurnLocally(result.x, result.y))
                    {
                        std::cout << "Critical error : failed to play move" << std::endl;
                        assert(false);
                    }

                    setState(State::Playing);
                    if (game.grid().isFinished())
                    {
                        setState(State::Finished);
                    }
                }
                else
                {
                    assert(state == State::PlayRequested);
                }
            } break;
            default:
                assert(false);
        }
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
            if (e.type == SDL_MOUSEBUTTONUP && (state == State::Playing && (isMyTurn() || !netService->isNetworked())))
            {
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    if (e.button.x >= 0 && e.button.x <= WIN_W
                        && e.button.y >= 0 && e.button.y <= WIN_H)
                    {
                        // Click released on the window : play ?
                        const unsigned int caseX = static_cast<unsigned int>(e.button.x / CASE_W);
                        const unsigned int caseY = static_cast<unsigned int>(e.button.y / CASE_H);
                        requestPlay(caseX, caseY);
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
                const TicTacToe::Case caseStatus = game.grid().grid()[x][y];
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