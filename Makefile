# MemLab - compilacion sencilla con make.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
BIN      := memlab
BIN_TEST := memlab_pruebas

FUENTES_COMUNES := src/util.cpp src/ui.cpp src/juego.cpp $(wildcard src/niveles/*.cpp)

.PHONY: todo limpiar pruebas demo jugar ejemplos ejemplos-limpiar
todo: $(BIN)

$(BIN): src/main.cpp $(FUENTES_COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_TEST): tests/pruebas.cpp $(FUENTES_COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o $@

pruebas: $(BIN_TEST)
	./$(BIN_TEST)

jugar: $(BIN)
	./$(BIN)

demo: $(BIN)
	./$(BIN) --demo --semilla 7

# Los ejemplos se compilan con AddressSanitizer para que los errores de memoria
# se vean en vez de quedar escondidos. Fallar es justo lo que deben hacer.
EJEMPLOS_FUENTES := $(wildcard ejemplos/*.cpp)
EJEMPLOS_BIN     := $(patsubst ejemplos/%.cpp,build/ejemplos/%,$(EJEMPLOS_FUENTES))

ejemplos: $(EJEMPLOS_BIN)
	@echo "Ejecutables en build/ejemplos/. Ejecutalos uno a uno, por ejemplo:"
	@echo "  ./build/ejemplos/01_fuga"

build/ejemplos/%: ejemplos/%.cpp
	@mkdir -p build/ejemplos
	$(CXX) -std=c++17 -g -fsanitize=address,undefined $< -o $@

limpiar:
	rm -f $(BIN) $(BIN_TEST)
	rm -rf build
