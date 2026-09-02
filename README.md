# Screensaver Paralelo

Proyecto 1 del curso de Computación Paralela y Distribuida.

## Descripción

El proyecto consiste en desarrollar un screensaver de partículas en movimiento utilizando C++.

Primero se implementará una versión secuencial y posteriormente una versión paralela utilizando OpenMP. El programa incluirá movimiento, rebotes contra los bordes, colisiones entre partículas y renderizado mediante SDL.

## Integrantes y responsabilidades

### Persona 1 —  Osman de León - Simulación y movimiento

* Inicialización de partículas.
* Movimiento, velocidad y física.
* Rebotes contra los bordes.
* Versión secuencial y paralela con OpenMP.
* Pruebas y benchmarks.

### Persona 2 — Colisiones e interacciones

* Detección de colisiones.
* Resolución de colisiones.
* Paralelización con OpenMP.
* Manejo de sincronización y race conditions.
* Pruebas y benchmarks.

### Persona 3 — Renderizado e infraestructura

* Ventana y renderizado con SDL.
* Manejo de eventos.
* Argumentos de línea de comandos.
* Integración de los módulos.
* Estabilidad final del programa.

## Tecnologías

* C++ (estándar 17)
* OpenMP
* SDL2
* CMake

## Requisitos y compilación

En macOS se necesita CMake, SDL2 y la librería de OpenMP (libomp):

```bash
brew install cmake sdl2 libomp
```

OpenMP no incluye Apple clang por defecto; el `CMakeLists.txt` detecta
automáticamente la instalación de Homebrew en `/opt/homebrew/opt/libomp`.

Para compilar:

```bash
cmake -S . -B build
cmake --build build
```

Ejecutables generados en `build/`:

* `screensaver` — el screensaver con ventana.
* `screensaver_benchmark` — mide tiempos secuencial/paralelo y calcula
  speedup y eficiencia (genera `resultados_benchmark.csv`).
* `collision_validation` — prueba de validación de colisiones.

## Uso del screensaver

```bash
./build/screensaver --particles N  [opciones]
```

### Argumentos

| Flag | Descripción | Default |
| --- | --- | --- |
| `--particles N` | Cantidad de partículas a renderizar (obligatorio, > 0) | — |
| `--width W` | Ancho del canvas en píxeles (mínimo 640) | 800 |
| `--height H` | Alto del canvas en píxeles (mínimo 480) | 600 |
| `--seed S` | Semilla de los datos pseudoaleatorios | 12345 |
| `--max-fps F` | Límite de fotogramas por segundo | 60 |
| `-h`, `--help` | Muestra la ayuda | — |

### Ejemplos

```bash
./build/screensaver --particles 500
./build/screensaver --particles 2000 --width 1280 --height 720
./build/screensaver --particles 100 --seed 7 --max-fps 120
```

### Controles

* `ESC` o cerrar la ventana: termina el programa.
* El título de la ventana muestra los FPS actuales.

## Estado del proyecto

Versión secuencial funcionando: movimiento, rebotes contra los bordes,
colisiones entre partículas y renderizado con framebuffer en SDL2.
Pendiente: integración de la versión paralela con OpenMP y sus benchmarks.

## Estructura principal

* `src/particles.cpp` — inicialización de partículas y configuración.
* `src/movement.cpp` — movimiento, física y rebotes (secuencial/paralelo).
* `src/collisions.cpp` — detección y resolución de colisiones (secuencial/paralelo).
* `src/renderer.cpp` — ventana SDL y framebuffer de píxeles.
* `src/main.cpp` — argumentos de línea de comandos y game loop.
* `src/benchmark.cpp` — mediciones de speedup y eficiencia.
