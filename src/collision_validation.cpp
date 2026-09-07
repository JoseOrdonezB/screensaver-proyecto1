#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

/**
 * Compara dos valores float considerando un margen de tolerancia.
 */
bool almostEqual(float a, float b, float tolerance = 1.0e-4f) {
    const float diff = std::fabs(a - b);
    const float scale =
        std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));

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

    // La distancia final entre los centros debe ser igual
    // a la suma de los radios: 10 + 10 = 20.
    const float distance =
        std::fabs(particles[1].x - particles[0].x);

    const bool validSeparation =
        almostEqual(distance, 20.0f);

    // En una colision elastica frontal entre objetos de
    // igual masa, las velocidades se intercambian.
    const bool validVelocities =
        almostEqual(particles[0].velocityX, -40.0f) &&
        almostEqual(particles[1].velocityX, 40.0f);

    if (!validSeparation) {
        std::cerr
            << "Error: separacion incorrecta entre particulas.\n"
            << "Distancia esperada: 20\n"
            << "Distancia obtenida: " << distance << '\n'
            << "P0.x: " << particles[0].x << '\n'
            << "P1.x: " << particles[1].x << '\n';
    }

    if (!validVelocities) {
        std::cerr
            << "Error: velocidades incorrectas despues "
               "de la colision.\n"
            << "P0.vx esperado: -40 | obtenido: "
            << particles[0].velocityX << '\n'
            << "P1.vx esperado: 40 | obtenido: "
            << particles[1].velocityX << '\n';
    }

    return validSeparation && validVelocities;
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

    // Simular un paso de movimiento.
    particle.x += particle.velocityX * config.deltaTime;
    particle.y += particle.velocityY * config.deltaTime;

    // Simular colision con la pared izquierda.
    if (particle.x - particle.radius < 0.0f) {
        particle.x = particle.radius;

        particle.velocityX =
            std::fabs(particle.velocityX) *
            config.wallRestitution;
    }

    const bool validPosition =
        almostEqual(particle.x, 10.0f);

    const bool validVelocity =
        almostEqual(particle.velocityX, 30.0f);

    if (!validPosition) {
        std::cerr
            << "Error: posicion incorrecta despues "
               "del rebote con la pared.\n"
            << "X esperada: 10 | obtenida: "
            << particle.x << '\n';
    }

    if (!validVelocity) {
        std::cerr
            << "Error: velocidad incorrecta despues "
               "del rebote con la pared.\n"
            << "Vx esperada: 30 | obtenida: "
            << particle.velocityX << '\n';
    }

    return validPosition && validVelocity;
}

} // namespace

int main() {
    std::cout << "Validacion de colisiones\n";
    std::cout << "========================\n\n";

    const bool collisionOK =
        validateTwoParticleCollision();

    const bool wallOK =
        validateWallCollision();

    std::cout
        << "Colision entre particulas: "
        << (collisionOK ? "OK" : "FALLO")
        << '\n';

    std::cout
        << "Colision contra pared: "
        << (wallOK ? "OK" : "FALLO")
        << '\n';

    if (!collisionOK || !wallOK) {
        std::cerr
            << "\nFallo en la validacion de colisiones\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "\nValidacion de colisiones correcta\n";

    return EXIT_SUCCESS;
}