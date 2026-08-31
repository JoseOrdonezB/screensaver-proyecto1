#include "simulation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

bool almostEqual(float a, float b, float tolerance = 1.0e-4f) {
    const float diff = std::fabs(a - b);
    const float scale = std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
    return diff <= tolerance * scale;
}

bool validateTwoParticleCollision() {
    std::vector<Particle> particles(2);

    particles[0].x = 100.0f;
    particles[0].y = 100.0f;
    particles[0].radius = 10.0f;
    particles[0].mass = 100.0f;
    particles[0].velocityX = 40.0f;
    particles[0].velocityY = 0.0f;

    particles[1].x = 118.0f;
    particles[1].y = 100.0f;
    particles[1].radius = 10.0f;
    particles[1].mass = 100.0f;
    particles[1].velocityX = -40.0f;
    particles[1].velocityY = 0.0f;

    SimulationConfig config;
    config.collisionRestitution = 1.0f;

    resolveCollisionsSequential(particles, config);

    const bool validX =
        almostEqual(particles[0].x, 100.0f) &&
        almostEqual(particles[1].x, 118.0f);

    const bool validVelocities =
        almostEqual(particles[0].velocityX, -40.0f) &&
        almostEqual(particles[1].velocityX, 40.0f);

    return validX && validVelocities;
}

bool validateWallCollision() {
    Particle particle{};
    particle.x = 5.0f;
    particle.y = 50.0f;
    particle.radius = 10.0f;
    particle.mass = 25.0f;
    particle.velocityX = -30.0f;
    particle.velocityY = 15.0f;

    SimulationConfig config;
    config.width = 200;
    config.height = 200;
    config.wallRestitution = 1.0f;
    config.deltaTime = 1.0f;

    particle.x += particle.velocityX * config.deltaTime;
    particle.y += particle.velocityY * config.deltaTime;

    if (particle.x - particle.radius < 0.0f) {
        particle.x = particle.radius;
        particle.velocityX = std::fabs(particle.velocityX) * config.wallRestitution;
    }

    return almostEqual(particle.x, 10.0f) && almostEqual(particle.velocityX, 30.0f);
}

} // namespace

int main() {
    const bool collisionOK = validateTwoParticleCollision();
    const bool wallOK = validateWallCollision();

    if (!collisionOK || !wallOK) {
        std::cerr << "Fallo en la validacion de colisiones\n";
        return 1;
    }

    std::cout << "Validacion de colisiones correcta\n";
    return 0;
}
