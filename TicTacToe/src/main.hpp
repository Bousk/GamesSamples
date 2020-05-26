#pragma once

#include <Game.hpp>
#include <Net.hpp>

#include <SDL.h>

#define CASE_W (200)
#define CASE_H (200)
#define WIN_W (3*CASE_W)
#define WIN_H (3*CASE_H)

inline SDL_Texture* LoadTexture(const char* filepath, SDL_Renderer* renderer)
{
    if (SDL_Surface* surface = SDL_LoadBMP(filepath))
    {
        //SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 174, 201));
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }
    return nullptr;
}