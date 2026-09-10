// Nivel 1: el bit como unidad. Mascaras, desplazamientos y operadores logicos.
#include <bitset>
#include <cstdint>
#include <iostream>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

// Dibuja un byte con la regla de indices y los pesos de cada posicion.
void dibujar_byte(const std::string& etiqueta, std::uint8_t b) {
  ui::caja(etiqueta + " = " + util::a_hex(b, 2) + " = " + std::to_string(b) + " (decimal)",
           {
               ui::gris("indice del bit  ") + ui::regla_bits(8),
               ui::fuerte("valor           ") + ui::cian(ui::bits(b, 8)),
               ui::gris("peso            128  64  32  16   8   4   2   1"),
           });
}

}  // namespace

Nivel nivel_bits(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "Bit a bit";
  nivel.lema = "la unidad mas pequena";
  nivel.descripcion =
      "Un byte son 8 interruptores. Aqui aprendes a leerlos y a manipularlos con "
      "mascaras, desplazamientos y operadores logicos.";

  std::uniform_int_distribution<int> rango_byte(0x21, 0xFE);
  std::uniform_int_distribution<int> rango_bit(0, 7);

  const std::uint8_t b = static_cast<std::uint8_t>(rango_byte(azar));
  const int k = rango_bit(azar);

  // Para que los retos se vean: 'apagar' usa un bit que este en 1 y 'encender'
  // uno que este en 0, asi el resultado siempre cambia respecto de b.
  const auto buscar_bit = [b](int desde, int valor_buscado) {
    for (int i = 0; i < 8; ++i) {
      const int candidato = (desde + i) % 8;
      if (((b >> candidato) & 1) == valor_buscado) return candidato;
    }
    return desde;
  };
  const int bit_apagado = buscar_bit(k, 0);  // candidato para encender
  const int bit_encendido = buscar_bit(k, 1);  // candidato para apagar
  const std::uint8_t mascara = static_cast<std::uint8_t>(rango_byte(azar));

  const auto escena = [b]() { dibujar_byte("byte b", b); };

  {
    Reto r;
    r.titulo = "Leer un byte";
    r.escena = escena;
    r.enunciado = "Escribe los 8 bits de b (del bit 7 al bit 0).";
    r.respuesta = util::a_binario(b, 8);
    r.verificar = verif::bits(b, 8);
    r.pista = "Divide en dos mitades: " + util::a_hex(b >> 4, 1) + " y " +
              util::a_hex(b & 0x0F, 1) + ". Cada digito hexadecimal son 4 bits.";
    r.explicacion = "Cada digito hex es exactamente un nibble (4 bits); por eso el hex "
                    "es tan comodo para leer memoria.";
    nivel.retos.push_back(r);
  }
  {
    const int valor = (b >> k) & 1;
    Reto r;
    r.titulo = "Consultar un bit";
    r.escena = escena;
    r.enunciado = "En C++, (b >> " + std::to_string(k) + ") & 1 vale ... (0 o 1)";
    r.respuesta = std::to_string(valor);
    r.verificar = verif::numero(valor);
    r.pista = "Desplaza el bit " + std::to_string(k) + " hasta la posicion 0 y quedate con el.";
    r.explicacion = "Este es el patron estandar para leer una bandera: desplazar y enmascarar.";
    nivel.retos.push_back(r);
  }
  {
    const std::uint8_t resultado = static_cast<std::uint8_t>(b | (1u << bit_apagado));
    Reto r;
    r.titulo = "Encender un bit";
    r.escena = escena;
    r.enunciado =
        "Cuanto vale b | (1 << " + std::to_string(bit_apagado) + ")? Responde en hex o binario.";
    r.respuesta = util::a_hex(resultado, 2);
    r.verificar = verif::bits(resultado, 8);
    r.pista = "1 << " + std::to_string(bit_apagado) + " vale " +
              util::a_hex(1u << bit_apagado, 2) +
              ". El OR deja en 1 todo bit que ya estuviera en 1.";
    r.explicacion = "OR con una mascara de un solo bit = encender esa bandera sin tocar el resto.";
    nivel.retos.push_back(r);
  }
  {
    const std::uint8_t resultado = static_cast<std::uint8_t>(b & ~(1u << bit_encendido));
    Reto r;
    r.titulo = "Apagar un bit";
    r.escena = escena;
    r.enunciado = "Cuanto vale b & ~(1 << " + std::to_string(bit_encendido) + ")?";
    r.respuesta = util::a_hex(resultado, 2);
    r.verificar = verif::bits(resultado, 8);
    r.pista = "~(1 << " + std::to_string(bit_encendido) + ") es " +
              util::a_binario(static_cast<std::uint8_t>(~(1u << bit_encendido)), 8) +
              ": unos en todas partes menos en el bit " + std::to_string(bit_encendido) + ".";
    r.explicacion = "AND con la mascara negada = apagar esa bandera. Ojo: ~ opera sobre int, "
                    "por eso conviene truncar el resultado al tipo correcto.";
    nivel.retos.push_back(r);
  }
  {
    const std::uint8_t resultado = static_cast<std::uint8_t>(b << 1);
    Reto r;
    r.titulo = "Desplazar y perder";
    r.escena = escena;
    r.enunciado = "uint8_t c = b << 1;  Cuanto vale c? (cuidado: solo caben 8 bits)";
    r.respuesta = util::a_hex(resultado, 2);
    r.verificar = verif::bits(resultado, 8);
    r.pista = "Desplazar a la izquierda multiplica por 2, pero el bit 7 se sale del byte y "
              "se pierde al guardar en un uint8_t.";
    r.explicacion = std::string("El bit que se sale desaparece: ") +
                    (((b >> 7) & 1) ? "aqui perdiste un 1." : "aqui el bit 7 era 0, no perdiste nada.");
    nivel.retos.push_back(r);
  }
  {
    const std::uint8_t resultado = static_cast<std::uint8_t>(b ^ mascara);
    Reto r;
    r.titulo = "XOR: invertir a voluntad";
    r.escena = escena;
    r.enunciado = "Con m = " + util::a_hex(mascara, 2) + " (" + util::a_binario(mascara, 8) +
                  "), cuanto vale b ^ m?";
    r.respuesta = util::a_hex(resultado, 2);
    r.verificar = verif::bits(resultado, 8);
    r.pista = "XOR da 1 solo cuando los bits son distintos. Un 1 en la mascara invierte ese bit.";
    r.explicacion = "Por eso (b ^ m) ^ m vuelve a ser b: XOR es su propia inversa.";
    nivel.retos.push_back(r);
  }
  {
    const int cuantos = static_cast<int>(std::bitset<8>(b).count());
    Reto r;
    r.titulo = "Contar unos";
    r.escena = escena;
    r.enunciado = "Cuantos bits de b estan en 1?";
    r.respuesta = std::to_string(cuantos);
    r.verificar = verif::numero(cuantos);
    r.pista = "Cuentalos en " + util::a_binario(b, 8) + ".";
    r.explicacion = "En C++20 esto es std::popcount(b); antes, std::bitset<8>(b).count().";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Disenar una mascara";
    r.escena = escena;
    r.enunciado = "Que mascara de 8 bits conserva solo el nibble bajo (los 4 bits de la derecha)?";
    r.respuesta = "0x0f";
    r.verificar = verif::bits(0x0F, 8);
    r.pista = "Necesitas 0000 1111.";
    r.explicacion = "b & 0x0F = " + util::a_hex(b & 0x0F, 2) +
                    ". Extraer campos con mascaras es la base de los formatos binarios.";
    nivel.retos.push_back(r);
  }

  return nivel;
}
