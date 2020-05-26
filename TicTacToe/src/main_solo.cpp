#include <main.hpp>

int main_solo()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "TicTacToe",                  // window title
        SDL_WINDOWPOS_CENTERED,           // initial x position
        SDL_WINDOWPOS_CENTERED,           // initial y position
        WIN_W,                               // width, in pixels
        WIN_H,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );
    const std::string baseTitle = "TicTacToe";
    auto updateWindowTitle = [&](const char* suffix)
    {
        std::string title = baseTitle + " - " + suffix;
        SDL_SetWindowTitle(window, title.c_str());
    };
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load textures to display : None, X & O
    std::array<SDL_Texture*, 3> plays{ LoadTexture("Empty.bmp", renderer), LoadTexture("X.bmp", renderer), LoadTexture("O.bmp", renderer) };

    TicTacToe::Grid game;
    const TicTacToe::Case players[2] = { TicTacToe::Case::X, TicTacToe::Case::O };
    unsigned int playingPlayer = 0;
    updateWindowTitle(players[playingPlayer] == TicTacToe::Case::X ? "X" : "O");
    while (1)
    {
        const TicTacToe::Case currentPlayer = players[playingPlayer];
        SDL_Event e;
        if (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                break;
            }
            if (e.type == SDL_MOUSEBUTTONUP && !game.isFinished())
            {
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    if (e.button.x >= 0 && e.button.x <= WIN_W
                        && e.button.y >= 0 && e.button.y <= WIN_H)
                    {
                        // Click released on the window : play ?
                        const unsigned int caseX = static_cast<unsigned int>(e.button.x / CASE_W);
                        const unsigned int caseY = static_cast<unsigned int>(e.button.y / CASE_H);
                        if (game.play(caseX, caseY, currentPlayer))
                        {
                            playingPlayer = (playingPlayer + 1) % 2;
                            updateWindowTitle(players[playingPlayer] == TicTacToe::Case::X ? "X" : "O");
                        }
                    }
                }
            }
        }

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
            const TicTacToe::Case winner = game.winner();
            if (winner == TicTacToe::Case::X)
                updateWindowTitle("X wins");
            else if (winner == TicTacToe::Case::O)
                updateWindowTitle("O wins");
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