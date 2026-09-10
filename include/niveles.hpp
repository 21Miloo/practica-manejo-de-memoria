// niveles.hpp - constructores de cada nivel del juego.
#pragma once

#include <random>

#include "juego.hpp"

Nivel nivel_bits(std::mt19937& azar);
Nivel nivel_bytes(std::mt19937& azar);
Nivel nivel_representacion(std::mt19937& azar);
Nivel nivel_punteros(std::mt19937& azar);
Nivel nivel_contiguidad(std::mt19937& azar);
Nivel nivel_heap(std::mt19937& azar);
