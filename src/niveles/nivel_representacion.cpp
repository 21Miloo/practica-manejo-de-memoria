// Nivel 3: los mismos bits, distintos significados.
// Complemento a dos, desbordamiento modular y IEEE-754.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

struct Datos {
  float f = 0.0f;
  std::uint32_t bits_f = 0;
};

std::string sin_ceros(double x) {
  std::string s = std::to_string(x);
  while (s.size() > 1 && s.back() == '0') s.pop_back();
  if (!s.empty() && s.back() == '.') s.pop_back();
  return s;
}

}  // namespace

Nivel nivel_representacion(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "El significado de los bits";
  nivel.lema = "un patron, varias lecturas";
  nivel.descripcion =
      "En memoria no hay numeros con signo ni flotantes: solo hay bits. El tipo es la "
      "regla de lectura que tu eliges. Cambia el tipo y el mismo patron dice otra cosa.";

  const std::vector<float> candidatos = {1.5f, -2.25f, 0.125f, 3.5f, -0.75f, 6.0f, 0.375f};
  auto d = std::make_shared<Datos>();
  d->f = candidatos[static_cast<std::size_t>(
      util::entero_en_rango(azar, 0, static_cast<int>(candidatos.size()) - 1))];
  std::memcpy(&d->bits_f, &d->f, sizeof(d->bits_f));

  const int n = util::entero_en_rango(azar, 3, 60);
  const std::uint8_t patron = static_cast<std::uint8_t>(util::entero_en_rango(azar, 0x81, 0xFE));

  {
    Reto r;
    r.titulo = "Aritmetica que da la vuelta";
    r.escena = []() {
      ui::caja("unsigned char", {ui::gris("rango: 0 .. 255   (8 bits, sin signo)"),
                                 ui::gris("la aritmetica sin signo es modular: se calcula mod 256")});
    };
    r.enunciado = "unsigned char c = 255; c = c + 1;  Cuanto vale c?";
    r.respuesta = "0";
    r.verificar = verif::numero(0);
    r.pista = "255 es 1111 1111. Al sumar 1 el acarreo se sale del byte.";
    r.explicacion = "Los tipos sin signo dan la vuelta de forma definida (mod 2^N). No es un "
                    "error del lenguaje: esta garantizado por el estandar.";
    nivel.retos.push_back(r);
  }
  {
    const std::uint8_t complemento = static_cast<std::uint8_t>(-n);
    Reto r;
    r.titulo = "Complemento a dos";
    r.escena = [n]() {
      const std::uint8_t positivo = static_cast<std::uint8_t>(n);
      ui::caja("Como se escribe un negativo en 8 bits",
               {
                   " " + std::to_string(n) + " en binario:  " + ui::cian(util::a_binario(positivo, 8)),
                   ui::gris("paso 1: invierte todos los bits  -> ") +
                       util::a_binario(static_cast<std::uint8_t>(~positivo), 8),
                   ui::gris("paso 2: suma 1                   -> ") + ui::fuerte("?"),
               });
    };
    r.enunciado = "Escribe los 8 bits de int8_t x = -" + std::to_string(n) + ";";
    r.respuesta = util::a_binario(complemento, 8);
    r.verificar = verif::bits(complemento, 8);
    r.pista = "Invierte los bits de " + std::to_string(n) + " y suma 1. Deberia empezar por 1.";
    r.explicacion = "Con complemento a dos, x + (-x) = 0 usando el mismo sumador que los "
                    "positivos: por eso el hardware no necesita circuitos separados para restar.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Mismos bits, sin signo";
    r.escena = [patron]() {
      ui::caja("El byte " + util::a_hex(patron, 2),
               {ui::gris("indice del bit  ") + ui::regla_bits(8),
                ui::fuerte("valor           ") + ui::cian(ui::bits(patron, 8)),
                "",
                ui::gris("el bit 7 esta en 1: para un tipo con signo, ese es el bit de signo")});
    };
    r.enunciado = "Si esos bits se leen como unsigned char, que valor decimal representan?";
    r.respuesta = std::to_string(patron);
    r.verificar = verif::numero(patron);
    r.pista = "Suma los pesos de los bits encendidos, sin tratar el bit 7 de forma especial.";
    r.explicacion = "Sin signo, los 8 bits son magnitud pura: 0..255.";
    nivel.retos.push_back(r);
  }
  {
    const int con_signo = static_cast<int>(static_cast<std::int8_t>(patron));
    Reto r;
    r.titulo = "Mismos bits, con signo";
    r.escena = [patron]() {
      ui::caja("El byte " + util::a_hex(patron, 2),
               {ui::fuerte("bits  ") + ui::cian(ui::bits(patron, 8)),
                ui::gris("como int8_t, el bit 7 pesa -128 en vez de +128")});
    };
    r.enunciado = "Y si esos mismos bits se leen como int8_t, que valor decimal representan?";
    r.respuesta = std::to_string(con_signo);
    r.verificar = verif::numero(con_signo);
    r.pista = "Restale 256 al valor sin signo (" + std::to_string(patron) + ").";
    r.explicacion = "Ni un solo bit cambio en memoria: cambio el tipo con el que los lees. "
                    "Esto es exactamente lo que hace un reinterpret_cast.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Un caracter es un numero";
    r.enunciado = "char c = 'A';  Cuanto vale c como entero decimal (ASCII)?";
    r.respuesta = std::to_string(static_cast<int>('A'));
    r.verificar = verif::numero(static_cast<int>('A'));
    r.pista = "Las mayusculas empiezan en 0x41.";
    r.explicacion = "'A' es 65 (0x41) y 'a' es 97 (0x61): se diferencian en un solo bit, el 5. "
                    "Por eso 'A' | 0x20 == 'a'.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Un float por dentro";
    r.escena = [d]() {
      const std::uint32_t b = d->bits_f;
      const int signo = static_cast<int>(b >> 31);
      const int exp_sesgado = static_cast<int>((b >> 23) & 0xFFu);
      const std::uint32_t mantisa = b & 0x7FFFFFu;
      ui::caja("float f = " + sin_ceros(d->f) + ";  (IEEE-754 de 32 bits)",
               {
                   ui::gris("signo(1) exponente(8)  mantisa(23)"),
                   ui::fuerte(" " + std::to_string(signo) + "      " +
                              util::a_binario(static_cast<unsigned>(exp_sesgado), 8) + "  " +
                              util::a_binario(mantisa, 23)),
                   "",
                   ui::gris("exponente almacenado = ") + std::to_string(exp_sesgado) +
                       ui::gris("   (el real es este menos el sesgo 127)"),
                   "",
                   ui::gris("y asi se ven esos 4 bytes en memoria:"),
               });
      std::cout << ui::volcado(&d->f, sizeof(d->f), 8, 0, sizeof(d->f)) << "\n";
    };
    r.enunciado = "uint32_t b; memcpy(&b, &f, 4);  Que valor tiene b? (en hex)";
    r.respuesta = util::a_hex(d->bits_f, 8);
    r.verificar = verif::direccion(d->bits_f);
    r.pista = "Junta los 32 bits del desglose: signo, exponente y mantisa, en ese orden.";
    r.explicacion = "Un float no es 'otra cosa' que un entero: es el mismo tipo de patron de "
                    "bits con otra convencion de lectura. memcpy es la forma legal de mirarlo; "
                    "leerlo con *(uint32_t*)&f rompe las reglas de aliasing estricto.";
    nivel.retos.push_back(r);
  }
  {
    const int exp_real = static_cast<int>((d->bits_f >> 23) & 0xFFu) - 127;
    Reto r;
    r.titulo = "El exponente real";
    r.enunciado = "Cual es el exponente real de f (el almacenado menos el sesgo de 127)?";
    r.respuesta = std::to_string(exp_real);
    r.verificar = verif::numero(exp_real);
    r.pista = "El campo almacenado vale " + std::to_string((d->bits_f >> 23) & 0xFFu) + ".";
    r.explicacion = "El valor es (-1)^signo x 1.mantisa x 2^exponente. El sesgo permite "
                    "comparar floats positivos como si fueran enteros, byte a byte.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Precision limitada";
    r.escena = []() {
      ui::caja("Por que 0.1 + 0.2 != 0.3",
               {ui::gris("0.1 en binario es periodico: 0.0001100110011... y hay que cortarlo."),
                ui::gris("El float guarda 1 bit de signo, 8 de exponente y el resto de mantisa.")});
    };
    r.enunciado = "Cuantos bits de mantisa se almacenan en un float de 32 bits?";
    r.respuesta = "23";
    r.verificar = verif::numero(23);
    r.pista = "32 - 1 (signo) - 8 (exponente).";
    r.explicacion = "Son 23 almacenados y 24 efectivos: el 1 inicial de los normalizados esta "
                    "implicito y no se guarda. Ese redondeo es la causa de 0.1 + 0.2 != 0.3.";
    nivel.retos.push_back(r);
  }

  return nivel;
}
