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

* C++
* OpenMP
* SDL
* CMake

## Estado del proyecto

Proyecto en etapa inicial de desarrollo.

## Estructura principal

* `include/`: archivos de encabezado.
* `src/`: implementación del programa.
* `tests/`: pruebas de los módulos.
* `benchmarks/`: pruebas de rendimiento.
* `docs/`: documentación y resultados.
* `scripts/`: scripts auxiliares.
