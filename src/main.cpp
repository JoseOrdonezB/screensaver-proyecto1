#include "renderer.h"
#include "simulation.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

namespace {

constexpr int MINIMUM_WIDTH = 640;
constexpr int MINIMUM_HEIGHT = 480;

constexpr int DEFAULT_WIDTH = 800;
constexpr int DEFAULT_HEIGHT = 600;
constexpr int DEFAULT_MAX_FPS = 60;
constexpr unsigned int DEFAULT_SEED = 12345;

constexpr int MAXIMUM_CANVAS_SIZE = 8192;
constexpr int MAXIMUM_FPS = 1000;

struct ProgramArguments {
    std::size_t particleCount = 0;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    unsigned int seed = DEFAULT_SEED;
    int maxFps = DEFAULT_MAX_FPS;
    bool helpRequested = false;
};

struct ParsedNumber {
    bool valid = false;
    long long value = 0;
};

ParsedNumber parseInteger(const char* text) {
    ParsedNumber result;

    if (text == nullptr || text[0] == '\0') {
        return result;
    }

    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0') {
        return result;
    }

    result.valid = true;
    result.value = value;

    return result;
}

void printUsage(const char* programName) {
    std::cout
        << "Uso: " << programName << " --particles N [opciones]\n\n"
        << "Opciones:\n"
        << "  --particles N       Cantidad de particulas a renderizar "
        << "(obligatorio, mayor que 0)\n"
        << "  --width W           Ancho del canvas en pixeles (minimo "
        << MINIMUM_WIDTH << ", default " << DEFAULT_WIDTH << ")\n"
        << "  --height H          Alto del canvas en pixeles (minimo "
        << MINIMUM_HEIGHT << ", default " << DEFAULT_HEIGHT << ")\n"
        << "  --seed S            Semilla de los datos pseudoaleatorios "
        << "(default " << DEFAULT_SEED << ")\n"
        << "  --max-fps F         Limite de fotogramas por segundo "
        << "(default " << DEFAULT_MAX_FPS << ")\n"
        << "  -h, --help          Muestra esta ayuda\n";
}

bool parseArguments(
    const int argc,
    char* argv[],
    ProgramArguments& arguments
) {
    const auto fail = [](const std::string& message) {
        std::cerr << "Error: " << message << "\n\n";
        return false;
    };

    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];

        if (flag == "-h" || flag == "--help") {
            arguments.helpRequested = true;
            continue;
        }

        if (flag == "--particles") {
            if (index + 1 >= argc) {
                return fail("falta el valor de --particles.");
            }

            const ParsedNumber number = parseInteger(argv[++index]);

            if (!number.valid || number.value <= 0) {
                return fail("--particles exige un entero mayor que 0.");
            }

            arguments.particleCount =
                static_cast<std::size_t>(number.value);
            continue;
        }

        if (flag == "--width") {
            if (index + 1 >= argc) {
                return fail("falta el valor de --width.");
            }

            const ParsedNumber number = parseInteger(argv[++index]);

            if (!number.valid || number.value <= 0) {
                return fail("--width exige un entero positivo.");
            }

            arguments.width = static_cast<int>(number.value);
            continue;
        }

        if (flag == "--height") {
            if (index + 1 >= argc) {
                return fail("falta el valor de --height.");
            }

            const ParsedNumber number = parseInteger(argv[++index]);

            if (!number.valid || number.value <= 0) {
                return fail("--height exige un entero positivo.");
            }

            arguments.height = static_cast<int>(number.value);
            continue;
        }

        if (flag == "--seed") {
            if (index + 1 >= argc) {
                return fail("falta el valor de --seed.");
            }

            const ParsedNumber number = parseInteger(argv[++index]);

            if (!number.valid || number.value < 0 ||
                number.value > 0xFFFFFFFFLL) {
                return fail(
                    "--seed exige un entero entre 0 y 4294967295."
                );
            }

            arguments.seed = static_cast<unsigned int>(number.value);
            continue;
        }

        if (flag == "--max-fps") {
            if (index + 1 >= argc) {
                return fail("falta el valor de --max-fps.");
            }

            const ParsedNumber number = parseInteger(argv[++index]);

            if (!number.valid || number.value < 1) {
                return fail("--max-fps exige un entero mayor que 0.");
            }

            arguments.maxFps = static_cast<int>(number.value);
            continue;
        }

        return fail("argumento desconocido '" + flag + "'.");
    }

    return true;
}

bool validateArguments(const ProgramArguments& arguments) {
    const auto fail = [](const std::string& message) {
        std::cerr << "Error: " << message << "\n\n";
        return false;
    };

    if (arguments.particleCount == 0) {
        return fail("--particles es obligatorio.");
    }

    if (arguments.width < MINIMUM_WIDTH ||
        arguments.width > MAXIMUM_CANVAS_SIZE) {
        return fail(
            "El ancho del canvas debe estar entre " +
            std::to_string(MINIMUM_WIDTH) + " y " +
            std::to_string(MAXIMUM_CANVAS_SIZE) + "."
        );
    }

    if (arguments.height < MINIMUM_HEIGHT ||
        arguments.height > MAXIMUM_CANVAS_SIZE) {
        return fail(
            "El alto del canvas debe estar entre " +
            std::to_string(MINIMUM_HEIGHT) + " y " +
            std::to_string(MAXIMUM_CANVAS_SIZE) + "."
        );
    }

    if (arguments.maxFps < 1 || arguments.maxFps > MAXIMUM_FPS) {
        return fail(
            "--max-fps debe estar entre 1 y " +
            std::to_string(MAXIMUM_FPS) + "."
        );
    }

    return true;
}

struct FpsCounter {
    int framesRendered = 0;
    int currentFps = 0;
    std::uint64_t windowStart = 0;

    void update(SDL_Window* window, const std::uint64_t nowTicks) {
        ++framesRendered;

        if (nowTicks - windowStart < 500) {
            return;
        }

        currentFps = static_cast<int>(
            framesRendered * 1000.0 /
            static_cast<double>(nowTicks - windowStart)
        );

        framesRendered = 0;
        windowStart = nowTicks;

        char title[128];
        std::snprintf(
            title,
            sizeof(title),
            "Screensaver Paralelo - %d FPS",
            currentFps
        );

        SDL_SetWindowTitle(window, title);
    }
};

bool handleEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }

        if (
            event.type == SDL_KEYDOWN &&
            event.key.keysym.sym == SDLK_ESCAPE
        ) {
            return false;
        }
    }

    return true;
}

float computeDeltaTime(std::uint64_t& previousFrameTime) {
    const std::uint64_t currentTicks = SDL_GetTicks64();
    const float deltaSeconds =
        static_cast<float>(currentTicks - previousFrameTime) / 1000.0f;

    previousFrameTime = currentTicks;

    return std::clamp(deltaSeconds, 0.0001f, 0.05f);
}

void updateSimulation(
    std::vector<Particle>& particles,
    SimulationConfig& config,
    const float deltaSeconds
) {
    config.deltaTime = deltaSeconds;

    updateSequential(particles, config);
    resolveCollisionsSequential(particles, config);
}

void renderFrame(
    Renderer& renderer,
    const std::vector<Particle>& particles
) {
    clearRenderer(renderer);
    drawRendererSequential(renderer, particles);
    presentRenderer(renderer);
}

void throttleToMaxFps(
    const std::uint64_t frameStartTime,
    const int maxFps
) {
    const std::uint64_t frameElapsed =
        SDL_GetTicks64() - frameStartTime;
    const std::uint32_t targetFrameTimeMs =
        static_cast<std::uint32_t>(1000 / maxFps);

    if (frameElapsed < targetFrameTimeMs) {
        SDL_Delay(targetFrameTimeMs - frameElapsed);
    }
}

void runGameLoop(
    Renderer& renderer,
    std::vector<Particle>& particles,
    SimulationConfig& config,
    const int maxFps
) {
    FpsCounter fps;
    std::uint64_t previousFrameTime = SDL_GetTicks64();
    fps.windowStart = previousFrameTime;

    bool running = true;

    while (running) {
        running = handleEvents();

        const float deltaSeconds = computeDeltaTime(previousFrameTime);
        updateSimulation(particles, config, deltaSeconds);
        renderFrame(renderer, particles);

        const std::uint64_t nowTicks = SDL_GetTicks64();
        fps.update(renderer.window, nowTicks);
        throttleToMaxFps(nowTicks, maxFps);
    }
}

}

int main(int argc, char* argv[]) {
    ProgramArguments arguments;

    if (!parseArguments(argc, argv, arguments)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (arguments.helpRequested) {
        printUsage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (!validateArguments(arguments)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr
            << "Error al inicializar SDL: "
            << SDL_GetError()
            << '\n';
        return EXIT_FAILURE;
    }

    Renderer renderer;

    if (!initRenderer(renderer, arguments.width, arguments.height)) {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SimulationConfig config;
    config.width = arguments.width;
    config.height = arguments.height;
    config.particleCount = arguments.particleCount;
    config.randomSeed = arguments.seed;

    std::vector<Particle> particles;

    try {
        initializeParticles(particles, config);
    } catch (const std::invalid_argument& exception) {
        std::cerr
            << "Error en la configuracion: "
            << exception.what()
            << '\n';

        shutdownRenderer(renderer);
        SDL_Quit();

        return EXIT_FAILURE;
    }

    std::cout
        << "Screensaver iniciado con "
        << arguments.particleCount << " particulas, canvas "
        << arguments.width << 'x' << arguments.height
        << ", semilla " << arguments.seed
        << ", max fps " << arguments.maxFps << ".\n"
        << "Cierra la ventana o presiona ESC para salir.\n";

    runGameLoop(renderer, particles, config, arguments.maxFps);

    shutdownRenderer(renderer);
    SDL_Quit();

    return EXIT_SUCCESS;
}