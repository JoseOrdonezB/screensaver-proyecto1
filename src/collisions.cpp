#include "collisions.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// Normaliza un vector para evitar divisiones por cero.
void normalizePair(
    const float dx,
    const float dy,
    float& normalX,
    float& normalY
) {
    const float distanceSquared = dx * dx + dy * dy;

    if (distanceSquared <= 1.0e-12f) {
        normalX = 1.0f;
        normalY = 0.0f;
        return;
    }

    const float distance = std::sqrt(distanceSquared);
    normalX = dx / distance;
    normalY = dy / distance;
}

// Resuelve la intersección de una pareja de partículas.
void resolveParticlePair(
    Particle& first,
    Particle& second,
    const SimulationConfig& config
) {
    const float dx = second.x - first.x;
    const float dy = second.y - first.y;
    const float distanceSquared = dx * dx + dy * dy;
    const float minimumDistance = first.radius + second.radius;
    const float minimumDistanceSquared = minimumDistance * minimumDistance;

    if (distanceSquared >= minimumDistanceSquared) {
        return;
    }

    const float distance = std::sqrt(std::max(distanceSquared, 1.0e-12f));
    float normalX = 0.0f;
    float normalY = 0.0f;
    normalizePair(dx, dy, normalX, normalY);

    const float overlap = minimumDistance - distance;
    const float massFirst = std::max(first.mass, 1.0e-6f);
    const float massSecond = std::max(second.mass, 1.0e-6f);
    const float inverseMassFirst = 1.0f / massFirst;
    const float inverseMassSecond = 1.0f / massSecond;
    const float totalInverseMass = inverseMassFirst + inverseMassSecond;

    if (totalInverseMass > 0.0f) {
        const float correction = overlap / totalInverseMass;

        first.x -= normalX * correction * inverseMassFirst;
        first.y -= normalY * correction * inverseMassFirst;
        second.x += normalX * correction * inverseMassSecond;
        second.y += normalY * correction * inverseMassSecond;
    }

    const float relativeVelocityX = second.velocityX - first.velocityX;
    const float relativeVelocityY = second.velocityY - first.velocityY;
    const float velocityAlongNormal =
        relativeVelocityX * normalX +
        relativeVelocityY * normalY;

    if (velocityAlongNormal >= 0.0f) {
        return;
    }

    const float restitution = std::clamp(
        config.collisionRestitution,
        0.0f,
        1.0f
    );
    const float impulseMagnitude =
        (-(1.0f + restitution) * velocityAlongNormal) /
        totalInverseMass;

    const float impulseX = impulseMagnitude * normalX;
    const float impulseY = impulseMagnitude * normalY;

    first.velocityX -= impulseX * inverseMassFirst;
    first.velocityY -= impulseY * inverseMassFirst;
    second.velocityX += impulseX * inverseMassSecond;
    second.velocityY += impulseY * inverseMassSecond;
}

}  // namespace

void resolveCollisionsSequential(
    std::vector<Particle>& particles,
    const SimulationConfig& config
) {
    const std::size_t particleCount = particles.size();

    for (std::size_t i = 0; i < particleCount; ++i) {
        for (std::size_t j = i + 1; j < particleCount; ++j) {
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
    const std::size_t particleCount = particles.size();

    if (particleCount < 2) {
        return;
    }

#pragma omp parallel for schedule(static) num_threads(config.threadCount)
    for (long long i = 0; i < static_cast<long long>(particleCount); ++i) {
        for (long long j = i + 1; j < static_cast<long long>(particleCount); ++j) {
#pragma omp critical
            {
                resolveParticlePair(
                    particles[static_cast<std::size_t>(i)],
                    particles[static_cast<std::size_t>(j)],
                    config
                );
            }
        }
    }
}
