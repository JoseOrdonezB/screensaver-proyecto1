#include "simulation.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <omp.h>

namespace {

// Evita que el compilador elimine los calculos del benchmark.
volatile float benchmarkResult = 0.0f;

// Calcula el promedio de una lista de tiempos medidos;
// devuelve 0 si la lista esta vacia.
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

// Genera la lista de cantidades de hilos a probar: potencias
// de dos desde 1 hasta 8, sin superar el maximo disponible.
std::vector<int> createThreadList() {
    const int maximumThreads = omp_get_max_threads();

    std::vector<int> threadCounts{1};

    for (
        int threads = 2;
        threads <= maximumThreads && threads <= 8;
        threads *= 2
    ) {
        threadCounts.push_back(threads);
    }

    return threadCounts;
}

// Los esquemas de distribucion que se comparan en el benchmark.
std::vector<ScheduleKind> createScheduleList() {
    return {
        ScheduleKind::Static,
        ScheduleKind::Dynamic,
        ScheduleKind::Guided
    };
}

// Nombre legible de un esquema de distribucion para CSV y consola.
std::string scheduleName(const ScheduleKind schedule) {
    switch (schedule) {
        case ScheduleKind::Dynamic:
            return "dynamic";

        case ScheduleKind::Guided:
            return "guided";

        case ScheduleKind::Static:
        default:
            return "static";
    }
}


// Mide solamente el movimiento secuencial.
double measureMovementSequential(
    const std::vector<Particle>& initialParticles,
    const SimulationConfig& config,
    const int frames
) {
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


// Mide solamente el movimiento paralelo.
double measureMovementParallel(
    const std::vector<Particle>& initialParticles,
    const SimulationConfig& config,
    const int frames
) {
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


// Mide movimiento y colisiones secuenciales.
double measureSimulationSequential(
    const std::vector<Particle>& initialParticles,
    const SimulationConfig& config,
    const int frames
) {
    std::vector<Particle> particles = initialParticles;

    const double startTime = omp_get_wtime();

    for (int frame = 0; frame < frames; ++frame) {
        updateSequential(particles, config);
        resolveCollisionsSequential(particles, config);
    }

    const double endTime = omp_get_wtime();

    if (!particles.empty()) {
        benchmarkResult = particles.front().x;
    }

    return endTime - startTime;
}


// Mide movimiento y colisiones paralelas.
double measureSimulationParallel(
    const std::vector<Particle>& initialParticles,
    const SimulationConfig& config,
    const int frames
) {
    std::vector<Particle> particles = initialParticles;

    const double startTime = omp_get_wtime();

    for (int frame = 0; frame < frames; ++frame) {
        updateParallel(particles, config);
        resolveCollisionsParallel(particles, config);
    }

    const double endTime = omp_get_wtime();

    if (!particles.empty()) {
        benchmarkResult = particles.front().x;
    }

    return endTime - startTime;
}


// Guarda una medicion individual en el CSV.
void writeMeasurement(
    std::ofstream& outputFile,
    const std::string& benchmarkType,
    const std::size_t particleCount,
    const int frames,
    const int threadCount,
    const std::string& schedule,
    const int repetition,
    const double sequentialTime,
    const double parallelTime
) {
    const double speedup =
        sequentialTime / parallelTime;

    const double efficiency =
        speedup / static_cast<double>(threadCount);

    outputFile
        << benchmarkType << ','
        << particleCount << ','
        << frames << ','
        << threadCount << ','
        << schedule << ','
        << repetition << ','
        << sequentialTime * 1000.0 << ','
        << parallelTime * 1000.0 << ','
        << speedup << ','
        << efficiency << '\n';
}


// Muestra por consola el promedio secuencial y paralelo junto
// con el speedup y la eficiencia calculados para una configuracion.
void printAverage(
    const int threadCount,
    const std::string& schedule,
    const double sequentialAverage,
    const double parallelAverage
) {
    const double speedup =
        sequentialAverage / parallelAverage;

    const double efficiency =
        speedup / static_cast<double>(threadCount);

    std::cout
        << "Hilos: " << threadCount
        << " [" << schedule << ']'
        << " | Secuencial: "
        << sequentialAverage * 1000.0 << " ms"
        << " | Paralelo: "
        << parallelAverage * 1000.0 << " ms"
        << " | Speedup: "
        << speedup
        << " | Eficiencia: "
        << efficiency * 100.0 << "%\n";
}


// Benchmark exclusivo del movimiento.
void runMovementBenchmark(
    std::ofstream& outputFile,
    const std::vector<int>& threadCounts
) {
    const std::vector<std::size_t> particleCounts{
        1000,
        10000,
        100000
    };

    constexpr int repetitions = 10;
    constexpr int frames = 200;

    std::cout
        << "Benchmark de movimiento\n"
        << "Frames: " << frames << '\n'
        << "Repeticiones: " << repetitions
        << "\n\n";

    for (const std::size_t particleCount : particleCounts) {
        SimulationConfig config;

        config.particleCount = particleCount;
        config.randomSeed = 12345;

        std::vector<Particle> initialParticles;
        initializeParticles(initialParticles, config);

        std::cout
            << "Particulas: "
            << particleCount
            << '\n';

        std::vector<double> sequentialTimes;
        sequentialTimes.reserve(repetitions);

        for (
            int repetition = 0;
            repetition < repetitions;
            ++repetition
        ) {
            sequentialTimes.push_back(
                measureMovementSequential(
                    initialParticles,
                    config,
                    frames
                )
            );
        }

        const double sequentialAverage =
            calculateAverage(sequentialTimes);

        for (const ScheduleKind schedule : createScheduleList()) {
            const std::string scheduleLabel =
                scheduleName(schedule);

            for (const int threadCount : threadCounts) {
                config.threadCount = threadCount;
                config.scheduleKind = schedule;

                std::vector<double> parallelTimes;
                parallelTimes.reserve(repetitions);

                for (
                    int repetition = 0;
                    repetition < repetitions;
                    ++repetition
                ) {
                    const double parallelTime =
                        measureMovementParallel(
                            initialParticles,
                            config,
                            frames
                        );

                    parallelTimes.push_back(parallelTime);

                    writeMeasurement(
                        outputFile,
                        "movement",
                        particleCount,
                        frames,
                        threadCount,
                        scheduleLabel,
                        repetition + 1,
                        sequentialTimes[
                            static_cast<std::size_t>(repetition)
                        ],
                        parallelTime
                    );
                }

                const double parallelAverage =
                    calculateAverage(parallelTimes);

                printAverage(
                    threadCount,
                    scheduleLabel,
                    sequentialAverage,
                    parallelAverage
                );
            }
        }

        std::cout << '\n';
    }
}


// Benchmark del movimiento junto con las colisiones.
void runSimulationBenchmark(
    std::ofstream& outputFile,
    const std::vector<int>& threadCounts
) {
    // Se usan menos particulas porque las colisiones son O(N^2).
    const std::vector<std::size_t> particleCounts{
        250,
        500,
        1000
    };

    constexpr int repetitions = 10;
    constexpr int frames = 50;

    std::cout
        << "Benchmark de simulacion completa\n"
        << "Movimiento + colisiones\n"
        << "Frames: " << frames << '\n'
        << "Repeticiones: " << repetitions
        << "\n\n";

    for (const std::size_t particleCount : particleCounts) {
        SimulationConfig config;

        config.particleCount = particleCount;
        config.randomSeed = 12345;

        std::vector<Particle> initialParticles;
        initializeParticles(initialParticles, config);

        std::cout
            << "Particulas: "
            << particleCount
            << '\n';

        std::vector<double> sequentialTimes;
        sequentialTimes.reserve(repetitions);

        for (
            int repetition = 0;
            repetition < repetitions;
            ++repetition
        ) {
            sequentialTimes.push_back(
                measureSimulationSequential(
                    initialParticles,
                    config,
                    frames
                )
            );
        }

        const double sequentialAverage =
            calculateAverage(sequentialTimes);

        for (const ScheduleKind schedule : createScheduleList()) {
            const std::string scheduleLabel =
                scheduleName(schedule);

            for (const int threadCount : threadCounts) {
                config.threadCount = threadCount;
                config.scheduleKind = schedule;

                std::vector<double> parallelTimes;
                parallelTimes.reserve(repetitions);

                for (
                    int repetition = 0;
                    repetition < repetitions;
                    ++repetition
                ) {
                    const double parallelTime =
                        measureSimulationParallel(
                            initialParticles,
                            config,
                            frames
                        );

                    parallelTimes.push_back(parallelTime);

                    writeMeasurement(
                        outputFile,
                        "simulation",
                        particleCount,
                        frames,
                        threadCount,
                        scheduleLabel,
                        repetition + 1,
                        sequentialTimes[
                            static_cast<std::size_t>(repetition)
                        ],
                        parallelTime
                    );
                }

                const double parallelAverage =
                    calculateAverage(parallelTimes);

                printAverage(
                    threadCount,
                    scheduleLabel,
                    sequentialAverage,
                    parallelAverage
                );
            }
        }

        std::cout << '\n';
    }
}

} // namespace


int main() {
    omp_set_dynamic(0);

    const std::vector<int> threadCounts =
        createThreadList();

    std::ofstream outputFile(
        "resultados_benchmark.csv"
    );

    if (!outputFile.is_open()) {
        std::cerr
            << "Error: no se pudo crear "
            << "resultados_benchmark.csv\n";

        return 1;
    }

    outputFile
        << "benchmark,"
        << "particles,"
        << "frames,"
        << "threads,"
        << "schedule,"
        << "repetition,"
        << "sequential_ms,"
        << "parallel_ms,"
        << "speedup,"
        << "efficiency\n";

    std::cout
        << std::fixed
        << std::setprecision(3);

    std::cout
        << "Benchmark del screensaver\n\n";

    runMovementBenchmark(
        outputFile,
        threadCounts
    );

    runSimulationBenchmark(
        outputFile,
        threadCounts
    );

    outputFile.close();

    std::cout
        << "Benchmark finalizado.\n"
        << "Resultados guardados en "
        << "resultados_benchmark.csv\n";

    return 0;
}