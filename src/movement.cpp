#include "simulation.h"

#include <cmath>
#include <cstddef>

namespace {

// Resuelve los rebotes contra los cuatro bordes del canvas.
void resolveWallBounce(
    Particle& particle,
    const SimulationConfig& config
) {
    // Borde izquierdo.
    if (particle.x - particle.radius < 0.0f) {
        particle.x = particle.radius;

        particle.velocityX =
            std::abs(particle.velocityX) * config.wallRestitution;
    }
    // Borde derecho.
    else if (particle.x + particle.radius >
             static_cast<float>(config.width)) {
        particle.x =
            static_cast<float>(config.width) - particle.radius;

        particle.velocityX =
            -std::abs(particle.velocityX) * config.wallRestitution;
    }

    // Borde superior.
    if (particle.y - particle.radius < 0.0f) {
        particle.y = particle.radius;

        particle.velocityY =
            std::abs(particle.velocityY) * config.wallRestitution;
    }
    // Borde inferior.
    else if (particle.y + particle.radius >
             static_cast<float>(config.height)) {
        particle.y =
            static_cast<float>(config.height) - particle.radius;

        particle.velocityY =
            -std::abs(particle.velocityY) * config.wallRestitution;
    }
}

// Actualiza una sola partícula.
void updateParticle(
    Particle& particle,
    const SimulationConfig& config
) {
    // Movimiento: posicion nueva = posicion anterior + velocidad * tiempo.
    particle.x += particle.velocityX * config.deltaTime;
    particle.y += particle.velocityY * config.deltaTime;

    resolveWallBounce(particle, config);
}

}  // namespace

void updateSequential(
    std::vector<Particle>& particles,
    const SimulationConfig& config
) {
    for (Particle& particle : particles) {
        updateParticle(particle, config);
    }
}

void updateParallel(
    std::vector<Particle>& particles,
    const SimulationConfig& config
) {
    const long long particleCount =
        static_cast<long long>(particles.size());

    /*
     * Cada iteración modifica una partícula diferente.
     * Por eso no se necesita critical, atomic o mutex.
     */
    #pragma omp parallel for schedule(static) \
        num_threads(config.threadCount)
    for (long long i = 0; i < particleCount; ++i) {
        updateParticle(
            particles[static_cast<std::size_t>(i)],
            config
        );
    }
}