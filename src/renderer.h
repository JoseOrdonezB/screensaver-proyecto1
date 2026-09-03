#pragma once

#include "simulation.h"

#include <cstdint>
#include <vector>

#include <SDL2/SDL.h>

// Almacena la ventana SDL y el framebuffer de píxeles que se dibuja en pantalla.
struct Renderer {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    int width = 0;
    int height = 0;

    std::vector<std::uint32_t> pixels;

    std::uint32_t backgroundColor = 0xFF101828;
};

// Crea la ventana SDL y el framebuffer a partir de las dimensiones dadas.
bool initRenderer(Renderer& renderer, int width, int height);

// Limpia el framebuffer para dibujar el siguiente frame.
void clearRenderer(Renderer& renderer);

// Dibuja todas las partículas en el framebuffer.
void drawRendererSequential(
    Renderer& renderer,
    const std::vector<Particle>& particles
);

// Muestra el framebuffer en pantalla.
void presentRenderer(Renderer& renderer);

// Libera los recursos SDL creados al inicializar el renderer.
void shutdownRenderer(Renderer& renderer);