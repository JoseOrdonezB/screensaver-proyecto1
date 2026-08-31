cmake_minimum_required(VERSION 3.16)
project(screensaver_parallel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

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

enable_testing()
add_test(NAME collision_validation COMMAND collision_validation)
