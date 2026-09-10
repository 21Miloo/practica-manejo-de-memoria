// Punto de entrada de la version web: el mismo motor del juego, compilado a
// WebAssembly. Aqui solo se conecta la entrada y la salida con el navegador.
#include <emscripten.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include "juego.hpp"
#include "ui.hpp"

// Pide una linea al navegador. La corrutina de Asyncify deja al programa en
// pausa mientras el jugador escribe, sin bloquear la pagina.
EM_ASYNC_JS(char*, pedir_linea_al_navegador, (const char* prompt), {
  const texto = await Module.pedirLinea(UTF8ToString(prompt));
  const largo = lengthBytesUTF8(texto) + 1;
  const destino = _malloc(largo);
  stringToUTF8(texto, destino, largo);
  return destino;
});

namespace {

bool leer_del_navegador(const std::string& prompt, std::string& destino) {
  // Se vacia la salida pendiente antes de ceder el control: asi la pagina ya
  // tiene pintado todo el reto cuando aparece el campo de entrada.
  std::cout << std::flush;
  char* crudo = pedir_linea_al_navegador(prompt.c_str());
  if (crudo == nullptr) return false;
  destino = crudo;
  std::free(crudo);
  return true;
}

}  // namespace

// La pagina llama a esta funcion para empezar una partida.
extern "C" EMSCRIPTEN_KEEPALIVE void memlab_jugar(unsigned semilla, int nivel) {
  ui::modo = ui::Modo::kWeb;
  ui::lector_entrada = leer_del_navegador;

  Opciones opciones;
  opciones.semilla = semilla;
  opciones.nivel_pedido = nivel;
  opciones.demo = false;
  opciones.guardar = false;  // en el navegador no hay archivo donde guardar
  jugar(opciones);
  // Avisa a la pagina de que ya no se va a pedir mas entrada.
  std::cout << ui::kMarcaFinPartida << "\n" << std::flush;
}

// Cuantos niveles tiene el juego, para que la pagina arme su indice sin
// duplicar la lista.
extern "C" EMSCRIPTEN_KEEPALIVE int memlab_total_niveles() {
  return static_cast<int>(construir_niveles(1).size());
}

int main() {
  // El modulo se queda cargado esperando a que la pagina llame a memlab_jugar.
  return 0;
}
