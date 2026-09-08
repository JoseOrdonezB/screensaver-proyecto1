#include "simulation.h"

#include <cmath>
#include <cstddef>

#include <omp.h>

namespace {

// Aplica el esquema de distribución de OpenMP configurado en la simulación.
void applySchedule(const SimulationConfig& config) {
    switch (config.scheduleKind) {
        case ScheduleKind::Dynamic:
            omp_set_schedule(omp_sched_dynamic, 1);
            break;

        case ScheduleKind::Guided:
            omp_set_schedule(omp_sched_guided, 1);
            break;

        case ScheduleKind::Static:
        default:
            // Bloque entero por hilo: comportamiento por defecto.
            omp_set_schedule(omp_sched_static, 0);
            break;
    }
}

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

    // Prepara el esquema de distribución antes de crear el equipo de hilos.
    applySchedule(config);

    /*
     * Cada iteración modifica una partícula diferente.
     * Por eso no se necesita critical, atomic o mutex.
     */
    #pragma omp parallel for schedule(runtime) \
        num_threads(config.threadCount)
    for (long long i = 0; i < particleCount; ++i) {
        updateParticle(
            particles[static_cast<std::size_t>(i)],
            config
        );
    }
}