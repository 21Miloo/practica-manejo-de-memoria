#include "ui.hpp"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>

namespace ui {

bool color_activo = true;

std::string tinte(const std::string& codigo, const std::string& texto) {
  if (!color_activo) return texto;
  return "\033[" + codigo + "m" + texto + "\033[0m";
}

std::size_t ancho(const std::string& s) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < s.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (c == 0x1b) {  // secuencia ANSI: \033[...m
      while (i < s.size() && s[i] != 'm') ++i;
      continue;
    }
    if ((c & 0xC0) == 0x80) continue;  // byte de continuacion UTF-8
    ++n;
  }
  return n;
}

namespace {

std::string repetir(const std::string& s, std::size_t n) {
  std::string salida;
  for (std::size_t i = 0; i < n; ++i) salida += s;
  return salida;
}

constexpr std::size_t kAnchoCaja = 74;

}  // namespace

std::vector<std::string> envolver(const std::string& texto, std::size_t ancho_max) {
  std::vector<std::string> lineas;
  std::istringstream is(texto);
  std::string palabra, actual;
  while (is >> palabra) {
    if (actual.empty()) {
      actual = palabra;
    } else if (ancho(actual) + 1 + ancho(palabra) <= ancho_max) {
      actual += " " + palabra;
    } else {
      lineas.push_back(actual);
      actual = palabra;
    }
  }
  if (!actual.empty()) lineas.push_back(actual);
  if (lineas.empty()) lineas.push_back("");
  return lineas;
}

void parrafo(const std::string& texto, const std::string& sangria, bool en_gris) {
  for (const std::string& l : envolver(texto, kAnchoCaja - ancho(sangria))) {
    std::cout << sangria << (en_gris ? gris(l) : l) << "\n";
  }
}

void titulo(const std::string& texto) {
  const std::string linea = repetir("═", kAnchoCaja);
  std::cout << "\n" << cian("╔" + linea + "╗") << "\n";
  const std::size_t hueco = kAnchoCaja > ancho(texto) + 2 ? kAnchoCaja - ancho(texto) - 2 : 0;
  std::cout << cian("║") << " " << fuerte(texto) << std::string(hueco, ' ') << " "
            << cian("║") << "\n";
  std::cout << cian("╚" + linea + "╝") << "\n";
}

void caja(const std::string& encabezado, const std::vector<std::string>& lineas) {
  const std::string barra = repetir("─", kAnchoCaja);
  std::cout << gris("┌" + barra + "┐") << "\n";
  if (!encabezado.empty()) {
    const std::size_t hueco =
        kAnchoCaja > ancho(encabezado) + 2 ? kAnchoCaja - ancho(encabezado) - 2 : 0;
    std::cout << gris("│") << " " << amarillo(encabezado) << std::string(hueco, ' ') << " "
              << gris("│") << "\n";
    std::cout << gris("├" + barra + "┤") << "\n";
  }
  std::vector<std::string> ajustadas;
  for (const std::string& l : lineas) {
    // Las lineas con color se dejan tal cual: ya vienen medidas por quien las creo.
    const bool tiene_color = l.find('\033') != std::string::npos;
    if (!tiene_color && ancho(l) > kAnchoCaja - 2) {
      for (const std::string& parte : envolver(l, kAnchoCaja - 2)) ajustadas.push_back(parte);
    } else {
      ajustadas.push_back(l);
    }
  }
  for (const std::string& l : ajustadas) {
    const std::size_t hueco = kAnchoCaja > ancho(l) + 2 ? kAnchoCaja - ancho(l) - 2 : 0;
    std::cout << gris("│") << " " << l << std::string(hueco, ' ') << " " << gris("│")
              << "\n";
  }
  std::cout << gris("└" + barra + "┘") << "\n";
}

void separador() { std::cout << gris(repetir("┄", kAnchoCaja + 2)) << "\n"; }

void codigo(const std::vector<std::string>& lineas) {
  for (const std::string& l : lineas) std::cout << "    " << verde(l) << "\n";
}

std::string volcado(const void* p, std::size_t n, std::size_t por_fila,
                    std::size_t resaltar_desde, std::size_t resaltar_cuantos) {
  const unsigned char* bytes = static_cast<const unsigned char*>(p);
  std::ostringstream os;
  for (std::size_t fila = 0; fila < n; fila += por_fila) {
    char direccion[32];
    std::snprintf(direccion, sizeof(direccion), "%p", static_cast<const void*>(bytes + fila));
    os << gris(direccion) << gris("  │ ");

    std::string ascii;
    for (std::size_t i = 0; i < por_fila; ++i) {
      if (fila + i >= n) {
        os << "   ";
        ascii += ' ';
        continue;
      }
      char hex[8];
      std::snprintf(hex, sizeof(hex), "%02x ", bytes[fila + i]);
      const bool marcado = resaltar_cuantos > 0 && fila + i >= resaltar_desde &&
                           fila + i < resaltar_desde + resaltar_cuantos;
      os << (marcado ? magenta(hex) : std::string(hex));
      const unsigned char c = bytes[fila + i];
      ascii += (std::isprint(c) ? static_cast<char>(c) : '.');
    }
    os << gris("│ ") << gris(ascii) << "\n";
  }
  std::string s = os.str();
  if (!s.empty()) s.pop_back();
  return s;
}

std::string bits(unsigned long long v, int nbits) {
  std::string salida;
  for (int i = nbits - 1; i >= 0; --i) {
    salida.push_back(((v >> i) & 1ull) ? '1' : '0');
    if (i % 4 == 0 && i != 0) salida.push_back(' ');
  }
  return salida;
}

std::string regla_bits(int nbits) {
  std::string salida;
  for (int i = nbits - 1; i >= 0; --i) {
    salida.push_back(static_cast<char>('0' + (i % 10)));
    if (i % 4 == 0 && i != 0) salida.push_back(' ');
  }
  return salida;
}

std::string celdas(const std::vector<std::string>& valores, int ancho_celda) {
  std::ostringstream os;
  os << "│";
  for (const std::string& v : valores) {
    const std::size_t libre =
        static_cast<std::size_t>(ancho_celda) > ancho(v) ? ancho_celda - ancho(v) : 0;
    const std::size_t izq = libre / 2;
    os << std::string(izq, ' ') << v << std::string(libre - izq, ' ') << "│";
  }
  return os.str();
}

std::string dir(const void* p) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%p", p);
  return buf;
}

std::string dir(unsigned long long v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%llx", v);
  return buf;
}

}  // namespace ui
