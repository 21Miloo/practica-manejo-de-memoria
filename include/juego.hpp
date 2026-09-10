// juego.hpp - modelo del juego: retos, niveles y bucle principal.
#pragma once

#include <functional>
#include <string>
#include <vector>

// Un reto es una pregunta con verificacion propia. La respuesta canonica se
// guarda aparte para el modo demostracion y para las pruebas automaticas.
struct Reto {
  std::string titulo;
  std::string enunciado;
  std::function<void()> escena;  // dibujo opcional antes de preguntar
  std::string respuesta;         // respuesta canonica (siempre valida)
  std::function<bool(const std::string&)> verificar;
  std::string pista;
  std::string explicacion;
  int puntos = 10;

  // Los talleres son retos interactivos con su propio bucle de comandos.
  bool taller = false;
  std::function<int(bool /*demo*/)> ejecutar_taller;
};

struct Nivel {
  std::string nombre;
  std::string lema;
  std::string descripcion;
  std::vector<Reto> retos;
};

// --- Constructores de verificadores -------------------------------------
namespace verif {

// Acepta decimal, 0x..., 0b... y numeros negativos.
std::function<bool(const std::string&)> numero(long long esperado);

// Acepta una cadena de 0 y 1 (con o sin espacios), 0b..., 0x... o decimal.
std::function<bool(const std::string&)> bits(unsigned long long esperado, int nbits);

// Acepta hexadecimal con o sin prefijo 0x.
std::function<bool(const std::string&)> direccion(unsigned long long esperado);

// Acepta cualquiera de las variantes de texto (comparacion normalizada).
std::function<bool(const std::string&)> texto(std::vector<std::string> aceptadas);

// Opcion multiple: la letra correcta o el texto de la opcion.
std::function<bool(const std::string&)> opcion(char letra, std::vector<std::string> sinonimos = {});

}  // namespace verif

// --- Motor ---------------------------------------------------------------
struct Opciones {
  unsigned semilla = 0;
  int nivel_pedido = 0;  // 0 = todos
  bool demo = false;
  bool guardar = true;
};

std::vector<Nivel> construir_niveles(unsigned semilla);
int jugar(const Opciones& opciones);
