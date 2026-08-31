#pragma once

#include "simulation.h"

#include <vector>

// Resuelve colisiones entre partículas de forma secuencial.
void resolveCollisionsSequential(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);

// Resuelve colisiones entre partículas usando OpenMP.
void resolveCollisionsParallel(
    std::vector<Particle>& particles,
    const SimulationConfig& config
);