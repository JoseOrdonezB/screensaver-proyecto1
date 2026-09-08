# Screensaver Paralelo con OpenMP

Proyecto #1 de **Computación Paralela y Distribuida**.

El proyecto implementa un screensaver de partículas en **C++17 + SDL2**, partiendo de una versión secuencial y desarrollando posteriormente una versión paralela utilizando **OpenMP**.

Las partículas se generan pseudoaleatoriamente, se desplazan dentro del canvas, rebotan contra las paredes y reaccionan a colisiones entre sí.

El proyecto incluye dos ejecutables del screensaver:

- `screensaver`: versión secuencial.
- `screensaver_parallel`: versión paralela con OpenMP.

También incluye herramientas para validar las colisiones y medir el rendimiento de ambas implementaciones.

---

## Características

- Simulación de N partículas.
- Colores generados pseudoaleatoriamente.
- Movimiento continuo.
- Rebotes contra los límites del canvas.
- Colisiones entre partículas.
- Corrección de solapamiento.
- Conservación aproximada del comportamiento físico mediante impulsos.
- Coeficientes de restitución para paredes y partículas.
- Renderizado con SDL2.
- Versión secuencial.
- Versión paralela con OpenMP.
- Número de hilos configurable.
- Validación automática de colisiones.
- Stress tests de la implementación paralela.
- Benchmark de movimiento.
- Benchmark de simulación completa.
- Cálculo de speedup y eficiencia.
- Exportación de mediciones a CSV.

---

## Tecnologías utilizadas

| Tecnología | Uso |
|---|---|
| C++17 | Implementación principal |
| OpenMP | Paralelización |
| SDL2 | Ventana y renderizado |
| CMake | Compilación |
| CSV | Almacenamiento de resultados |
| Git | Control de versiones |

---

## Estructura del proyecto

```text
screensaver-proyecto1/
│
├── CMakeLists.txt
├── README.md
│
└── src/
    ├── main.cpp
    ├── main_parallel.cpp
    │
    ├── particles.cpp
    ├── movement.cpp
    ├── collisions.cpp
    ├── renderer.cpp
    │
    ├── simulation.h
    ├── collisions.h
    ├── renderer.h
    │
    ├── benchmark.cpp
    └── collision_validation.cpp
```

### Archivos principales

`main.cpp` contiene el programa secuencial y administra la ventana, argumentos, simulación y game loop.

`main_parallel.cpp` contiene la versión paralela del screensaver y permite seleccionar la cantidad de hilos OpenMP.

`particles.cpp` se encarga de la creación e inicialización de las partículas.

`movement.cpp` contiene las implementaciones secuencial y paralela del movimiento.

`collisions.cpp` implementa la detección y resolución de colisiones entre partículas.

`renderer.cpp` contiene las operaciones relacionadas con SDL2 y el dibujo de las partículas.

`benchmark.cpp` compara el rendimiento de las versiones secuencial y paralela.

`collision_validation.cpp` contiene pruebas para verificar el funcionamiento de las colisiones.

---

# Compilación

## Dependencias

Se necesita:

- Compilador compatible con C++17.
- CMake 3.16 o superior.
- SDL2.
- OpenMP.

### Ubuntu / Debian / WSL

```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev
```

GCC normalmente incluye soporte para OpenMP.

Se puede comprobar con:

```bash
g++ --version
cmake --version
sdl2-config --version
```

---

## Compilar el proyecto

Desde la raíz:

```bash
cmake -S . -B build
cmake --build build -j
```

Al finalizar se generan los siguientes ejecutables:

```text
build/screensaver
build/screensaver_parallel
build/screensaver_benchmark
build/collision_validation
```

---

# Screensaver secuencial

La versión secuencial puede ejecutarse con:

```bash
./build/screensaver --particles 500
```

Ejemplo con parámetros adicionales:

```bash
./build/screensaver \
    --particles 1000 \
    --width 1280 \
    --height 720 \
    --seed 12345 \
    --max-fps 60
```

### Argumentos

| Argumento | Descripción |
|---|---|
| `--particles N` | Número de partículas. Obligatorio |
| `--width W` | Ancho del canvas |
| `--height H` | Alto del canvas |
| `--seed S` | Semilla pseudoaleatoria |
| `--max-fps F` | Límite de FPS |
| `--help` | Muestra la ayuda |

El tamaño mínimo permitido para el canvas es **640x480**.

---

# Screensaver paralelo

La versión paralela utiliza OpenMP para ejecutar las partes paralelizables de la simulación.

Ejemplo:

```bash
./build/screensaver_parallel --particles 500 --threads 4
```

También puede probarse con diferentes cantidades de hilos:

```bash
./build/screensaver_parallel --particles 500 --threads 1
./build/screensaver_parallel --particles 500 --threads 2
./build/screensaver_parallel --particles 500 --threads 4
./build/screensaver_parallel --particles 500 --threads 8
```

Si no se especifica `--threads`, se utiliza la cantidad máxima de hilos reportada por OpenMP.

### Argumentos

| Argumento | Descripción |
|---|---|
| `--particles N` | Número de partículas. Obligatorio |
| `--threads T` | Número de hilos OpenMP |
| `--width W` | Ancho del canvas |
| `--height H` | Alto del canvas |
| `--seed S` | Semilla pseudoaleatoria |
| `--max-fps F` | Límite de FPS |
| `--help` | Muestra la ayuda |

Ejemplo:

```bash
./build/screensaver_parallel \
    --particles 1000 \
    --threads 4 \
    --width 1280 \
    --height 720
```

---

# Paralelización

La simulación contiene dos partes principales que se analizaron para paralelización:

1. Movimiento de partículas.
2. Detección de colisiones.

## Movimiento

El movimiento de las partículas es apropiado para paralelización porque cada iteración puede actualizar una partícula diferente.

La versión paralela utiliza OpenMP con un esquema de distribución configurable.

Conceptualmente:

```cpp
#pragma omp parallel for schedule(runtime) \
    num_threads(config.threadCount)
for (...) {
    // actualizar particula
}
```

El esquema se define en tiempo de ejecución con `omp_set_schedule()`. Por defecto se usa `static` (distribución estática), pero el benchmark puede comparar las tres variantes disponibles.

Esto permite distribuir las partículas entre varios hilos.

---

## Colisiones

Las colisiones requieren mayor cuidado porque una colisión puede modificar simultáneamente dos partículas.

Una primera implementación utilizaba una sección crítica para cada pareja:

```cpp
#pragma omp critical
{
    resolveParticlePair(...);
}
```

Aunque esta estrategia protegía el acceso a memoria compartida, generaba una gran cantidad de sincronización y reducía considerablemente el rendimiento.

La implementación optimizada separa el proceso en dos etapas.

### 1. Detección paralela

Los hilos buscan parejas de partículas que potencialmente están colisionando.

Durante esta fase las partículas son únicamente leídas.

Cada hilo mantiene su propia lista local de parejas:

```text
Thread 0 -> candidatos locales
Thread 1 -> candidatos locales
Thread 2 -> candidatos locales
Thread 3 -> candidatos locales
```

Esto evita que varios hilos modifiquen simultáneamente la misma estructura.

Al igual que en el movimiento, el loop de detección usa `schedule(runtime)` con `omp_set_schedule()`, permitiendo comparar esquemas estáticos, dinámicos y guiados.

### 2. Resolución

Después de finalizar la detección paralela existe una barrera implícita de OpenMP.

Las parejas encontradas son posteriormente procesadas para corregir:

- solapamiento;
- posición;
- velocidad;
- impulso de colisión.

Esta estrategia reduce la contención sobre memoria compartida y evita utilizar una sección crítica para cada una de las posibles parejas.

---

# Complejidad de colisiones

Para `N` partículas, una búsqueda directa de todas las parejas necesita evaluar aproximadamente:

```text
N(N - 1) / 2
```

combinaciones.

Por ejemplo:

```text
N = 1000

1000 * 999 / 2 = 499500 parejas
```

Por esta razón el benchmark de simulación completa utiliza cantidades menores de partículas que el benchmark exclusivo de movimiento.

---

# Validación

El proyecto incluye un ejecutable específico:

```bash
./build/collision_validation
```

La validación comprueba:

- colisión frontal entre dos partículas;
- intercambio esperado de velocidades;
- corrección del solapamiento;
- rebote contra una pared;
- versión secuencial;
- versión paralela con 1 hilo;
- versión paralela con 2 hilos;
- versión paralela con 4 hilos;
- versión paralela con 8 hilos.

También se ejecuta un stress test con múltiples partículas y repeticiones.

Una ejecución correcta produce:

```text
Validacion de colisiones

Secuencial: OK

Paralelo:
  1 hilo:  OK
  2 hilos: OK
  4 hilos: OK
  8 hilos: OK

Colision contra pared: OK

Stress test paralelo
Particulas por prueba: 500
Repeticiones: 20

  1 hilo:  OK
  2 hilos: OK
  4 hilos: OK
  8 hilos: OK

Todas las validaciones fueron correctas.
```

---

# Benchmark

Para ejecutar las pruebas de rendimiento:

```bash
./build/screensaver_benchmark
```

El benchmark realiza **10 repeticiones por configuración** y genera:

```text
resultados_benchmark.csv
```

El archivo contiene:

```text
benchmark
particles
frames
threads
schedule
repetition
sequential_ms
parallel_ms
speedup
efficiency
```

La columna `schedule` indica el esquema de distribución de OpenMP utilizado: `static`, `dynamic` o `guided`.

---

## Benchmark de movimiento

Compara:

```text
updateSequential()
        vs
updateParallel()
```

Actualmente se prueban:

```text
1 000 partículas
10 000 partículas
100 000 partículas
```

con:

```text
1, 2, 4 y 8 hilos
```

y los esquemas de distribución:

```text
static, dynamic y guided
```

y 200 frames por medición.

---

## Benchmark de simulación completa

También se mide:

```text
SECUENCIAL

updateSequential()
+
resolveCollisionsSequential()
```

contra:

```text
PARALELO

updateParallel()
+
resolveCollisionsParallel()
```

Actualmente se prueban:

```text
250 partículas
500 partículas
1000 partículas
```

con 50 frames por medición y 10 repeticiones, probando también los tres esquemas de distribución (`static`, `dynamic` y `guided`).

---

## Comparación de esquemas de distribución

El proyecto compara los tres esquemas de distribución de OpenMP que ofrece `schedule(runtime)`:

- `static`: bloques contiguos de iteraciones repartidos por adelantado.
- `dynamic`: las iteraciones se asignan dinámicamente en trozos pequeños.
- `guided`: trozos decrecientes, similar a dynamic pero con menos overhead.

La elección del esquema depende del tipo de trabajo:

- **Trabajo uniforme** (movimiento): todas las iteraciones tardan lo mismo. `static` es el adecuado porque no agrega overhead.
- **Trabajo desbalanceado** (detección de colisiones): el loop interno depende de `i`, por lo que las primeras iteraciones hacen mucho más trabajo que las últimas. Aquí `dynamic` o `guided` pueden mejorar el balanceo.

En el benchmark de simulación con 1000 partículas y 8 hilos, las mediciones de desarrollo mostraron:

| Esquema | Speedup promedio |
|---:|---:|
| static | **1.449x** |
| guided | 1.290x |
| dynamic | 1.276x |

También se puede ver el efecto contrario: en el benchmark de movimiento con uniformidad total y pocas partículas, el overhead de `dynamic` degrada el rendimiento respecto a `static`.

Esto demuestra que la elección del esquema de distribución es otra decisión que debe basarse en mediciones y no en suposiciones.

---

# Speedup y eficiencia

El speedup indica cuántas veces más rápida es la implementación paralela respecto a la secuencial:

```text
Speedup = Tiempo secuencial / Tiempo paralelo
```

Un resultado:

```text
Speedup > 1
```

indica una mejora de rendimiento.

La eficiencia se calcula como:

```text
Eficiencia = Speedup / Número de hilos
```

y permite analizar qué tan bien se aprovechan los recursos disponibles.

---

# Resultados preliminares

Las mediciones dependen del hardware y de la carga del sistema, por lo que los resultados definitivos deben obtenerse en condiciones controladas.

En las pruebas de desarrollo realizadas hasta ahora se observó speedup tanto en el movimiento como en la simulación completa.

Por ejemplo, una ejecución de desarrollo de la simulación completa produjo:

| Partículas | Hilos | Speedup |
|---:|---:|---:|
| 250 | 2 | 1.817x |
| 250 | 4 | **2.528x** |
| 250 | 8 | 2.250x |
| 500 | 2 | 1.281x |
| 500 | 4 | 1.489x |
| 500 | 8 | **1.679x** |
| 1000 | 2 | 1.322x |
| 1000 | 4 | **1.840x** |
| 1000 | 8 | 1.835x |

Estos valores son preliminares y pueden variar entre ejecuciones.

Un aspecto observado es que aumentar la cantidad de hilos no garantiza automáticamente un mejor resultado. Para cargas pequeñas, el costo de crear, coordinar y sincronizar trabajo paralelo puede superar el beneficio obtenido.

---

# Evolución de la versión paralela

Durante el desarrollo se implementaron y evaluaron diferentes estrategias.

La primera versión paralela de colisiones utilizaba una sección crítica para cada posible pareja de partículas.

Aunque era una solución segura para el acceso compartido, las mediciones mostraron una fuerte pérdida de rendimiento.

En una prueba con 1000 partículas y 8 hilos se obtuvo aproximadamente:

```text
Speedup: 0.049x
```

Esto indicó que la sincronización se había convertido en un cuello de botella.

Después de modificar la estrategia para separar la detección de candidatos y la resolución de las colisiones, la misma clase de prueba alcanzó un speedup superior a:

```text
1.8x
```

La comparación muestra por qué la paralelización no consiste únicamente en agregar hilos: también es necesario analizar la granularidad del trabajo, la sincronización y el acceso a memoria compartida.

---

# Pruebas manuales

Algunos comandos útiles para verificar el proyecto:

### Secuencial

```bash
./build/screensaver --particles 500
```

### Paralelo con 2 hilos

```bash
./build/screensaver_parallel --particles 500 --threads 2
```

### Paralelo con 4 hilos

```bash
./build/screensaver_parallel --particles 500 --threads 4
```

### Paralelo con 8 hilos

```bash
./build/screensaver_parallel --particles 500 --threads 8
```

### Validación

```bash
./build/collision_validation
```

### Benchmark

```bash
./build/screensaver_benchmark
```

---

# Salir del screensaver

Para cerrar cualquiera de las versiones:

- cerrar la ventana; o
- presionar `ESC`.

---

# Notas sobre rendimiento

Los resultados pueden variar dependiendo de:

- procesador;
- número de núcleos e hilos disponibles;
- sistema operativo;
- compilador;
- optimizaciones de compilación;
- procesos ejecutándose en segundo plano;
- cantidad de partículas;
- cantidad de hilos OpenMP.

Por esta razón las comparaciones de rendimiento deben realizarse en la misma computadora y bajo condiciones similares.

Para resultados finales se recomienda utilizar una compilación optimizada.

Por ejemplo:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

# Objetivo del proyecto

El objetivo principal es analizar el proceso de transformación de una solución secuencial a una solución paralela utilizando memoria compartida.

El proyecto permite observar conceptos como:

- descomposición del problema;
- distribución de trabajo;
- memoria compartida;
- sincronización;
- granularidad;
- overhead;
- speedup;
- eficiencia;
- escalabilidad.

La comparación entre diferentes implementaciones permite observar que una versión paralela no necesariamente es más rápida por el simple hecho de utilizar más hilos.

La optimización requiere medir, identificar cuellos de botella y modificar la estrategia de paralelización.

---

## Estado actual

```text
[OK] Screensaver secuencial
[OK] Screensaver paralelo con OpenMP
[OK] Movimiento paralelo
[OK] Colisiones entre partículas
[OK] Detección paralela de colisiones
[OK] Configuración de número de hilos
[OK] Esquemas de distribución configurables (static/dynamic/guided)
[OK] Comparación de esquemas en el benchmark
[OK] Validación de colisiones
[OK] Stress tests
[OK] Benchmark de movimiento
[OK] Benchmark de simulación completa
[OK] Cálculo de speedup
[OK] Cálculo de eficiencia
[OK] Exportación de resultados a CSV
```

El proyecto se encuentra actualmente en la etapa de **pruebas de rendimiento y documentación de resultados**.