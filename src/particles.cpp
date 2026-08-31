#include "simulation.h"

#include <cmath>
#include <random>
#include <stdexcept>

namespace {

// Valor de PI para calcular la dirección y la masa.
constexpr float PI = 3.14159265358979323846f;

// Verifica que la configuración permita crear partículas válidas.
void validateConfiguration(const SimulationConfig& config) {
    if (config.width <= 0 || config.height <= 0) {
        throw std::invalid_argument(
            "El ancho y el alto del canvas deben ser mayores que cero."
        );
    }

    if (config.minimumRadius <= 0.0f) {
        throw std::invalid_argument(
            "El radio minimo debe ser mayor que cero."
        );
    }

    if (config.maximumRadius < config.minimumRadius) {
        throw std::invalid_argument(
            "El radio maximo no puede ser menor que el radio minimo."
        );
    }

    if (config.minimumSpeed < 0.0f) {
        throw std::invalid_argument(
            "La velocidad minima no puede ser negativa."
        );
    }

    if (config.maximumSpeed < config.minimumSpeed) {
        throw std::invalid_argument(
            "La velocidad maxima no puede ser menor que la minima."
        );
    }

    if (config.maximumRadius * 2.0f >
            static_cast<float>(config.width) ||
        config.maximumRadius * 2.0f >
            static_cast<float>(config.height)) {
        throw std::invalid_argument(
            "El canvas es demasiado pequeno para el radio configurado."
        );
    }

    if (config.threadCount <= 0) {
        throw std::invalid_argument(
            "La cantidad de hilos debe ser mayor que cero."
        );
    }
}

}  // namespace

void initializeParticles(
    std::vector<Particle>& particles,
    const SimulationConfig& config
) {
    validateConfiguration(config);

    // Elimina las partículas anteriores.
    particles.clear();

    // Reserva la memoria necesaria desde el principio.
    particles.reserve(config.particleCount);

    // Generador pseudoaleatorio con semilla fija.
    std::mt19937 generator(config.randomSeed);

    std::uniform_real_distribution<float> radiusDistribution(
        config.minimumRadius,
        config.maximumRadius
    );

    std::uniform_real_distribution<float> speedDistribution(
        config.minimumSpeed,
        config.maximumSpeed
    );

    std::uniform_real_distribution<float> angleDistribution(
        0.0f,
        2.0f * PI
    );

    std::uniform_int_distribution<int> colorDistribution(0, 255);

    for (std::size_t i = 0; i < config.particleCount; ++i) {
        Particle particle{};

        // Se genera primero el radio para calcular una posición válida.
        particle.radius = radiusDistribution(generator);

        // Se considera la partícula como un círculo de densidad uniforme.
        particle.mass = PI * particle.radius * particle.radius;

        // Evita que la partícula aparezca fuera del canvas.
        std::uniform_real_distribution<float> xDistribution(
            particle.radius,
            static_cast<float>(config.width) - particle.radius
        );

        std::uniform_real_distribution<float> yDistribution(
            particle.radius,
            static_cast<float>(config.height) - particle.radius
        );

        particle.x = xDistribution(generator);
        particle.y = yDistribution(generator);

        // Se genera una dirección y una rapidez inicial.
        const float angle = angleDistribution(generator);
        const float speed = speedDistribution(generator);

        particle.velocityX = std::cos(angle) * speed;
        particle.velocityY = std::sin(angle) * speed;

        // Se genera un color pseudoaleatorio.
        particle.red = static_cast<std::uint8_t>(
            colorDistribution(generator)
        );

        particle.green = static_cast<std::uint8_t>(
            colorDistribution(generator)
        );

        particle.blue = static_cast<std::uint8_t>(
            colorDistribution(generator)
        );

        particle.alpha = 255;

        particles.push_back(particle);
    }
}