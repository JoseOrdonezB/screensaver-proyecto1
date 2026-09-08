#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Representa una partícula dentro de la simulación.
struct Particle {
    // Posición actual.
    float x;
    float y;

    // Velocidad en cada eje.
    float velocityX;
    float velocityY;

    // Propiedades físicas.
    float radius;
    float mass;

    // Color de la partícula.
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

// Esquema de distribución de iteraciones de OpenMP.
enum class ScheduleKind {
    // Bloques contiguos predefinidos (comportamiento base).
    Static,

    // Asignación dinámica por trozos pequeños mientras hay trabajo.
    Dynamic,

    // Trozos decrecientes: balanceo con menos overhead que dynamic.
    Guided
};

// Contiene los parámetros generales de la simulación.
struct SimulationConfig {
    // Dimensiones del canvas.
    int width = 800;
    int height = 600;

    // Cantidad de partículas.
    std::size_t particleCount = 1000;

    // Tiempo simulado entre frames.
    float deltaTime = 1.0f / 60.0f;

    // Límites de velocidad inicial.
    float minimumSpeed = 50.0f;
    float maximumSpeed = 150.0f;

    // Límites del radio de las partículas.
    float minimumRadius = 4.0f;
    float maximumRadius = 10.0f;

    // Conservación de velocidad en los rebotes.
    // 1.0 significa un rebote completamente elástico.
    float wallRestitution = 1.0f;

    // Repuesta de las colisiones entre partículas.
    // 1.0 significa colisión totalmente elástica.
    float collisionRestitution = 0.95f;

    // Semilla para generar siempre los mismos datos.
    unsigned int randomSeed = 12345;

    // Cantidad de hilos para OpenMP.
    int threadCount = 1;

    // Esquema de distribución usado por las rutinas paralelas.
    ScheduleKind scheduleKind = ScheduleKind::Static;
};

// Crea e inicializa todas las partículas.
void initializeParticles(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);

// Actualiza el movimiento de forma secuencial.
void updateSequential(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);

// Actualiza el movimiento utilizando OpenMP.
void updateParallel(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);

// Resuelve colisiones entre partículas de forma secuencial.
void resolveCollisionsSequential(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);

// Resuelve colisiones entre partículas utilizando OpenMP.
void resolveCollisionsParallel(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);