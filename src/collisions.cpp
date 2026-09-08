#include "collisions.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

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

// Normaliza un vector evitando divisiones por cero.
void normalizePair(
    const float dx,
    const float dy,
    float& normalX,
    float& normalY
) {
    const float distanceSquared =
        dx * dx + dy * dy;

    if (distanceSquared <= 1.0e-12f) {
        normalX = 1.0f;
        normalY = 0.0f;
        return;
    }

    const float distance =
        std::sqrt(distanceSquared);

    normalX = dx / distance;
    normalY = dy / distance;
}


// Verifica si dos particulas se intersectan.
bool particlesOverlap(
    const Particle& first,
    const Particle& second
) {
    const float dx =
        second.x - first.x;

    const float dy =
        second.y - first.y;

    const float distanceSquared =
        dx * dx + dy * dy;

    const float minimumDistance =
        first.radius + second.radius;

    return distanceSquared <
        minimumDistance * minimumDistance;
}


// Resuelve la colision entre dos particulas.
void resolveParticlePair(
    Particle& first,
    Particle& second,
    const SimulationConfig& config
) {
    const float dx =
        second.x - first.x;

    const float dy =
        second.y - first.y;

    const float distanceSquared =
        dx * dx + dy * dy;

    const float minimumDistance =
        first.radius + second.radius;

    const float minimumDistanceSquared =
        minimumDistance * minimumDistance;

    if (distanceSquared >= minimumDistanceSquared) {
        return;
    }

    const float distance =
        std::sqrt(
            std::max(
                distanceSquared,
                1.0e-12f
            )
        );

    float normalX = 0.0f;
    float normalY = 0.0f;

    normalizePair(
        dx,
        dy,
        normalX,
        normalY
    );

    const float overlap =
        minimumDistance - distance;

    const float massFirst =
        std::max(
            first.mass,
            1.0e-6f
        );

    const float massSecond =
        std::max(
            second.mass,
            1.0e-6f
        );

    const float inverseMassFirst =
        1.0f / massFirst;

    const float inverseMassSecond =
        1.0f / massSecond;

    const float totalInverseMass =
        inverseMassFirst +
        inverseMassSecond;

    if (totalInverseMass > 0.0f) {
        const float correction =
            overlap / totalInverseMass;

        first.x -=
            normalX *
            correction *
            inverseMassFirst;

        first.y -=
            normalY *
            correction *
            inverseMassFirst;

        second.x +=
            normalX *
            correction *
            inverseMassSecond;

        second.y +=
            normalY *
            correction *
            inverseMassSecond;
    }

    const float relativeVelocityX =
        second.velocityX -
        first.velocityX;

    const float relativeVelocityY =
        second.velocityY -
        first.velocityY;

    const float velocityAlongNormal =
        relativeVelocityX * normalX +
        relativeVelocityY * normalY;

    if (velocityAlongNormal >= 0.0f) {
        return;
    }

    const float restitution =
        std::clamp(
            config.collisionRestitution,
            0.0f,
            1.0f
        );

    const float impulseMagnitude =
        (
            -(1.0f + restitution) *
            velocityAlongNormal
        ) /
        totalInverseMass;

    const float impulseX =
        impulseMagnitude * normalX;

    const float impulseY =
        impulseMagnitude * normalY;

    first.velocityX -=
        impulseX * inverseMassFirst;

    first.velocityY -=
        impulseY * inverseMassFirst;

    second.velocityX +=
        impulseX * inverseMassSecond;

    second.velocityY +=
        impulseY * inverseMassSecond;
}

} // namespace


void resolveCollisionsSequential(
    std::vector<Particle>& particles,
    const SimulationConfig& config
) {
    const std::size_t particleCount =
        particles.size();

    for (
        std::size_t i = 0;
        i < particleCount;
        ++i
    ) {
        for (
            std::size_t j = i + 1;
            j < particleCount;
            ++j
        ) {
            resolveParticlePair(
                particles[i],
                particles[j],
                config
            );
        }
    }
}


void resolveCollisionsParallel(
    std::vector<Particle>& particles,
    const SimulationConfig& config
) {
    const std::size_t particleCount =
        particles.size();

    if (particleCount < 2) {
        return;
    }

    using CollisionPair =
        std::pair<std::size_t, std::size_t>;

    const int threadCount =
        std::max(1, config.threadCount);

    std::vector<std::vector<CollisionPair>>
        localCollisions(
            static_cast<std::size_t>(threadCount)
        );

    // Prepara el esquema de distribución antes de crear el equipo de hilos.
    applySchedule(config);

    // Cada hilo busca posibles colisiones
    // sin modificar las particulas.
#pragma omp parallel num_threads(threadCount)
    {
        const int threadId =
            omp_get_thread_num();

        std::vector<CollisionPair>&
            collisions =
                localCollisions[
                    static_cast<std::size_t>(
                        threadId
                    )
                ];

#pragma omp for schedule(runtime)
        for (
            long long i = 0;
            i <
                static_cast<long long>(
                    particleCount
                );
            ++i
        ) {
            for (
                long long j = i + 1;
                j <
                    static_cast<long long>(
                        particleCount
                    );
                ++j
            ) {
                const std::size_t firstIndex =
                    static_cast<std::size_t>(i);

                const std::size_t secondIndex =
                    static_cast<std::size_t>(j);

                if (
                    particlesOverlap(
                        particles[firstIndex],
                        particles[secondIndex]
                    )
                ) {
                    collisions.emplace_back(
                        firstIndex,
                        secondIndex
                    );
                }
            }
        }
    }

    // Se resuelven solamente las parejas encontradas.
    for (
        const std::vector<CollisionPair>& collisions :
        localCollisions
    ) {
        for (
            const CollisionPair& collision :
            collisions
        ) {
            resolveParticlePair(
                particles[collision.first],
                particles[collision.second],
                config
            );
        }
    }
}