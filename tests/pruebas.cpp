// Pruebas automaticas de MemLab, sin dependencias externas.
// La prueba clave: para cada reto de cada nivel, su respuesta canonica debe
// pasar su propio verificador (con varias semillas distintas).
#include <iostream>
#include <string>
#include <vector>

#include "juego.hpp"
#include "util.hpp"

namespace {

int fallos = 0;
int comprobaciones = 0;

void revisar(bool condicion, const std::string& descripcion) {
  ++comprobaciones;
  if (!condicion) {
    ++fallos;
    std::cout << "  FALLO: " << descripcion << "\n";
  }
}

void probar_util() {
  revisar(util::parsear_entero("42").value_or(-1) == 42, "decimal");
  revisar(util::parsear_entero("0x2a").value_or(-1) == 42, "hexadecimal");
  revisar(util::parsear_entero("0X2A").value_or(-1) == 42, "hexadecimal en mayusculas");
  revisar(util::parsear_entero("0b101010").value_or(-1) == 42, "binario con prefijo");
  revisar(util::parsear_entero("-7").value_or(0) == -7, "negativo");
  revisar(!util::parsear_entero("hola").has_value(), "texto no es numero");
  revisar(!util::parsear_entero("").has_value(), "cadena vacia");
  revisar(!util::parsear_entero("0x2g").has_value(), "digito hex invalido");

  revisar(util::parsear_bits("0010 1010").value_or(0) == 42, "bits con espacios");
  revisar(util::parsear_bits("00101010").value_or(0) == 42, "bits pegados");
  revisar(!util::parsear_bits("2").has_value(), "los bits solo son 0 y 1");

  revisar(util::parsear_direccion("0x7ffd1234").value_or(0) == 0x7ffd1234ull, "direccion con 0x");
  revisar(util::parsear_direccion("7ffd1234").value_or(0) == 0x7ffd1234ull, "direccion sin 0x");

  revisar(util::a_binario(42, 8) == "00101010", "a_binario");
  revisar(util::a_hex(42, 2) == "0x2a", "a_hex con relleno");
  revisar(util::normalizar("  Little  Endian! ") == "little endian", "normalizar");
  revisar(util::normalizar("Mont\xc3\xad" "culo") == "monticulo", "normalizar quita acentos");
}

void probar_verificadores() {
  const auto n = verif::numero(42);
  revisar(n("42") && n("0x2a") && n("0b101010"), "verif::numero acepta varias bases");
  revisar(!n("41") && !n("") && !n("hola"), "verif::numero rechaza lo incorrecto");

  const auto b = verif::bits(0x2a, 8);
  revisar(b("00101010") && b("0010 1010") && b("0x2a") && b("42"),
          "verif::bits acepta binario, hex y decimal");
  revisar(!b("00101011"), "verif::bits rechaza un bit cambiado");

  const auto d = verif::direccion(0x7ffdull);
  revisar(d("0x7ffd") && d("7FFD"), "verif::direccion ignora prefijo y mayusculas");
  revisar(!d("0x7ffe"), "verif::direccion rechaza otra direccion");

  const auto t = verif::texto({"little endian"});
  revisar(t("Little Endian") && t("  little   endian  "), "verif::texto normaliza");
  revisar(!t("big endian"), "verif::texto rechaza lo contrario");

  const auto o = verif::opcion('b', {"fuga de memoria"});
  revisar(o("b") && o("B)") && o("fuga de memoria"), "verif::opcion acepta letra y texto");
  revisar(!o("a"), "verif::opcion rechaza otra letra");
}

void probar_niveles() {
  const std::vector<unsigned> semillas = {1, 7, 42, 123, 2024, 31337};
  for (unsigned semilla : semillas) {
    const std::vector<Nivel> niveles = construir_niveles(semilla);
    revisar(niveles.size() == 6, "hay 6 niveles (semilla " + std::to_string(semilla) + ")");

    for (std::size_t i = 0; i < niveles.size(); ++i) {
      const Nivel& nivel = niveles[i];
      const std::string prefijo =
          "nivel " + std::to_string(i + 1) + " (" + nivel.nombre + ", semilla " +
          std::to_string(semilla) + ")";
      revisar(!nivel.nombre.empty() && !nivel.descripcion.empty(), prefijo + ": tiene textos");
      revisar(nivel.retos.size() >= 5, prefijo + ": tiene suficientes retos");

      for (const Reto& r : nivel.retos) {
        const std::string id = prefijo + " reto '" + r.titulo + "'";
        revisar(!r.titulo.empty(), id + ": titulo no vacio");
        revisar(!r.enunciado.empty(), id + ": enunciado no vacio");
        revisar(!r.respuesta.empty(), id + ": respuesta no vacia");
        revisar(r.puntos > 0, id + ": puntos positivos");
        revisar(static_cast<bool>(r.verificar), id + ": tiene verificador");
        if (r.taller) {
          revisar(static_cast<bool>(r.ejecutar_taller), id + ": el taller es ejecutable");
          continue;
        }
        revisar(!r.pista.empty(), id + ": tiene pista");
        revisar(!r.explicacion.empty(), id + ": tiene explicacion");
        // Lo esencial: la respuesta que el juego revela debe ser aceptada.
        revisar(r.verificar(r.respuesta), id + ": la respuesta canonica es valida");
        revisar(!r.verificar(""), id + ": no acepta una respuesta vacia");
      }
    }
  }
}

}  // namespace

int main() {
  std::cout << "Pruebas de MemLab\n";
  probar_util();
  probar_verificadores();
  probar_niveles();
  std::cout << (fallos == 0 ? "TODO BIEN: " : "CON FALLOS: ") << comprobaciones - fallos << "/"
            << comprobaciones << " comprobaciones correctas\n";
  return fallos == 0 ? 0 : 1;
}
