// Nivel 2: el byte, el volcado hexadecimal y el orden de los bytes (endianness).
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

bool es_little_endian() {
  const std::uint32_t sonda = 0x01020304u;
  std::uint8_t primero = 0;
  std::memcpy(&primero, &sonda, 1);
  return primero == 0x04;
}

// Memoria viva del nivel: las escenas leen estos bytes reales. Se guarda en un
// shared_ptr para que siga existiendo mientras exista el Nivel.
struct Datos {
  std::uint32_t v = 0;
  std::uint16_t corto = 0;
  unsigned char crudo[4] = {0, 0, 0, 0};
};

}  // namespace

Nivel nivel_bytes(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "Bytes en fila";
  nivel.lema = "que ve la maquina";
  nivel.descripcion =
      "La memoria es un arreglo gigante de bytes numerados. Un valor de varios bytes "
      "se reparte entre direcciones consecutivas, y el orden en que se reparte importa.";

  std::uniform_int_distribution<unsigned> rango(0x10000000u, 0x7EFFFFFFu);
  auto d = std::make_shared<Datos>();
  d->v = rango(azar);
  d->corto = static_cast<std::uint16_t>(rango(azar) & 0xFFFFu);
  for (int i = 0; i < 4; ++i) d->crudo[i] = static_cast<unsigned char>(rango(azar) & 0xFFu);

  const bool little = es_little_endian();
  std::uint8_t primer_byte = 0;
  std::memcpy(&primer_byte, &d->v, 1);

  const auto escena_v = [d]() {
    ui::caja("uint32_t v = " + util::a_hex(d->v, 8) + ";",
             {
                 ui::gris("sizeof(v) = ") + std::to_string(sizeof(d->v)) + ui::gris(" bytes"),
                 "",
                 ui::gris("volcado de la memoria donde vive v:"),
             });
    std::cout << ui::volcado(&d->v, sizeof(d->v), 8, 0, sizeof(d->v)) << "\n";
  };

  {
    Reto r;
    r.titulo = "Tamano en bytes";
    r.escena = escena_v;
    r.enunciado = "Cuantos bytes ocupa un uint32_t?";
    r.respuesta = std::to_string(sizeof(std::uint32_t));
    r.verificar = verif::numero(static_cast<long long>(sizeof(std::uint32_t)));
    r.pista = "El 32 del nombre son bits. Un byte son 8 bits.";
    r.explicacion = "sizeof siempre se mide en bytes (mas exactamente, en unidades de char, "
                    "y sizeof(char) es 1 por definicion).";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "El primer byte";
    r.escena = escena_v;
    r.enunciado =
        "Mira el volcado: cual es el byte que esta en la direccion mas baja de v? (en hex)";
    r.respuesta = util::a_hex(primer_byte, 2);
    r.verificar = verif::bits(primer_byte, 8);
    r.pista = "Es el primero de la fila del volcado. Comparalo con " + util::a_hex(d->v, 8) + ".";
    r.explicacion = std::string("Esta maquina es ") + (little ? "little endian" : "big endian") +
                    ": guarda primero el byte " + (little ? "menos" : "mas") + " significativo.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Poner nombre al orden";
    r.escena = escena_v;
    r.enunciado = "Como se llama ese orden de bytes? Escribe 'little endian' o 'big endian'.";
    r.respuesta = little ? "little endian" : "big endian";
    r.verificar = little ? verif::texto({"little endian", "little", "littleendian"})
                         : verif::texto({"big endian", "big", "bigendian"});
    r.pista = "Si el byte menos significativo va primero, el extremo 'pequeno' va primero.";
    r.explicacion = "x86 y ARM (en su modo habitual) son little endian; las redes usan big "
                    "endian, por eso existen htons/htonl.";
    nivel.retos.push_back(r);
  }
  {
    std::uint32_t reconstruido = 0;
    std::memcpy(&reconstruido, d->crudo, sizeof(reconstruido));
    Reto r;
    r.titulo = "Reconstruir un valor";
    r.escena = [d]() {
      ui::caja("unsigned char crudo[4];  // asi se ven estos 4 bytes en memoria",
               {ui::gris("bytes, de la direccion mas baja a la mas alta:"),
                ui::fuerte(util::a_hex(d->crudo[0], 2) + "  " + util::a_hex(d->crudo[1], 2) +
                           "  " + util::a_hex(d->crudo[2], 2) + "  " +
                           util::a_hex(d->crudo[3], 2))});
      std::cout << ui::volcado(d->crudo, sizeof(d->crudo), 8, 0, sizeof(d->crudo)) << "\n";
    };
    r.enunciado = "uint32_t x; memcpy(&x, crudo, 4);  Que valor tiene x? (en hex)";
    r.respuesta = util::a_hex(reconstruido, 8);
    r.verificar = verif::direccion(reconstruido);
    r.pista = little ? "En little endian el ultimo byte del volcado es el mas significativo: "
                       "leelos al reves."
                     : "En big endian los bytes se leen en el mismo orden en que aparecen.";
    r.explicacion = "memcpy copia bytes tal cual; la interpretacion la pone el tipo destino.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Espiar con un puntero a byte";
    r.escena = escena_v;
    r.enunciado = "Que imprime  *reinterpret_cast<unsigned char*>(&v)  ? (en hex)";
    r.respuesta = util::a_hex(primer_byte, 2);
    r.verificar = verif::bits(primer_byte, 8);
    r.pista = "Un unsigned char* ve la memoria byte a byte, empezando por la direccion de v.";
    r.explicacion = "unsigned char* es el unico puntero con el que el estandar te permite "
                    "inspeccionar la representacion de cualquier objeto.";
    nivel.retos.push_back(r);
  }
  {
    const std::uint8_t bajo = static_cast<std::uint8_t>(d->corto & 0xFF);
    const std::uint8_t alto = static_cast<std::uint8_t>(d->corto >> 8);
    const std::uint8_t esperado = little ? alto : bajo;
    Reto r;
    r.titulo = "Dos bytes, dos direcciones";
    r.escena = [d]() {
      ui::caja("uint16_t corto = " + util::a_hex(d->corto, 4) + ";",
               {ui::gris("sizeof(corto) = ") + std::to_string(sizeof(d->corto)) +
                ui::gris(" bytes")});
      std::cout << ui::volcado(&d->corto, sizeof(d->corto), 8, 0, sizeof(d->corto)) << "\n";
    };
    r.enunciado = "Cual es el segundo byte de corto en memoria (el de la direccion mas alta)?";
    r.respuesta = util::a_hex(esperado, 2);
    r.verificar = verif::bits(esperado, 8);
    r.pista = "El valor se parte en " + util::a_hex(alto, 2) + " (parte alta) y " +
              util::a_hex(bajo, 2) + " (parte baja).";
    r.explicacion = "El mismo criterio de orden aplica a cualquier tipo de varios bytes.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "La unidad de medida";
    r.enunciado = "Cuanto vale sizeof(char) en C++, en cualquier plataforma conforme?";
    r.respuesta = "1";
    r.verificar = verif::numero(1);
    r.pista = "Es la definicion misma de la unidad con la que sizeof mide.";
    r.explicacion = "sizeof(char) == 1 por definicion. Lo que no esta fijado es cuantos bits "
                    "tiene un char: CHAR_BIT es 8 en la practica, pero el estandar solo exige >= 8.";
    nivel.retos.push_back(r);
  }

  return nivel;
}
