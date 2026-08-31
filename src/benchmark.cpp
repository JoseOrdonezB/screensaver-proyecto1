#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <omp.h>

namespace {

// Evita que el compilador elimine los cálculos del benchmark.
volatile float benchmarkResult = 0.0f;

double calculateAverage(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }

    const double total = std::accumulate(
        values.begin(),
        values.end(),
        0.0
    );

    return total / static_cast<double>(values.size());
}

double measureSequential(
    const std::vector<Particle>& initialParticles,
    const SimulationConfig& config,
    int frames
) {
    // La copia se realiza antes de comenzar a medir.
    std::vector<Particle> particles = initialParticles;

    const double startTime = omp_get_wtime();

    for (int frame = 0; frame < frames; ++frame) {
        updateSequential(particles, config);
    }

    const double endTime = omp_get_wtime();

    if (!particles.empty()) {
        benchmarkResult = particles.front().x;
    }

    return endTime - startTime;
}

double measureParallel(
    const std::vector<Particle>& initialParticles,
    const SimulationConfig& config,
    int frames
) {
    // La copia se realiza antes de comenzar a medir.
    std::vector<Particle> particles = initialParticles;

    const double startTime = omp_get_wtime();

    for (int frame = 0; frame < frames; ++frame) {
        updateParallel(particles, config);
    }

    const double endTime = omp_get_wtime();

    if (!particles.empty()) {
        benchmarkResult = particles.front().x;
    }

    return endTime - startTime;
}

bool almostEqual(float first, float second) {
    constexpr float tolerance = 0.0001f;

    const float difference = std::abs(first - second);

    const float largestValue = std::max(
        1.0f,
        std::max(std::abs(first), std::abs(second))
    );

    return difference <= tolerance * largestValue;
}

bool compareParticles(
    const std::vector<Particle>& sequentialParticles,
    const std::vector<Particle>& parallelParticles
) {
    if (sequentialParticles.size() != parallelParticles.size()) {
        return false;
    }

    for (std::size_t i = 0;
         i < sequentialParticles.size();
         ++i) {
        const Particle& sequential = sequentialParticles[i];
        const Particle& parallel = parallelParticles[i];

        if (!almostEqual(sequential.x, parallel.x) ||
            !almostEqual(sequential.y, parallel.y) ||
            !almostEqual(
                sequential.velocityX,
                parallel.velocityX
            ) ||
            !almostEqual(
                sequential.velocityY,
                parallel.velocityY
            )) {
            std::cerr
                << "Diferencia encontrada en la particula "
                << i
                << '\n';

            return false;
        }
    }

    return true;
}

bool verifyResults(
    const std::vector<Particle>& initialParticles,
    SimulationConfig config
) {
    std::vector<Particle> sequentialParticles = initialParticles;
    std::vector<Particle> parallelParticles = initialParticles;

    updateSequential(sequentialParticles, config);
    updateParallel(parallelParticles, config);

    return compareParticles(
        sequentialParticles,
        parallelParticles
    );
}

std::vector<int> createThreadList() {
    const int maximumThreads = omp_get_max_threads();

    std::vector<int> threadCounts{1};

    for (int threads = 2;
         threads <= maximumThreads && threads <= 8;
         threads *= 2) {
        threadCounts.push_back(threads);
    }

    return threadCounts;
}

}  // namespace

int main() {
    // Evita que OpenMP cambie dinámicamente la cantidad de hilos.
    omp_set_dynamic(0);

    const std::vector<std::size_t> particleCounts{
        1000,
        10000,
        100000
    };

    const std::vector<int> threadCounts = createThreadList();

    constexpr int repetitions = 10;
    constexpr int frames = 200;

    std::ofstream outputFile("resultados_benchmark.csv");

    if (!outputFile.is_open()) {
        std::cerr
            << "Error: no se pudo crear resultados_benchmark.csv\n";

        return 1;
    }

    outputFile
        << "particles,"
        << "frames,"
        << "threads,"
        << "repetition,"
        << "sequential_ms,"
        << "parallel_ms,"
        << "speedup,"
        << "efficiency\n";

    std::cout << std::fixed << std::setprecision(3);

    std::cout
        << "Benchmark de movimiento de particulas\n"
        << "Frames por medicion: " << frames << '\n'
        << "Repeticiones: " << repetitions << "\n\n";

    for (const std::size_t particleCount : particleCounts) {
        SimulationConfig config;

        config.particleCount = particleCount;
        config.randomSeed = 12345;

        std::vector<Particle> initialParticles;

        initializeParticles(initialParticles, config);

        config.threadCount = 1;

        if (!verifyResults(initialParticles, config)) {
            std::cerr
                << "Error: la version secuencial y paralela "
                << "producen resultados diferentes.\n";

            return 1;
        }

        std::cout
            << "Particulas: "
            << particleCount
            << '\n';

        std::vector<double> sequentialTimes;
        sequentialTimes.reserve(repetitions);

        // La versión secuencial se mide una vez por repetición.
        for (int repetition = 0;
             repetition < repetitions;
             ++repetition) {
            sequentialTimes.push_back(
                measureSequential(
                    initialParticles,
                    config,
                    frames
                )
            );
        }

        const double sequentialAverage =
            calculateAverage(sequentialTimes);

        for (const int threadCount : threadCounts) {
            config.threadCount = threadCount;

            std::vector<double> parallelTimes;
            parallelTimes.reserve(repetitions);

            for (int repetition = 0;
                 repetition < repetitions;
                 ++repetition) {
                const double parallelTime =
                    measureParallel(
                        initialParticles,
                        config,
                        frames
                    );

                parallelTimes.push_back(parallelTime);

                const double sequentialTime =
                    sequentialTimes[
                        static_cast<std::size_t>(repetition)
                    ];

                const double speedup =
                    sequentialTime / parallelTime;

                const double efficiency =
                    speedup /
                    static_cast<double>(threadCount);

                outputFile
                    << particleCount << ','
                    << frames << ','
                    << threadCount << ','
                    << repetition + 1 << ','
                    << sequentialTime * 1000.0 << ','
                    << parallelTime * 1000.0 << ','
                    << speedup << ','
                    << efficiency << '\n';
            }

            const double parallelAverage =
                calculateAverage(parallelTimes);

            const double speedup =
                sequentialAverage / parallelAverage;

            const double efficiency =
                speedup /
                static_cast<double>(threadCount);

            std::cout
                << "  Hilos: " << threadCount
                << " | Secuencial: "
                << sequentialAverage * 1000.0 << " ms"
                << " | Paralelo: "
                << parallelAverage * 1000.0 << " ms"
                << " | Speedup: "
                << speedup
                << " | Eficiencia: "
                << efficiency * 100.0 << "%\n";
        }

        std::cout << '\n';
    }

    outputFile.close();

    std::cout
        << "Benchmark finalizado.\n"
        << "Resultados guardados en resultados_benchmark.csv\n";

    return 0;
}