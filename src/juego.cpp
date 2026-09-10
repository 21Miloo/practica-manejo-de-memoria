#include "juego.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <random>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace verif {

std::function<bool(const std::string&)> numero(long long esperado) {
  return [esperado](const std::string& entrada) {
    const auto v = util::parsear_entero(entrada);
    return v.has_value() && *v == esperado;
  };
}

std::function<bool(const std::string&)> bits(unsigned long long esperado, int nbits) {
  return [esperado, nbits](const std::string& entrada) {
    const std::string limpio = util::sin_separadores(util::recortar(entrada));
    // Una cadena de puros 0 y 1 se lee como binario, aunque parezca decimal.
    const bool solo_binario =
        !limpio.empty() &&
        std::all_of(limpio.begin(), limpio.end(), [](char c) { return c == '0' || c == '1'; });
    if (solo_binario) {
      const auto v = util::parsear_bits(limpio);
      return v.has_value() && *v == esperado;
    }
    const auto v = util::parsear_entero(entrada);
    if (!v.has_value()) return false;
    const unsigned long long mascara =
        nbits >= 64 ? ~0ull : ((1ull << nbits) - 1ull);
    return (static_cast<unsigned long long>(*v) & mascara) == (esperado & mascara);
  };
}

std::function<bool(const std::string&)> direccion(unsigned long long esperado) {
  return [esperado](const std::string& entrada) {
    const auto v = util::parsear_direccion(entrada);
    return v.has_value() && *v == esperado;
  };
}

std::function<bool(const std::string&)> texto(std::vector<std::string> aceptadas) {
  for (std::string& s : aceptadas) s = util::normalizar(s);
  return [aceptadas](const std::string& entrada) {
    const std::string e = util::normalizar(entrada);
    if (e.empty()) return false;
    return std::find(aceptadas.begin(), aceptadas.end(), e) != aceptadas.end();
  };
}

std::function<bool(const std::string&)> opcion(char letra, std::vector<std::string> sinonimos) {
  sinonimos.push_back(std::string(1, letra));
  sinonimos.push_back(std::string(1, letra) + ")");
  return texto(std::move(sinonimos));
}

}  // namespace verif

std::vector<Nivel> construir_niveles(unsigned semilla) {
  std::mt19937 azar(semilla);
  std::vector<Nivel> niveles;
  niveles.push_back(nivel_bits(azar));
  niveles.push_back(nivel_bytes(azar));
  niveles.push_back(nivel_representacion(azar));
  niveles.push_back(nivel_punteros(azar));
  niveles.push_back(nivel_contiguidad(azar));
  niveles.push_back(nivel_heap(azar));
  niveles.push_back(nivel_arreglos(azar));
  niveles.push_back(nivel_fugas(azar));
  return niveles;
}

namespace {

constexpr const char* kArchivoProgreso = ".memlab_progreso";

// En modo web, la pagina dibuja el progreso a partir de estas lineas.
void emitir_estado(int nivel, const std::string& nombre_nivel, int reto, int retos,
                   int puntos, int maximo) {
  if (ui::modo != ui::Modo::kWeb) return;
  std::cout << ui::kMarcaEstado << "{\"nivel\":" << nivel << ",\"nombreNivel\":\""
            << nombre_nivel << "\",\"reto\":" << reto << ",\"retos\":" << retos
            << ",\"puntos\":" << puntos << ",\"maximo\":" << maximo << "}\n";
}

// El enunciado se destaca: en la terminal con "? " y en la web con su propia
// marca, para que la pagina lo pinte como pregunta.
void mostrar_enunciado(const std::string& enunciado) {
  if (ui::modo == ui::Modo::kWeb) {
    std::cout << ui::kMarcaPregunta << enunciado << "\n";
    return;
  }
  std::cout << "\n" << ui::amarillo("? ");
  bool primera = true;
  for (const std::string& l : ui::envolver(enunciado, 70)) {
    std::cout << (primera ? "" : "  ") << l << "\n";
    primera = false;
  }
}

void mostrar_ayuda_comandos() {
  ui::caja("Comandos disponibles en cualquier momento",
           {
               ui::fuerte("pista") + "   muestra una ayuda (cuesta la mitad de los puntos)",
               ui::fuerte("saltar") + "  pasa al siguiente reto y revela la respuesta",
               ui::fuerte("mapa") + "    vuelve a dibujar la escena del reto",
               ui::fuerte("ayuda") + "   muestra esta lista",
               ui::fuerte("salir") + "   termina la partida y muestra el resumen",
               "",
               ui::gris("Formatos aceptados: 42 (decimal), 0x2a (hex), 0b101010 (binario)."),
               ui::gris("Si la respuesta es un patron de bits puedes escribir 0010 1010."),
           });
}

void mostrar_resumen(int puntos, int maximo, const std::map<std::string, int>& por_nivel) {
  ui::titulo("Resumen de la partida");
  std::vector<std::string> lineas;
  for (const auto& par : por_nivel) {
    lineas.push_back(par.first + ": " + ui::fuerte(std::to_string(par.second)) + " puntos");
  }
  lineas.push_back("");
  const int porcentaje = maximo > 0 ? (puntos * 100) / maximo : 0;
  lineas.push_back("Total: " + ui::fuerte(std::to_string(puntos) + " / " + std::to_string(maximo)) +
                   "  (" + std::to_string(porcentaje) + "%)");
  std::string rango;
  if (porcentaje >= 90) rango = ui::verde("Arquitecto de memoria");
  else if (porcentaje >= 70) rango = ui::verde("Domador de punteros");
  else if (porcentaje >= 50) rango = ui::amarillo("Aprendiz de bits");
  else rango = ui::rojo("Todavia hay fugas: vuelve a intentarlo");
  lineas.push_back("Rango: " + rango);
  ui::caja("", lineas);
}

void guardar_progreso(int puntos, int maximo) {
  std::ofstream archivo(kArchivoProgreso, std::ios::app);
  if (!archivo) return;
  archivo << puntos << " / " << maximo << "\n";
}

// Lee una linea del jugador. Devuelve false si se acabo la entrada.
bool leer_linea(std::string& destino) {
  if (!ui::leer_linea("memlab> ", destino)) return false;
  destino = util::recortar(destino);
  return true;
}

enum class Resultado { kAcierto, kSaltado, kSalir };

Resultado jugar_reto(const Reto& reto, int numero_reto, int total_retos, int& puntos_ganados,
                     bool demo) {
  ui::separador();
  std::cout << ui::fuerte("Reto " + std::to_string(numero_reto) + "/" +
                          std::to_string(total_retos) + ": " + reto.titulo)
            << "  " << ui::gris("(" + std::to_string(reto.puntos) + " puntos)") << "\n\n";
  if (reto.escena) reto.escena();
  mostrar_enunciado(reto.enunciado);

  if (reto.taller) {
    puntos_ganados = reto.ejecutar_taller(demo);
    return Resultado::kAcierto;
  }

  if (demo) {
    std::cout << ui::cian("memlab> ") << reto.respuesta << ui::gris("   (modo demostracion)")
              << "\n";
    const bool ok = reto.verificar(reto.respuesta);
    std::cout << (ok ? ui::verde("  Correcto.")
                     : ui::rojo("  ERROR: la respuesta canonica no pasa la verificacion."))
              << "\n";
    if (!reto.explicacion.empty()) ui::parrafo(reto.explicacion, "  ", true);
    puntos_ganados = ok ? reto.puntos : 0;
    return ok ? Resultado::kAcierto : Resultado::kSalir;
  }

  int intentos = 0;
  bool uso_pista = false;
  std::string entrada;
  while (true) {
    if (!leer_linea(entrada)) return Resultado::kSalir;
    const std::string comando = util::normalizar(entrada);

    if (comando == "salir" || comando == "quit" || comando == "exit") return Resultado::kSalir;
    if (comando == "ayuda" || comando == "help") { mostrar_ayuda_comandos(); continue; }
    if (comando == "mapa") {
      if (reto.escena) reto.escena();
      mostrar_enunciado(reto.enunciado);
      continue;
    }
    if (comando == "pista") {
      uso_pista = true;
      std::cout << ui::amarillo("  Pista: ") << "\n";
      ui::parrafo(reto.pista, "    ");
      continue;
    }
    if (comando == "saltar" || comando == "skip") {
      std::cout << ui::gris("  Respuesta: ") << ui::fuerte(reto.respuesta) << "\n";
      if (!reto.explicacion.empty()) ui::parrafo(reto.explicacion, "  ", true);
      puntos_ganados = 0;
      return Resultado::kSaltado;
    }
    if (entrada.empty()) continue;

    ++intentos;
    if (reto.verificar(entrada)) {
      int ganados = reto.puntos;
      if (uso_pista) ganados /= 2;
      if (intentos >= 3) ganados = std::max(1, ganados / 2);
      puntos_ganados = ganados;
      std::cout << ui::verde("  Correcto. ") << ui::gris("+" + std::to_string(ganados) + " puntos")
                << "\n";
      if (!reto.explicacion.empty()) ui::parrafo(reto.explicacion, "  ", true);
      return Resultado::kAcierto;
    }

    std::cout << ui::rojo("  No es eso. ");
    if (intentos == 1) std::cout << ui::gris("Escribe 'pista' si quieres una ayuda.");
    else if (intentos == 2) std::cout << ui::gris("Escribe 'saltar' para ver la respuesta.");
    else std::cout << ui::gris("Se aceptan decimal (42), hex (0x2a) y binario (0b101010).");
    std::cout << "\n";
  }
}

}  // namespace

int jugar(const Opciones& opciones) {
  std::vector<Nivel> niveles = construir_niveles(opciones.semilla);

  ui::titulo("MemLab - laboratorio de memoria, punteros y bits en C++");
  ui::caja("Como funciona",
           {
               "Cada nivel inspecciona memoria " + ui::fuerte("real") + " de este proceso:",
               "las direcciones, los volcados hexadecimales y los tamanos que veras",
               "salen de variables vivas, no de ejemplos inventados.",
               "",
               ui::gris("Semilla de la partida: " + std::to_string(opciones.semilla) +
                        "  (usa --semilla N para repetir la misma partida)"),
           });
  if (!opciones.demo) mostrar_ayuda_comandos();

  int puntos = 0;
  int maximo = 0;
  std::map<std::string, int> por_nivel;
  bool salir = false;

  // Puntaje maximo de la partida completa: la pagina lo necesita desde el
  // primer reto para dibujar la barra de progreso.
  int maximo_total = 0;
  for (std::size_t i = 0; i < niveles.size(); ++i) {
    if (opciones.nivel_pedido != 0 && opciones.nivel_pedido != static_cast<int>(i) + 1) continue;
    for (const Reto& r : niveles[i].retos) maximo_total += r.puntos;
  }

  for (std::size_t i = 0; i < niveles.size() && !salir; ++i) {
    const Nivel& nivel = niveles[i];
    const int numero_nivel = static_cast<int>(i) + 1;
    if (opciones.nivel_pedido != 0 && opciones.nivel_pedido != numero_nivel) continue;

    ui::titulo("Nivel " + std::to_string(numero_nivel) + " - " + nivel.nombre + "  |  " +
               nivel.lema);
    ui::caja("", {nivel.descripcion});

    int puntos_nivel = 0;
    for (std::size_t j = 0; j < nivel.retos.size(); ++j) {
      const Reto& reto = nivel.retos[j];
      maximo += reto.puntos;
      emitir_estado(numero_nivel, nivel.nombre, static_cast<int>(j) + 1,
                    static_cast<int>(nivel.retos.size()), puntos, maximo_total);
      int ganados = 0;
      const Resultado r = jugar_reto(reto, static_cast<int>(j) + 1,
                                     static_cast<int>(nivel.retos.size()), ganados, opciones.demo);
      puntos += ganados;
      puntos_nivel += ganados;
      emitir_estado(numero_nivel, nivel.nombre, static_cast<int>(j) + 1,
                    static_cast<int>(nivel.retos.size()), puntos, maximo_total);
      if (r == Resultado::kSalir) { salir = true; break; }
    }
    por_nivel["Nivel " + std::to_string(numero_nivel) + " - " + nivel.nombre] = puntos_nivel;
  }

  mostrar_resumen(puntos, maximo, por_nivel);
  if (opciones.guardar && !opciones.demo) guardar_progreso(puntos, maximo);
  return 0;
}
