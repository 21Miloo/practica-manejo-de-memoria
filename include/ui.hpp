// ui.hpp - dibujo en terminal: colores, cajas y visualizacion de memoria.
#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ui {

// Como se dibuja la salida. En modo Web no se emiten cajas de caracteres ni
// codigos ANSI: se emiten lineas con marcas que el navegador convierte en DOM.
enum class Modo { kTerminal, kWeb };
extern Modo modo;

// Marcas de estructura (primer byte de la linea) y de color, usadas solo en
// modo Web. Son bytes de control, asi que nunca chocan con el contenido.
constexpr char kMarcaColorInicio = '\x02';   // \x02 CODIGO \x03 texto \x04
constexpr char kMarcaColorTexto = '\x03';
constexpr char kMarcaColorFin = '\x04';
constexpr char kMarcaCaja = '\x05';          // abre una tarjeta; resto = encabezado
constexpr char kMarcaFin = '\x06';           // cierra el bloque abierto
constexpr char kMarcaTitulo = '\x07';        // titulo de nivel; resto = texto
constexpr char kMarcaCodigo = '\x08';        // abre un bloque de codigo
constexpr char kMarcaMono = '\x0b';          // abre un bloque monoespaciado
constexpr char kMarcaSeparador = '\x0e';     // linea divisoria
constexpr char kMarcaEntrada = '\x0f';       // el motor pide una linea al usuario
constexpr char kMarcaEstado = '\x10';        // estado de la partida en JSON
constexpr char kMarcaFinPartida = '\x11';    // la partida termino
constexpr char kMarcaPregunta = '\x12';      // enunciado del reto

// Si es false, todas las funciones de color devuelven el texto tal cual.
extern bool color_activo;

// Lector de entrada. Si esta vacio se lee de la entrada estandar; la version
// web instala aqui un lector que pide la linea al navegador.
using LectorEntrada = std::function<bool(const std::string&, std::string&)>;
extern LectorEntrada lector_entrada;

// Pide una linea al jugador mostrando 'prompt'. Devuelve false si se acabo la
// entrada. Es el unico punto por el que el juego lee del usuario.
bool leer_linea(const std::string& prompt, std::string& destino);

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
