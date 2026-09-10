// MemLab - juego de terminal para entender memoria, punteros y bits en C++.
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "juego.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

void mostrar_uso() {
  std::cout << "MemLab - laboratorio de memoria, punteros y bits en C++\n\n"
            << "Uso: memlab [opciones]\n\n"
            << "  --nivel N        juega solo el nivel N (1 a 6)\n"
            << "  --semilla N      fija la semilla para repetir la misma partida\n"
            << "  --demo           recorre todos los retos mostrando las respuestas\n"
            << "  --lista          muestra los niveles disponibles y termina\n"
            << "  --sin-color      desactiva los colores ANSI\n"
            << "  --sin-guardado   no escribe el archivo .memlab_progreso\n"
            << "  --ayuda, -h      muestra esta ayuda\n";
}

void mostrar_lista() {
  const std::vector<Nivel> niveles = construir_niveles(1);
  ui::titulo("Niveles de MemLab");
  std::vector<std::string> lineas;
  for (std::size_t i = 0; i < niveles.size(); ++i) {
    lineas.push_back(ui::fuerte(std::to_string(i + 1) + ". " + niveles[i].nombre) + "  " +
                     ui::gris("(" + niveles[i].lema + ", " +
                              std::to_string(niveles[i].retos.size()) + " retos)"));
  }
  ui::caja("", lineas);
}

}  // namespace

int main(int argc, char** argv) {
  Opciones opciones;
  opciones.semilla = static_cast<unsigned>(
      std::chrono::steady_clock::now().time_since_epoch().count() & 0xFFFFu);
  ui::color_activo = isatty(fileno(stdout)) != 0;
  bool solo_lista = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto siguiente = [&](long long por_defecto) -> long long {
      if (i + 1 >= argc) return por_defecto;
      const auto v = util::parsear_entero(argv[++i]);
      return v.value_or(por_defecto);
    };

    if (arg == "--ayuda" || arg == "-h" || arg == "--help") { mostrar_uso(); return 0; }
    else if (arg == "--nivel") opciones.nivel_pedido = static_cast<int>(siguiente(0));
    else if (arg == "--semilla") opciones.semilla = static_cast<unsigned>(siguiente(0));
    else if (arg == "--demo") opciones.demo = true;
    else if (arg == "--sin-color") ui::color_activo = false;
    else if (arg == "--color") ui::color_activo = true;
    else if (arg == "--sin-guardado") opciones.guardar = false;
    else if (arg == "--lista") solo_lista = true;
    else {
      std::cerr << "Opcion desconocida: " << arg << "\n\n";
      mostrar_uso();
      return 1;
    }
  }

  if (solo_lista) { mostrar_lista(); return 0; }
  if (opciones.nivel_pedido < 0 || opciones.nivel_pedido > 6) {
    std::cerr << "El nivel debe estar entre 1 y 6.\n";
    return 1;
  }
  return jugar(opciones);
}
