// Nivel 6: memoria dinamica. Pila contra monticulo, fugas, punteros colgantes
// y un taller interactivo con un heap simulado.
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

std::uintptr_t entero(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }

// Distancia entre dos direcciones, en la unidad que se lea mejor.
std::string distancia_legible(std::uintptr_t a, std::uintptr_t b) {
  const unsigned long long bytes = a > b ? a - b : b - a;
  const unsigned long long gb = bytes / (1024ull * 1024ull * 1024ull);
  if (gb >= 1024ull) return std::to_string(gb / 1024ull) + " TB";
  if (gb >= 1ull) return std::to_string(gb) + " GB";
  return std::to_string(bytes / (1024ull * 1024ull)) + " MB";
}

// --- Taller: un heap de mentira, con reglas de verdad ---------------------

struct Bloque {
  int id = 0;
  std::uintptr_t direccion = 0;
  int elementos = 0;
  bool activo = false;
  std::vector<int> datos;
};

class TallerHeap {
 public:
  int ejecutar(bool demo) {
    mostrar_mision();
    if (demo) return correr_demostracion();
    return bucle_interactivo();
  }

 private:
  std::vector<Bloque> bloques_;
  std::uintptr_t proxima_direccion_ = 0x55a3c0001000ull;
  int siguiente_id_ = 1;
  int errores_ = 0;
  bool reservo_cuatro_ = false;
  bool escribio_todo_ = false;
  bool libero_todo_ = false;

  void mostrar_mision() {
    ui::caja("Taller final: administra el monticulo tu mismo",
             {
                 "Mision:",
                 "  1. reserva un bloque de 4 enteros",
                 "  2. escribe en el los valores 10, 20, 30 y 40",
                 "  3. liberalo",
                 "  4. termina sin fugas y sin accesos invalidos",
                 "",
                 ui::fuerte("Comandos:"),
                 "  reservar N          equivale a  int* p = new int[N];",
                 "  escribir ID I VAL   equivale a  p[I] = VAL;",
                 "  leer ID I           equivale a  std::cout << p[I];",
                 "  liberar ID          equivale a  delete[] p;",
                 "  mapa                dibuja el estado del monticulo",
                 "  listo               termina el taller y lo evalua",
             });
  }

  Bloque* buscar(int id) {
    for (Bloque& b : bloques_)
      if (b.id == id) return &b;
    return nullptr;
  }

  void reservar(int n) {
    if (n <= 0 || n > 16) {
      std::cout << ui::rojo("  Reserva entre 1 y 16 enteros.") << "\n";
      return;
    }
    // Un asignador real reutiliza huecos liberados: aqui hacemos lo mismo, y de
    // paso se ve por que un puntero colgante a veces 'parece' funcionar.
    for (Bloque& b : bloques_) {
      if (!b.activo && b.elementos == n) {
        b.activo = true;
        b.id = siguiente_id_++;
        std::cout << ui::verde("  Reservados " + std::to_string(n * static_cast<int>(sizeof(int))) +
                               " bytes en " + ui::dir(b.direccion) + " -> bloque #" +
                               std::to_string(b.id))
                  << "\n"
                  << ui::gris("  (el asignador reutilizo un hueco liberado: es la misma "
                              "direccion de antes)")
                  << "\n";
        if (n == 4) reservo_cuatro_ = true;
        return;
      }
    }
    Bloque b;
    b.id = siguiente_id_++;
    b.elementos = n;
    b.activo = true;
    b.datos.assign(static_cast<std::size_t>(n), 0);
    b.direccion = proxima_direccion_;
    proxima_direccion_ += static_cast<std::uintptr_t>(n) * sizeof(int) + 16;  // + cabecera
    bloques_.push_back(b);
    std::cout << ui::verde("  Reservados " + std::to_string(n * static_cast<int>(sizeof(int))) +
                           " bytes en " + ui::dir(b.direccion) + " -> bloque #" +
                           std::to_string(b.id))
              << "\n";
    if (n == 4) reservo_cuatro_ = true;
  }

  void escribir(int id, int indice, int valor) {
    Bloque* b = buscar(id);
    if (b == nullptr) {
      std::cout << ui::rojo("  No existe el bloque #" + std::to_string(id)) << "\n";
      return;
    }
    if (!b->activo) {
      ++errores_;
      std::cout << ui::rojo("  USO DESPUES DE LIBERAR (use after free).") << "\n";
      ui::parrafo("El bloque #" + std::to_string(id) +
                      " ya se libero: el puntero quedo colgante. Escribir ahi es comportamiento "
                      "indefinido; podrias estar pisando memoria que el asignador ya entrego a "
                      "otra parte del programa.",
                  "  ", true);
      return;
    }
    if (indice < 0 || indice >= b->elementos) {
      ++errores_;
      std::cout << ui::rojo("  FUERA DE RANGO.") << "\n";
      ui::parrafo(
          "El bloque #" + std::to_string(id) + " tiene indices 0.." +
              std::to_string(b->elementos - 1) + ". Escribir en [" + std::to_string(indice) +
              "] toca la direccion " +
              ui::dir(b->direccion + static_cast<std::uintptr_t>(indice) * sizeof(int)) +
              ", fuera del bloque. C++ no comprueba limites: nadie te avisa, y el error "
              "aparece cuando se corrompe lo que hubiera al lado.",
          "  ", true);
      return;
    }
    b->datos[static_cast<std::size_t>(indice)] = valor;
    std::cout << ui::gris("  bloque #" + std::to_string(id) + "[" + std::to_string(indice) +
                          "] = " + std::to_string(valor) + "   (direccion " +
                          ui::dir(b->direccion + static_cast<std::uintptr_t>(indice) * sizeof(int)) +
                          ")")
              << "\n";
    revisar_escritura(*b);
  }

  void revisar_escritura(const Bloque& b) {
    if (b.elementos != 4) return;
    const std::vector<int> objetivo = {10, 20, 30, 40};
    if (b.datos == objetivo) {
      escribio_todo_ = true;
      std::cout << ui::verde("  Objetivo cumplido: el bloque contiene 10, 20, 30, 40.") << "\n";
    }
  }

  void leer(int id, int indice) {
    Bloque* b = buscar(id);
    if (b == nullptr) {
      std::cout << ui::rojo("  No existe el bloque #" + std::to_string(id)) << "\n";
      return;
    }
    if (!b->activo) {
      ++errores_;
      std::cout << ui::rojo("  USO DESPUES DE LIBERAR (lectura).") << "\n";
      ui::parrafo("Aqui devolveria " +
                      std::to_string(indice >= 0 && indice < b->elementos
                                         ? b->datos[static_cast<std::size_t>(indice)]
                                         : 0) +
                      ", el valor viejo que sigue en esos bytes. Que 'funcione' es justo lo "
                      "peligroso: manana el asignador reutiliza el hueco y devuelve basura.",
                  "  ", true);
      return;
    }
    if (indice < 0 || indice >= b->elementos) {
      ++errores_;
      std::cout << ui::rojo("  FUERA DE RANGO en la lectura.") << "\n";
      return;
    }
    std::cout << ui::cian("  " + std::to_string(b->datos[static_cast<std::size_t>(indice)])) << "\n";
  }

  void liberar(int id) {
    Bloque* b = buscar(id);
    if (b == nullptr) {
      std::cout << ui::rojo("  No existe el bloque #" + std::to_string(id)) << "\n";
      return;
    }
    if (!b->activo) {
      ++errores_;
      std::cout << ui::rojo("  DOBLE LIBERACION (double free).") << "\n";
      ui::parrafo("Liberar dos veces corrompe las estructuras internas del asignador y suele "
                  "terminar en un aborto. Costumbre util: despues de delete, p = nullptr.",
                  "  ", true);
      return;
    }
    b->activo = false;
    std::cout << ui::verde("  Liberado el bloque #" + std::to_string(id) + " (" +
                           ui::dir(b->direccion) + ").")
              << "\n"
              << ui::gris("  Ojo: el puntero sigue guardando esa direccion. Ahora es colgante.")
              << "\n";
  }

  void mapa() {
    std::vector<std::string> lineas;
    lineas.push_back(ui::gris("  id   direccion          bytes  estado     contenido"));
    for (const Bloque& b : bloques_) {
      std::ostringstream os;
      os << "  #" << b.id << "   " << ui::dir(b.direccion) << "   "
         << b.elementos * static_cast<int>(sizeof(int)) << "     "
         << (b.activo ? ui::verde("en uso ") : ui::gris("libre  ")) << "   ";
      std::string contenido;
      for (int v : b.datos) contenido += std::to_string(v) + " ";
      os << (b.activo ? contenido : ui::gris(contenido + "(basura)"));
      lineas.push_back(os.str());
    }
    if (bloques_.empty()) lineas.push_back(ui::gris("  (el monticulo esta vacio)"));
    int fugas = 0;
    for (const Bloque& b : bloques_)
      if (b.activo) ++fugas;
    lineas.push_back("");
    lineas.push_back(fugas == 0 ? ui::verde("  sin bloques pendientes")
                                : ui::amarillo("  bloques todavia sin liberar: " +
                                               std::to_string(fugas)));
    ui::caja("Estado del monticulo", lineas);
  }

  int evaluar() {
    int fugas = 0;
    int bytes_fugados = 0;
    for (const Bloque& b : bloques_) {
      if (b.activo) {
        ++fugas;
        bytes_fugados += b.elementos * static_cast<int>(sizeof(int));
      }
    }
    libero_todo_ = (fugas == 0);

    std::vector<std::string> lineas;
    const auto marca = [](bool ok) { return ok ? ui::verde("[ok]  ") : ui::rojo("[no]  "); };
    lineas.push_back(marca(reservo_cuatro_) + "reservar un bloque de 4 enteros");
    lineas.push_back(marca(escribio_todo_) + "escribir 10, 20, 30, 40");
    lineas.push_back(marca(libero_todo_) + "no dejar fugas de memoria");
    lineas.push_back(marca(errores_ == 0) + "no cometer accesos invalidos");
    lineas.push_back("");
    if (fugas > 0)
      lineas.push_back(ui::rojo("  Fuga: " + std::to_string(bytes_fugados) +
                                " bytes en " + std::to_string(fugas) +
                                " bloque(s) sin liberar."));
    if (errores_ > 0)
      lineas.push_back(ui::rojo("  Accesos invalidos: " + std::to_string(errores_)));

    int puntos = 0;
    if (reservo_cuatro_) puntos += 8;
    if (escribio_todo_) puntos += 8;
    if (libero_todo_) puntos += 8;
    puntos += (errores_ == 0) ? 6 : std::max(0, 6 - errores_ * 2);
    lineas.push_back("");
    lineas.push_back(ui::fuerte("  Puntos del taller: " + std::to_string(puntos) + " / 30"));
    ui::caja("Evaluacion del taller", lineas);

    ui::caja("Lo que hace un programa serio de C++ moderno",
             {ui::gris("  auto p = std::make_unique<int[]>(4);   // se libera solo al salir"),
              ui::gris("  std::vector<int> v(4);                 // crece, copia y limpia por ti"),
              "",
              "El taller que acabas de hacer a mano es justo lo que RAII automatiza:",
              "el destructor libera aunque haya una excepcion o un return temprano."});
    return puntos;
  }

  void procesar(const std::string& linea) {
    std::istringstream is(util::a_minusculas(linea));
    std::string cmd;
    is >> cmd;
    if (cmd.empty()) return;

    if (cmd == "reservar" || cmd == "new" || cmd == "reserva") {
      int n = 0;
      if (!(is >> n)) { std::cout << ui::rojo("  Uso: reservar N") << "\n"; return; }
      reservar(n);
    } else if (cmd == "escribir" || cmd == "write" || cmd == "escribe") {
      int id = 0, i = 0, v = 0;
      if (!(is >> id >> i >> v)) { std::cout << ui::rojo("  Uso: escribir ID INDICE VALOR") << "\n"; return; }
      escribir(id, i, v);
    } else if (cmd == "leer" || cmd == "read" || cmd == "lee") {
      int id = 0, i = 0;
      if (!(is >> id >> i)) { std::cout << ui::rojo("  Uso: leer ID INDICE") << "\n"; return; }
      leer(id, i);
    } else if (cmd == "liberar" || cmd == "delete" || cmd == "free" || cmd == "libera") {
      int id = 0;
      if (!(is >> id)) { std::cout << ui::rojo("  Uso: liberar ID") << "\n"; return; }
      liberar(id);
    } else if (cmd == "mapa" || cmd == "map") {
      mapa();
    } else if (cmd == "ayuda" || cmd == "help") {
      mostrar_mision();
    } else {
      std::cout << ui::rojo("  Comando desconocido. Escribe 'ayuda'.") << "\n";
    }
  }

  int bucle_interactivo() {
    std::string linea;
    while (true) {
      std::cout << ui::cian("heap> ") << std::flush;
      if (!std::getline(std::cin, linea)) { std::cout << "\n"; break; }
      linea = util::recortar(linea);
      if (!isatty(fileno(stdin))) std::cout << linea << "\n";
      const std::string norm = util::normalizar(linea);
      if (norm == "listo" || norm == "fin" || norm == "done" || norm == "salir") break;
      procesar(linea);
    }
    return evaluar();
  }

  int correr_demostracion() {
    const std::vector<std::string> guion = {"reservar 4",   "escribir 1 0 10", "escribir 1 1 20",
                                            "escribir 1 2 30", "escribir 1 3 40", "leer 1 2",
                                            "mapa",         "liberar 1",       "leer 1 2",
                                            "liberar 1",    "mapa"};
    for (const std::string& c : guion) {
      std::cout << ui::cian("heap> ") << c << "\n";
      procesar(c);
    }
    ui::parrafo("(la demostracion provoca a proposito un uso despues de liberar y una doble "
                "liberacion para que veas los avisos)",
                "  ", true);
    return evaluar();
  }
};

}  // namespace

Nivel nivel_heap(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "El monticulo";
  nivel.lema = "memoria que tu pides y tu devuelves";
  nivel.descripcion =
      "La pila se administra sola: entra y sale con cada llamada. El monticulo no: lo "
      "que pides con new sigue ahi hasta que alguien lo devuelve con delete. Ese "
      "'alguien' eres tu.";

  std::uniform_int_distribution<int> tam(3, 9);
  const int n = tam(azar);

  {
    // Comparamos direcciones reales de pila y monticulo en esta ejecucion.
    int en_pila = 0;
    auto en_monticulo = std::make_unique<int>(0);
    const std::uintptr_t dir_pila = entero(&en_pila);
    const std::uintptr_t dir_heap = entero(en_monticulo.get());
    const bool pila_mas_alta = dir_pila > dir_heap;

    Reto r;
    r.titulo = "Dos barrios de la memoria";
    r.escena = [dir_pila, dir_heap]() {
      ui::codigo({"int en_pila = 0;                 // variable local: pila",
                  "int* en_monticulo = new int(0);  // reservado a mano: monticulo"});
      ui::caja("Direcciones reales de esta ejecucion",
               {ui::gris("&en_pila      = ") + ui::cian(ui::dir(dir_pila)),
                ui::gris("en_monticulo  = ") + ui::cian(ui::dir(dir_heap)),
                "",
                ui::gris("entre ambas hay " + distancia_legible(dir_pila, dir_heap) +
                         " de espacio de direcciones sin usar")});
    };
    r.enunciado = "Cual de las dos direcciones es mayor? Responde 'pila' o 'monticulo'.";
    r.respuesta = pila_mas_alta ? "pila" : "monticulo";
    r.verificar = pila_mas_alta ? verif::texto({"pila", "stack", "la pila"})
                                : verif::texto({"monticulo", "heap", "el monticulo"});
    r.pista = "Compara los dos numeros hexadecimales de la escena.";
    r.explicacion = "En Linux la pila vive en la parte alta del espacio de direcciones y crece "
                    "hacia abajo; el monticulo esta mas abajo y crece hacia arriba. Ver la "
                    "direccion te dice de donde salio un objeto.";
    nivel.retos.push_back(r);
  }
  {
    const long long bytes = static_cast<long long>(n) * static_cast<long long>(sizeof(int));
    Reto r;
    r.titulo = "Cuanto pides realmente";
    r.escena = [n]() {
      ui::codigo({"int* p = new int[" + std::to_string(n) + "];"});
    };
    r.enunciado = "Cuantos bytes de datos reserva esa linea?";
    r.respuesta = std::to_string(bytes);
    r.verificar = verif::numero(bytes);
    r.pista = std::to_string(n) + " x sizeof(int).";
    r.explicacion = "Ademas el asignador guarda unos bytes de cabecera propios para saber "
                    "despues cuanto liberar: por eso pedir un millon de bloques diminutos "
                    "desperdicia mucha mas memoria de la que crees.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "La pareja correcta";
    r.escena = []() {
      ui::codigo({"int* p = new int[10];", "// ... uso de p ...", "???  p;"});
    };
    r.enunciado = "Con que hay que liberar esa reserva? Escribe 'delete' o 'delete[]'.";
    r.respuesta = "delete[]";
    r.verificar = verif::texto({"delete[]", "delete []", "deletecorchetes", "delete corchetes"});
    r.pista = "Lo que se reserva con new[] se libera con la version de corchetes.";
    r.explicacion = "new va con delete y new[] con delete[]. Mezclarlos es comportamiento "
                    "indefinido: delete a secas no ejecuta los destructores del resto de "
                    "elementos ni devuelve el bloque como corresponde.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Una fuga";
    r.escena = []() {
      ui::codigo({"void procesar() {", "    int* datos = new int[1000];",
                  "    if (hay_error()) return;   // <-- salida temprana",
                  "    delete[] datos;", "}"});
      ui::caja("Que pasa cuando hay_error() devuelve true?",
               {"a) el compilador libera datos al salir de la funcion",
                "b) se pierden 4000 bytes: nadie puede liberarlos ya",
                "c) el sistema operativo lo detecta y aborta el programa"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"fuga", "fuga de memoria", "memory leak"});
    r.pista = "Al volver de la funcion se destruye la variable 'datos' (el puntero), no lo "
              "que apunta.";
    r.explicacion = "Eso es una fuga: la memoria sigue reservada pero ya no queda ningun "
                    "puntero hacia ella. En un bucle o un servidor de larga vida, se acumula "
                    "hasta agotar la RAM. Herramientas: valgrind, -fsanitize=address.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Un puntero colgante";
    r.escena = []() {
      ui::codigo({"int* p = new int(42);", "delete p;", "std::cout << *p;   // <-- ?"});
      ui::caja("Que ocurre en la ultima linea?",
               {"a) imprime siempre 0", "b) es comportamiento indefinido: puede imprimir 42, "
                "basura o fallar",
                "c) el programa no compila"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"comportamiento indefinido", "undefined behavior",
                                      "puntero colgante", "dangling"});
    r.pista = "delete devuelve el bloque al asignador, pero no borra los bytes ni cambia p.";
    r.explicacion = "Lo traicionero es que muchas veces 'funciona' e imprime 42, porque los "
                    "bytes siguen ahi hasta que alguien reutilice el hueco. El fallo aparece "
                    "meses despues, en otro modulo. Asigna nullptr tras delete.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "RAII al rescate";
    r.escena = []() {
      ui::codigo({"void procesar() {",
                  "    auto datos = std::make_unique<int[]>(1000);",
                  "    if (hay_error()) return;   // ahora si", "}"});
      ui::caja("Cuando se libera esa memoria?",
               {"a) al terminar el programa", "b) cuando el recolector de basura lo decida",
                "c) al salir del ambito, en el destructor de unique_ptr, pase lo que pase"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "c";
    r.verificar = verif::opcion('c', {"raii", "al salir del ambito", "en el destructor"});
    r.pista = "C++ no tiene recolector de basura, pero si destructores deterministas.";
    r.explicacion = "RAII: el tiempo de vida del recurso es el del objeto que lo posee. "
                    "Funciona igual con un return temprano o con una excepcion. Regla practica: "
                    "en codigo moderno casi nunca deberias escribir new ni delete a mano.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Taller: administra el monticulo";
    r.puntos = 30;
    r.taller = true;
    r.enunciado = "Escribe comandos para cumplir la mision. 'ayuda' los lista, 'listo' termina.";
    r.respuesta = "reservar 4 / escribir / liberar 1 / listo";
    r.verificar = [](const std::string&) { return true; };
    r.ejecutar_taller = [](bool demo) {
      TallerHeap taller;
      return taller.ejecutar(demo);
    };
    nivel.retos.push_back(r);
  }

  return nivel;
}
