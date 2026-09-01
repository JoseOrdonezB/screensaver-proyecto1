#pragma once

#include "simulation.h"

#include <cstdint>
#include <vector>

#include <SDL2/SDL.h>

struct Renderer {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    int width = 0;
    int height = 0;

    std::vector<std::uint32_t> pixels;

    std::uint32_t backgroundColor = 0xFF101828;
};

bool initRenderer(Renderer& renderer, int width, int height);

void clearRenderer(Renderer& renderer);

void drawRendererSequential(
    Renderer& renderer,
    const std::vector<Particle>& particles
);

void presentRenderer(Renderer& renderer);

void shutdownRenderer(Renderer& renderer);