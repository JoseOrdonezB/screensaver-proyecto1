#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
std::uint32_t packColor(
    const std::uint8_t red,
    const std::uint8_t green,
    const std::uint8_t blue,
    const std::uint8_t alpha
) {
    return
        (static_cast<std::uint32_t>(alpha) << 24) |
        (static_cast<std::uint32_t>(red) << 16) |
        (static_cast<std::uint32_t>(green) << 8) |
        static_cast<std::uint32_t>(blue);
}

void fillParticleCircle(
    Renderer& renderer,
    const Particle& particle
) {
    const int centerX = static_cast<int>(particle.x);
    const int centerY = static_cast<int>(particle.y);
    const int radius =
        static_cast<int>(std::ceil(particle.radius));

    const int minX = std::max(0, centerX - radius);
    const int maxX =
        std::min(renderer.width - 1, centerX + radius);
    const int minY = std::max(0, centerY - radius);
    const int maxY =
        std::min(renderer.height - 1, centerY + radius);

    const float radiusSquared =
        particle.radius * particle.radius;

    const std::uint32_t color = packColor(
        particle.red,
        particle.green,
        particle.blue,
        particle.alpha
    );

    for (int y = minY; y <= maxY; ++y) {
        const float dy = y - particle.y;
        const float dySquared = dy * dy;

        std::size_t index =
            static_cast<std::size_t>(y) *
                static_cast<std::size_t>(renderer.width) +
            static_cast<std::size_t>(minX);

        for (int x = minX; x <= maxX; ++x) {
            const float dx = x - particle.x;

            if (dx * dx + dySquared <= radiusSquared) {
                renderer.pixels[index] = color;
            }

            ++index;
        }
    }
}

}

bool initRenderer(Renderer& renderer, const int width, const int height) {
    if (width <= 0 || height <= 0) {
        std::cerr
            << "Error: dimensiones de pantalla invalidas "
            << '(' << width << "x" << height << ").\n";
        return false;
    }

    renderer.window = SDL_CreateWindow(
        "Screensaver Paralelo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (renderer.window == nullptr) {
        std::cerr
            << "Error al crear la ventana: "
            << SDL_GetError()
            << '\n';
        return false;
    }

    renderer.renderer = SDL_CreateRenderer(
        renderer.window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (renderer.renderer == nullptr) {
        std::cerr
            << "Error al crear el renderer: "
            << SDL_GetError()
            << '\n';
        shutdownRenderer(renderer);
        return false;
    }

    renderer.texture = SDL_CreateTexture(
        renderer.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (renderer.texture == nullptr) {
        std::cerr
            << "Error al crear la textura: "
            << SDL_GetError()
            << '\n';
        shutdownRenderer(renderer);
        return false;
    }

    renderer.width = width;
    renderer.height = height;
    renderer.pixels.assign(
        static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height),
        renderer.backgroundColor
    );

    return true;
}

void clearRenderer(Renderer& renderer) {
    for (std::uint32_t& pixel : renderer.pixels) {
        pixel = renderer.backgroundColor;
    }
}

void drawRendererSequential(
    Renderer& renderer,
    const std::vector<Particle>& particles
) {
    for (const Particle& particle : particles) {
        fillParticleCircle(renderer, particle);
    }
}

void presentRenderer(Renderer& renderer) {
    const int pitch = renderer.width * static_cast<int>(sizeof(std::uint32_t));

    SDL_UpdateTexture(
        renderer.texture,
        nullptr,
        renderer.pixels.data(),
        pitch
    );

    SDL_RenderCopy(renderer.renderer, renderer.texture, nullptr, nullptr);
    SDL_RenderPresent(renderer.renderer);
}

void shutdownRenderer(Renderer& renderer) {
    if (renderer.texture != nullptr) {
        SDL_DestroyTexture(renderer.texture);
        renderer.texture = nullptr;
    }

    if (renderer.renderer != nullptr) {
        SDL_DestroyRenderer(renderer.renderer);
        renderer.renderer = nullptr;
    }

    if (renderer.window != nullptr) {
        SDL_DestroyWindow(renderer.window);
        renderer.window = nullptr;
    }

    renderer.pixels.clear();
    renderer.width = 0;
    renderer.height = 0;
}