#include <main.hpp>

#include <Errors.hpp>
#include <Messages.hpp>
#include <Sockets.hpp>
#include <Serialization/Deserializer.hpp>
#include <Serialization/Serializer.hpp>
#include <UDP/Protocols/ReliableOrdered.hpp>
#include <UDP/UDPClient.hpp>

#include <iostream>

static constexpr Bousk::uint16 HostPort = 8888;

int main_p2p(const bool isHost)
{
    if (!Bousk::Network::Start())
    {
        std::cout << "Network lib initialisation error : " << Bousk::Network::Errors::Get();
        return -1;
    }

    Bousk::Network::UDP::Client client;
    client.registerChannel<Bousk::Network::UDP::Protocols::ReliableOrdered>();
    // Initialize client with any port, host has a fix port for client to connect to
    const Bousk::uint16 port = isHost ? HostPort : 0;
    if (!client.init(port))
    {
        std::cout << "Client initialisation error : " << Bousk::Network::Errors::Get();
        Bousk::Network::Release();
        return -2;
    }

    Bousk::Network::Address opponent;
    if (!isHost)
    {
        const Bousk::Network::Address host = Bousk::Network::Address::Loopback(Bousk::Network::Address::Type::IPv4, HostPort);
        opponent = host;
        client.connect(host);
    }
    else
    {
        std::cout << "Waiting for opponent..." << std::endl;
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("TicTacToe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_OPENGL);

    std::string baseTitle = "TicTacToe - ";
    baseTitle += isHost ? "Host" : "Client";
    auto updateWindowTitle = [&](const char* suffix)
    {
        std::string title = baseTitle + " - " + suffix;
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
                            client.sendTo(opponent, serializer.buffer(), serializer.bufferSize(), 0);
                            setState(State::OpponentTurn);
                        }
                    }
                }
            }
        }
        // Receive network data
        client.receive();
        // Process network data
        auto messages = client.poll();
        for (const auto& msg : messages)
        {
            if (msg->is<Bousk::Network::Messages::IncomingConnection>())
            {
                if (isHost && state == State::WaitingOpponent)
                {
                    // Accept opponent
                    opponent = msg->emitter();
                    client.connect(opponent);
                    setState(State::WaitingConnection);
                }
            }
            else if (msg->is<Bousk::Network::Messages::Connection>())
            {
                if (state == State::WaitingConnection)
                {
                    const Bousk::Network::Messages::Connection* connectionMsg = msg->as<Bousk::Network::Messages::Connection>();
                    if (connectionMsg->result == Bousk::Network::Messages::Connection::Result::Success)
                    {
                        // Host plays first
                        setState(isHost ? State::MyTurn : State::OpponentTurn);
                    }
                    else if (isHost)
                    {
                        // Go back to waiting an opponent
                        setState(State::WaitingOpponent);
                    }
                }
            }
            else if (msg->is<Bousk::Network::Messages::UserData>())
            {
                const Bousk::Network::Messages::UserData* data = msg->as<Bousk::Network::Messages::UserData>();
                Bousk::Serialization::Deserializer deserializer(data->data.data(), data->data.size());
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
            }
            else if (msg->is<Bousk::Network::Messages::Disconnection>())
            {
                updateWindowTitle("Disconnected");
            }
        }
        // Send network data
        client.processSend();

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
    Bousk::Network::Release();
    return 0;
}