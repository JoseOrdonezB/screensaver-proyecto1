cmake_minimum_required(VERSION 3.16)
project(screensaver_parallel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OpenMP no viene incluido con Apple clang por defecto.
# En macOS con Homebrew vive en /opt/homebrew/opt/libomp.
# Se puede sobrescribir pasando -DOpenMP_ROOT=<ruta>.
if(NOT DEFINED OpenMP_ROOT AND EXISTS "/opt/homebrew/opt/libomp")
    set(OpenMP_ROOT "/opt/homebrew/opt/libomp" CACHE PATH "Ubicacion de libomp de Homebrew")
endif()

find_package(OpenMP REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(screensaver_benchmark
    src/benchmark.cpp
    src/particles.cpp
    src/movement.cpp
    src/collisions.cpp
)

target_include_directories(screensaver_benchmark PRIVATE src)
target_link_libraries(screensaver_benchmark PRIVATE OpenMP::OpenMP_CXX)

add_executable(collision_validation
    src/collision_validation.cpp
    src/particles.cpp
    src/movement.cpp
    src/collisions.cpp
)

target_include_directories(collision_validation PRIVATE src)
target_link_libraries(collision_validation PRIVATE OpenMP::OpenMP_CXX)

add_executable(screensaver
    src/main.cpp
    src/renderer.cpp
    src/particles.cpp
    src/movement.cpp
    src/collisions.cpp
)

target_include_directories(screensaver PRIVATE src)
target_link_libraries(screensaver PRIVATE
    OpenMP::OpenMP_CXX
    SDL2::SDL2
)

enable_testing()
add_test(NAME collision_validation COMMAND collision_validation)
