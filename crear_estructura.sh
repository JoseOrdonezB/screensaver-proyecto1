#!/usr/bin/env bash

set -euo pipefail

PROJECT_NAME="${1:-screensaver-paralelo}"

if [[ -e "$PROJECT_NAME" ]]; then
    echo "Error: ya existe un archivo o carpeta llamada '$PROJECT_NAME'." >&2
    echo "Usa otro nombre o ejecuta el script desde otra ubicación." >&2
    exit 1
fi

echo "Creando estructura del proyecto: $PROJECT_NAME"

mkdir -p "$PROJECT_NAME/src"

touch \
    "$PROJECT_NAME/CMakeLists.txt" \
    "$PROJECT_NAME/README.md" \
    "$PROJECT_NAME/.gitignore" \
    "$PROJECT_NAME/src/main.cpp" \
    "$PROJECT_NAME/src/simulation.h" \
    "$PROJECT_NAME/src/particles.cpp" \
    "$PROJECT_NAME/src/movement.cpp" \
    "$PROJECT_NAME/src/collisions.h" \
    "$PROJECT_NAME/src/collisions.cpp" \
    "$PROJECT_NAME/src/renderer.h" \
    "$PROJECT_NAME/src/renderer.cpp"

echo
echo "Estructura creada correctamente en: $PROJECT_NAME/"
echo
echo "Para entrar al proyecto ejecuta:"
echo "  cd \"$PROJECT_NAME\""