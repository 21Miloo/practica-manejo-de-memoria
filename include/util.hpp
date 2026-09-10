// util.hpp - utilidades de texto y numeros para MemLab.
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace util {

// Quita espacios en blanco al inicio y al final.
std::string recortar(std::string s);

// Pasa a minusculas (ASCII).
std::string a_minusculas(std::string s);

// Elimina espacios, guiones bajos, comas y apostrofes: "1010 1100" -> "10101100".
std::string sin_separadores(std::string s);

// Normaliza para comparar texto libre: minusculas, sin acentos, sin signos.
std::string normalizar(std::string s);

// Interpreta "0x2a", "0b1010", "42", "-7". Devuelve vacio si no es un numero.
std::optional<long long> parsear_entero(const std::string& s);

// Interpreta una cadena que solo contiene 0 y 1 como binario.
std::optional<unsigned long long> parsear_bits(const std::string& s);

// Interpreta una direccion: "0x7ffd..." o "7ffd..." (siempre hexadecimal).
std::optional<unsigned long long> parsear_direccion(const std::string& s);

// Representaciones canonicas.
std::string a_binario(unsigned long long v, int bits);
std::string a_hex(unsigned long long v, int digitos = 0);

}  // namespace util
