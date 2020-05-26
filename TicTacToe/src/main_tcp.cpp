#include <main.hpp>

#include <Address.hpp>
#include <Errors.hpp>
#include <Sockets.hpp>
#include <Serialization/Deserializer.hpp>
#include <Serialization/Serializer.hpp>
#include <TCP/Client.hpp>
#include <TCP/Server.hpp>

#include <iostream>

#define SERVER_PORT (8888)

int main_tcp(const bool isServer)
{
    if (!Bousk::Network::Start())
    {
        std::cout << "Network lib initialisation error : " << Bousk::Network::Errors::Get();
        return -1;
    }

    Bousk::Network::TCP::Server server;
    Bousk::Network::TCP::Client client;
    if (isServer)
    {
        if (!server.start(SERVER_PORT))
        {
            std::cout << "Server initialisation error : " << Bousk::Network::Errors::Get();
            Bousk::Network::Release();
            return -2;
        }
        std::cout << "Waiting for players..." << std::endl;
    }
    else
    {
        const Bousk::Network::Address serverAddress = Bousk::Network::Address::Loopback(Bousk::Network::Address::Type::IPv4, SERVER_PORT);
        if (!client.connect(serverAddress))
        {
            std::cout << "Client initialisation error : " << Bousk::Network::Errors::Get();
            Bousk::Network::Release();
            return -2;
        }
        std::cout << "Connecting to server..." << std::endl;
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "TicTacToe",                  // window title
        SDL_WINDOWPOS_CENTERED,           // initial x position
        SDL_WINDOWPOS_CENTERED,           // initial y position
        WIN_W,                               // width, in pixels
        WIN_H,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    enum class State {
        WaitingConnection,
        WaitingGameStart,
        MyTurn,
        OpponentTurn,
        Finished,
    };
    // Always wait for game start : server will wait for players, clients will wait for connection then Start message
    State state = State::WaitingGameStart;

    // Load textures to display : None, X & O
    std::array<SDL_Texture*, 3> plays{ LoadTexture("Empty.bmp", renderer), LoadTexture("X.bmp", renderer), LoadTexture("O.bmp", renderer) };
    std::array<SDL_Texture*, 3> results{ LoadTexture("Tie.bmp", renderer), LoadTexture("Victory.bmp", renderer), LoadTexture("Lose.bmp", renderer) };

    TicTacToe::Grid game;
    // Wait for server to tell us what we play
    TicTacToe::Case localPlayerSymbol = TicTacToe::Case::Empty;
    TicTacToe::Case opponentSymbol = TicTacToe::Case::Empty;
    while (1)
    {
        SDL_Event e;
        if (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                break;
            }
            //if (e.type == SDL_MOUSEBUTTONUP)
            //{
            //    if (e.button.button == SDL_BUTTON_LEFT)
            //    {
            //        if (e.button.x >= 0 && e.button.x <= WIN_W
            //            && e.button.y >= 0 && e.button.y <= WIN_H)
            //        {
            //            // Click released on the window : play ?
            //            const unsigned int caseX = static_cast<unsigned int>(e.button.x / CASE_W);
            //            const unsigned int caseY = static_cast<unsigned int>(e.button.y / CASE_H);
            //            if (game.play(caseX, caseY, localPlayerSymbol))
            //            {
            //                TicTacToe::Net::Play msg;
            //                msg.x = caseX;
            //                msg.y = caseY;
            //                Bousk::Serialization::Serializer serializer;
            //                if (!msg.write(serializer))
            //                {
            //                    std::cout << "Critical error : failed to serialize play packet" << std::endl;
            //                    assert(false);
            //                }
            //                client.sendTo(opponent, serializer.buffer(), serializer.bufferSize(), 0);
            //            }
            //        }
            //    }
            //}
        }
        // Receive network data
        //client.receive();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
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
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        // Horizontal
        SDL_RenderDrawLine(renderer, 0, CASE_H, WIN_W, CASE_H);
        SDL_RenderDrawLine(renderer, 0, CASE_H * 2, WIN_W, CASE_H * 2);
        // Vertical
        SDL_RenderDrawLine(renderer, CASE_W, 0, CASE_W, WIN_H);
        SDL_RenderDrawLine(renderer, CASE_W * 2, 0, CASE_W * 2, WIN_H);

        if (game.isFinished())
        {
            const SDL_Rect position{ CASE_W, 0, CASE_W, CASE_H / 2 };
            SDL_RenderCopy(renderer, results[static_cast<unsigned int>(game.winner())], NULL, &position);
        }

        SDL_RenderPresent(renderer);
        // Send network data
        //client.processSend();
        SDL_Delay(1);
    }

    for (SDL_Texture* texture : plays)
    {
        SDL_DestroyTexture(texture);
    }
    for (SDL_Texture* texture : results)
    {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    Bousk::Network::Release();
    return 0;
}