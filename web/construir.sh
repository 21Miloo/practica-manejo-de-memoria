#!/usr/bin/env bash
# Compila el motor de MemLab a WebAssembly.
#
# Requiere el SDK de Emscripten activado:
#   git clone https://github.com/emscripten-core/emsdk.git
#   cd emsdk && ./emsdk install latest && ./emsdk activate latest
#   source ./emsdk_env.sh
#
# Uso: ./web/construir.sh
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v em++ > /dev/null; then
  echo "No encuentro em++. Activa el SDK de Emscripten:" >&2
  echo "  source /ruta/a/emsdk/emsdk_env.sh" >&2
  exit 1
fi

mkdir -p web/wasm

em++ -std=c++17 -O2 -Iinclude \
  src/web.cpp src/util.cpp src/ui.cpp src/juego.cpp src/niveles/*.cpp \
  -sASYNCIFY \
  -sMODULARIZE -sEXPORT_ES6 -sEXPORT_NAME=crearMemLab \
  -sEXPORTED_FUNCTIONS=_main,_malloc,_free,_memlab_jugar,_memlab_total_niveles \
  -sEXPORTED_RUNTIME_METHODS=UTF8ToString,stringToUTF8,lengthBytesUTF8,ccall \
  -sALLOW_MEMORY_GROWTH \
  -o web/wasm/memlab.js

echo "Listo: web/wasm/memlab.js + memlab.wasm"
echo "Para jugar:  python3 -m http.server -d web 8000   y abre http://localhost:8000"
