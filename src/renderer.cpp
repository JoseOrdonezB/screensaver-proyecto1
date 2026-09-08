#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

// Tamaño del halo luminoso en función del radio de la partícula.
constexpr float GLOW_MULTIPLIER = 1.8f;

// Exponente del decaimiento del brillo: mayor valor, halo mas compacto.
constexpr float GLOW_FALLOFF = 2.0f;

// Empaqueta RGBA en un único entero de 32 bits.
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

// Extrae el componente rojo de un pixel ARGB.
std::uint8_t extractRed(const std::uint32_t pixel) {
    return static_cast<std::uint8_t>((pixel >> 16) & 0xFF);
}

// Extrae el componente verde de un pixel ARGB.
std::uint8_t extractGreen(const std::uint32_t pixel) {
    return static_cast<std::uint8_t>((pixel >> 8) & 0xFF);
}

// Extrae el componente azul de un pixel ARGB.
std::uint8_t extractBlue(const std::uint32_t pixel) {
    return static_cast<std::uint8_t>(pixel & 0xFF);
}

// Mezcla un color con el fondo según un factor alpha entre 0.0 y 1.0.
std::uint32_t blendWithBackground(
    const std::uint32_t backgroundColor,
    const std::uint8_t red,
    const std::uint8_t green,
    const std::uint8_t blue,
    const float alpha
) {
    const float bgRed = extractRed(backgroundColor);
    const float bgGreen = extractGreen(backgroundColor);
    const float bgBlue = extractBlue(backgroundColor);

    const float outRed = alpha * red + (1.0f - alpha) * bgRed;
    const float outGreen = alpha * green + (1.0f - alpha) * bgGreen;
    const float outBlue = alpha * blue + (1.0f - alpha) * bgBlue;

    return packColor(
        static_cast<std::uint8_t>(outRed),
        static_cast<std::uint8_t>(outGreen),
        static_cast<std::uint8_t>(outBlue),
        255
    );
}

// Rellena el círculo de una partícula y su halo luminoso.
void fillParticleCircle(
    Renderer& renderer,
    const Particle& particle
) {
    const int centerX = static_cast<int>(particle.x);
    const int centerY = static_cast<int>(particle.y);

    const float radius = particle.radius;
    const float glowRadius = radius * GLOW_MULTIPLIER;

    // El bounding box cubre tambien el halo externo.
    const int glowRadiusPixels =
        static_cast<int>(std::ceil(glowRadius));

    const int minX = std::max(0, centerX - glowRadiusPixels);
    const int maxX =
        std::min(renderer.width - 1, centerX + glowRadiusPixels);
    const int minY = std::max(0, centerY - glowRadiusPixels);
    const int maxY =
        std::min(renderer.height - 1, centerY + glowRadiusPixels);

    const float radiusSquared = radius * radius;
    const float glowRadiusSquared = glowRadius * glowRadius;

    // Color solido del nucleo de la particula.
    const std::uint32_t coreColor = packColor(
        particle.red,
        particle.green,
        particle.blue,
        particle.alpha
    );

    const float glowWidth =
        std::max(glowRadius - radius, 1.0e-6f);

    for (int y = minY; y <= maxY; ++y) {
        const float dy = y - particle.y;
        const float dySquared = dy * dy;

        std::size_t index =
            static_cast<std::size_t>(y) *
                static_cast<std::size_t>(renderer.width) +
            static_cast<std::size_t>(minX);

        for (int x = minX; x <= maxX; ++x) {
            const float dx = x - particle.x;
            const float distanceSquared = dx * dx + dySquared;

            // Nucleo solido.
            if (distanceSquared <= radiusSquared) {
                renderer.pixels[index] = coreColor;
            }
            // Halo luminoso con decaimiento hacia el fondo.
            else if (distanceSquared <= glowRadiusSquared) {
                const float distance = std::sqrt(distanceSquared);
                const float normalized =
                    (distance - radius) / glowWidth;
                const float alpha =
                    std::max(0.0f, 1.0f - std::pow(normalized, GLOW_FALLOFF));

                renderer.pixels[index] = blendWithBackground(
                    renderer.backgroundColor,
                    particle.red,
                    particle.green,
                    particle.blue,
                    alpha
                );
            }

            ++index;
        }
    }
}

}

// Crea la ventana SDL, el renderer y la textura que muestra el framebuffer.
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

// Pinta todo el framebuffer con el color de fondo.
void clearRenderer(Renderer& renderer) {
    for (std::uint32_t& pixel : renderer.pixels) {
        pixel = renderer.backgroundColor;
    }
}

// Dibuja todas las partículas en el framebuffer de píxeles.
void drawRendererSequential(
    Renderer& renderer,
    const std::vector<Particle>& particles
) {
    for (const Particle& particle : particles) {
        fillParticleCircle(renderer, particle);
    }
}

// Envía el contenido del framebuffer a la pantalla con SDL.
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

// Libera la textura, el renderer y la ventana creados por SDL.
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