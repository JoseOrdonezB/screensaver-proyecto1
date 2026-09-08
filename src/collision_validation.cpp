#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

// Compara dos valores de punto flotante con una tolerancia relativa
// al mayor de ambos, evitando falsos negativos por error de precision.
bool almostEqual(
    float a,
    float b,
    float tolerance = 1.0e-4f
) {
    const float diff = std::fabs(a - b);

    const float scale =
        std::max(
            1.0f,
            std::max(std::fabs(a), std::fabs(b))
        );

    return diff <= tolerance * scale;
}


// ============================================================
// ESCENARIO SIMPLE
// ============================================================

// Crea dos particulas enfrentadas con velocidades opuestas e igual
// masa para probar el intercambio de velocidades en una colision.
std::vector<Particle> createCollisionScenario() {
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

    return particles;
}


// Verifica que tras la colision las particulas queden separadas por
// la distancia esperada y hayan intercambiado sus velocidades.
bool validateCollisionResult(
    const std::vector<Particle>& particles,
    const std::string& testName
) {
    const float dx =
        particles[1].x - particles[0].x;

    const float dy =
        particles[1].y - particles[0].y;

    const float distance =
        std::sqrt(dx * dx + dy * dy);

    const bool validSeparation =
        almostEqual(distance, 20.0f);

    const bool validVelocities =
        almostEqual(
            particles[0].velocityX,
            -40.0f
        ) &&
        almostEqual(
            particles[1].velocityX,
            40.0f
        );

    if (!validSeparation) {
        std::cerr
            << "[" << testName << "] "
            << "Separacion incorrecta.\n"
            << "  Esperada: 20\n"
            << "  Obtenida: " << distance
            << '\n';
    }

    if (!validVelocities) {
        std::cerr
            << "[" << testName << "] "
            << "Velocidades incorrectas.\n"
            << "  P0.vx: "
            << particles[0].velocityX
            << '\n'
            << "  P1.vx: "
            << particles[1].velocityX
            << '\n';
    }

    return validSeparation &&
           validVelocities;
}


// Ejecuta el escenario de colision con la resolucion secuencial
// y comprueba que se obtenga el resultado esperado.
bool validateSequentialCollision() {
    std::vector<Particle> particles =
        createCollisionScenario();

    SimulationConfig config;
    config.collisionRestitution = 1.0f;

    resolveCollisionsSequential(
        particles,
        config
    );

    return validateCollisionResult(
        particles,
        "Secuencial"
    );
}


// Ejecuta el escenario de colision con la resolucion paralela usando
// una cantidad dada de hilos y comprueba el resultado esperado.
bool validateParallelCollision(
    const int threadCount
) {
    std::vector<Particle> particles =
        createCollisionScenario();

    SimulationConfig config;

    config.collisionRestitution = 1.0f;
    config.threadCount = threadCount;

    resolveCollisionsParallel(
        particles,
        config
    );

    return validateCollisionResult(
        particles,
        "Paralelo " +
        std::to_string(threadCount) +
        " hilos"
    );
}


// ============================================================
// COLISION CONTRA PARED
// ============================================================

// Avanza una particula contra el borde izquierdo del canvas y
// verifica que su posicion se corrija y su velocidad se invierta.
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

    particle.x +=
        particle.velocityX *
        config.deltaTime;

    particle.y +=
        particle.velocityY *
        config.deltaTime;

    if (
        particle.x - particle.radius <
        0.0f
    ) {
        particle.x =
            particle.radius;

        particle.velocityX =
            std::fabs(
                particle.velocityX
            ) *
            config.wallRestitution;
    }

    const bool validPosition =
        almostEqual(
            particle.x,
            10.0f
        );

    const bool validVelocity =
        almostEqual(
            particle.velocityX,
            30.0f
        );

    return validPosition &&
           validVelocity;
}


// ============================================================
// STRESS TEST
// ============================================================

// Crea un conjunto determinista de particulas.
// Se usa siempre la misma semilla para poder repetir
// exactamente las mismas condiciones.
std::vector<Particle> createStressScenario(
    const std::size_t particleCount
) {
    std::vector<Particle> particles(
        particleCount
    );

    std::mt19937 generator(12345);

    std::uniform_real_distribution<float>
        positionX(50.0f, 750.0f);

    std::uniform_real_distribution<float>
        positionY(50.0f, 550.0f);

    std::uniform_real_distribution<float>
        velocity(-100.0f, 100.0f);

    std::uniform_real_distribution<float>
        radius(4.0f, 12.0f);

    for (Particle& particle : particles) {
        particle.x =
            positionX(generator);

        particle.y =
            positionY(generator);

        particle.radius =
            radius(generator);

        particle.mass =
            particle.radius *
            particle.radius;

        particle.velocityX =
            velocity(generator);

        particle.velocityY =
            velocity(generator);
    }

    return particles;
}


// Comprueba que ninguna particula termine con valores
// NaN, infinitos, radios invalidos o masas invalidas.
bool validateParticleState(
    const std::vector<Particle>& particles
) {
    for (
        std::size_t i = 0;
        i < particles.size();
        ++i
    ) {
        const Particle& particle =
            particles[i];

        const bool finite =
            std::isfinite(particle.x) &&
            std::isfinite(particle.y) &&
            std::isfinite(
                particle.velocityX
            ) &&
            std::isfinite(
                particle.velocityY
            ) &&
            std::isfinite(
                particle.radius
            ) &&
            std::isfinite(
                particle.mass
            );

        const bool validPhysicalValues =
            particle.radius > 0.0f &&
            particle.mass > 0.0f;

        if (
            !finite ||
            !validPhysicalValues
        ) {
            std::cerr
                << "Particula invalida "
                << i
                << '\n';

            return false;
        }
    }

    return true;
}


// Ejecuta repetidamente la resolucion paralela de
// colisiones para aumentar la probabilidad de detectar
// errores de concurrencia o estados numericos invalidos.
bool runParallelStressTest(
    const int threadCount,
    const int repetitions,
    const std::size_t particleCount
) {
    SimulationConfig config;

    config.width = 800;
    config.height = 600;
    config.collisionRestitution = 1.0f;
    config.threadCount = threadCount;

    for (
        int repetition = 0;
        repetition < repetitions;
        ++repetition
    ) {
        std::vector<Particle> particles =
            createStressScenario(
                particleCount
            );

        resolveCollisionsParallel(
            particles,
            config
        );

        if (!validateParticleState(particles)) {
            std::cerr
                << "Stress test fallo con "
                << threadCount
                << " hilos en repeticion "
                << repetition + 1
                << ".\n";

            return false;
        }
    }

    return true;
}

} // namespace


int main() {
    std::cout
        << "Validacion de colisiones\n"
        << "========================\n\n";

    bool allTestsPassed = true;


    // ========================================================
    // PRUEBA SECUENCIAL
    // ========================================================

    const bool sequentialOK =
        validateSequentialCollision();

    std::cout
        << "Secuencial: "
        << (sequentialOK ? "OK" : "FALLO")
        << '\n';

    allTestsPassed =
        allTestsPassed &&
        sequentialOK;


    // ========================================================
    // PRUEBAS PARALELAS SIMPLES
    // ========================================================

    std::cout
        << "\nParalelo:\n";

    const int threadCounts[] = {
        1,
        2,
        4,
        8
    };

    for (
        const int threadCount :
        threadCounts
    ) {
        const bool parallelOK =
            validateParallelCollision(
                threadCount
            );

        std::cout
            << "  "
            << threadCount
            << (
                threadCount == 1
                    ? " hilo:  "
                    : " hilos: "
            )
            << (
                parallelOK
                    ? "OK"
                    : "FALLO"
            )
            << '\n';

        allTestsPassed =
            allTestsPassed &&
            parallelOK;
    }


    // ========================================================
    // PARED
    // ========================================================

    const bool wallOK =
        validateWallCollision();

    std::cout
        << "\nColision contra pared: "
        << (
            wallOK
                ? "OK"
                : "FALLO"
        )
        << '\n';

    allTestsPassed =
        allTestsPassed &&
        wallOK;


    // ========================================================
    // STRESS TEST
    // ========================================================

    constexpr int STRESS_REPETITIONS = 20;

    constexpr std::size_t
        STRESS_PARTICLES = 500;

    std::cout
        << "\nStress test paralelo\n"
        << "---------------------\n"
        << "Particulas por prueba: "
        << STRESS_PARTICLES
        << '\n'
        << "Repeticiones: "
        << STRESS_REPETITIONS
        << "\n\n";

    for (
        const int threadCount :
        threadCounts
    ) {
        const bool stressOK =
            runParallelStressTest(
                threadCount,
                STRESS_REPETITIONS,
                STRESS_PARTICLES
            );

        std::cout
            << "  "
            << threadCount
            << (
                threadCount == 1
                    ? " hilo:  "
                    : " hilos: "
            )
            << (
                stressOK
                    ? "OK"
                    : "FALLO"
            )
            << '\n';

        allTestsPassed =
            allTestsPassed &&
            stressOK;
    }


    // ========================================================
    // RESULTADO
    // ========================================================

    if (!allTestsPassed) {
        std::cerr
            << "\nFallo en la validacion "
            << "de colisiones.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "\nTodas las validaciones "
        << "fueron correctas.\n";

    return EXIT_SUCCESS;
}