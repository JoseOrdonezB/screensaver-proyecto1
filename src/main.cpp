#include "renderer.h"
#include "simulation.h"

#include <cstdlib>
#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr
            << "Error al inicializar SDL: "
            << SDL_GetError()
            << '\n';
        return EXIT_FAILURE;
    }

    Renderer renderer;

    if (!initRenderer(renderer, 800, 600)) {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SimulationConfig config;
    config.width = 800;
    config.height = 600;
    config.particleCount = 200;

    std::vector<Particle> particles;
    initializeParticles(particles, config);

    clearRenderer(renderer);
    drawRendererSequential(renderer, particles);
    presentRenderer(renderer);

    bool running = true;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (
                event.type == SDL_KEYDOWN &&
                event.key.keysym.sym == SDLK_ESCAPE
            ) {
                running = false;
            }
        }

        SDL_Delay(10);
    }

    shutdownRenderer(renderer);
    SDL_Quit();

    return EXIT_SUCCESS;
}