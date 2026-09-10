// ui.hpp - dibujo en terminal: colores, cajas y visualizacion de memoria.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ui {

// Si es false, todas las funciones de color devuelven el texto tal cual.
extern bool color_activo;

std::string tinte(const std::string& codigo, const std::string& texto);

inline std::string rojo(const std::string& t) { return tinte("31", t); }
inline std::string verde(const std::string& t) { return tinte("32", t); }
inline std::string amarillo(const std::string& t) { return tinte("33", t); }
inline std::string azul(const std::string& t) { return tinte("34", t); }
inline std::string magenta(const std::string& t) { return tinte("35", t); }
inline std::string cian(const std::string& t) { return tinte("36", t); }
inline std::string gris(const std::string& t) { return tinte("90", t); }
inline std::string fuerte(const std::string& t) { return tinte("1", t); }

// Ancho visible de una cadena UTF-8 (ignora secuencias ANSI y bytes de
// continuacion; suficiente para el texto que usa el juego).
std::size_t ancho(const std::string& s);

// Parte un texto en lineas de como mucho 'ancho_max' columnas, sin cortar
// palabras. Solo para texto sin secuencias de color.
std::vector<std::string> envolver(const std::string& texto, std::size_t ancho_max = 70);

// Imprime un parrafo envuelto con sangria (para explicaciones y enunciados).
void parrafo(const std::string& texto, const std::string& sangria = "  ",
             bool en_gris = false);

void titulo(const std::string& texto);
void caja(const std::string& encabezado, const std::vector<std::string>& lineas);
void separador();
void codigo(const std::vector<std::string>& lineas);

// --- Visualizacion de memoria -------------------------------------------

// Volcado hexadecimal de memoria real, con direcciones y columna ASCII.
// 'resaltar_desde'/'resaltar_cuantos' pintan un rango de bytes.
std::string volcado(const void* p, std::size_t n, std::size_t por_fila = 8,
                    std::size_t resaltar_desde = 0, std::size_t resaltar_cuantos = 0);

// "0100 1010" con los bits agrupados de a 4.
std::string bits(unsigned long long v, int nbits);

// Regla con el indice de cada bit, alineada con bits().
std::string regla_bits(int nbits);

// Fila de celdas de un arreglo: | 10 | 20 | 30 |
std::string celdas(const std::vector<std::string>& valores, int ancho_celda = 6);

// Direccion en hexadecimal, con formato uniforme.
std::string dir(const void* p);
std::string dir(unsigned long long v);

}  // namespace ui
