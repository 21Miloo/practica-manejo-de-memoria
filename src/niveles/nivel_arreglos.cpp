// Nivel 7: arreglos dinamicos. Reservar en ejecucion, crecer copiando,
// matrices dinamicas y un taller donde el jugador hace de std::vector.
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

// --- Taller: construir a mano un arreglo que crece ------------------------

class TallerArreglo {
 public:
  int ejecutar(bool demo) {
    // El bloque inicial: capacidad 2, vacio.
    capacidad_ = 2;
    datos_.clear();
    direccion_ = 0x55b1a0002000ull;
    mostrar_mision();
    ver();
    if (demo) return correr_demostracion();
    return bucle_interactivo();
  }

 private:
  int capacidad_ = 2;
  std::vector<int> datos_;
  unsigned long long direccion_ = 0;  // simulada: 64 bits en cualquier plataforma

  // Bloque viejo pendiente de liberar tras un crecimiento.
  bool hay_viejo_ = false;
  int capacidad_vieja_ = 0;
  unsigned long long direccion_vieja_ = 0;
  bool copiado_ = true;

  bool activo_ = true;  // el bloque actual sigue reservado
  int desbordamientos_ = 0;
  int bytes_fugados_ = 0;
  bool crecio_duplicando_ = true;
  bool perdio_datos_ = false;

  void mostrar_mision() {
    ui::caja("Taller: hazte tu propio std::vector",
             {
                 "Empiezas con un bloque de capacidad 2 y ningun elemento.",
                 "Mision: guardar 10, 20, 30, 40 y 50, y liberar todo al final.",
                 "",
                 ui::fuerte("Comandos:"),
                 "  agregar V        escribe V en la siguiente posicion libre",
                 "  reservar N       new int[N]: pide un bloque nuevo mas grande",
                 "  copiar           copia los elementos del bloque viejo al nuevo",
                 "  liberar_viejo    delete[] del bloque viejo",
                 "  liberar          delete[] del bloque actual",
                 "  ver              dibuja el estado de los bloques",
                 "  listo            termina el taller y lo evalua",
                 "",
                 ui::gris("Crecer son siempre tres pasos: reservar, copiar y liberar el viejo."),
             });
  }

  std::string fila_celdas(const std::vector<int>& v, int capacidad) {
    std::vector<std::string> celdas;
    for (int i = 0; i < capacidad; ++i) {
      celdas.push_back(i < static_cast<int>(v.size()) ? std::to_string(v[i]) : "-");
    }
    return ui::celdas(celdas);
  }

  void ver() {
    std::vector<std::string> lineas;
    lineas.push_back(ui::gris("bloque actual  ") + ui::cian(ui::dir(direccion_)) +
                     ui::gris("   capacidad ") + std::to_string(capacidad_) +
                     ui::gris(", tamano ") + std::to_string(datos_.size()) +
                     ui::gris(", ") + std::to_string(capacidad_ * static_cast<int>(sizeof(int))) +
                     ui::gris(" bytes"));
    lineas.push_back("  " + fila_celdas(datos_, capacidad_));
    if (!activo_) lineas.push_back(ui::gris("  (ya liberado)"));
    if (hay_viejo_) {
      lineas.push_back("");
      lineas.push_back(ui::amarillo("bloque viejo   ") + ui::cian(ui::dir(direccion_vieja_)) +
                       ui::gris("   capacidad ") + std::to_string(capacidad_vieja_) +
                       ui::amarillo("  <- sigue reservado"));
      lineas.push_back(ui::gris(copiado_ ? "  ya copiaste sus datos al bloque nuevo"
                                         : "  todavia no copias sus datos al bloque nuevo"));
    }
    ui::caja("Estado del arreglo dinamico", lineas);
  }

  void agregar(int v) {
    if (!activo_) {
      std::cout << ui::rojo("  El bloque actual ya se libero: escribir ahi es un uso despues "
                            "de liberar.")
                << "\n";
      ++desbordamientos_;
      return;
    }
    if (static_cast<int>(datos_.size()) >= capacidad_) {
      ++desbordamientos_;
      std::cout << ui::rojo("  DESBORDAMIENTO: no cabe.") << "\n";
      ui::parrafo("El bloque tiene capacidad " + std::to_string(capacidad_) +
                      " y ya guarda " + std::to_string(datos_.size()) +
                      " elementos. Escribir en el indice " + std::to_string(datos_.size()) +
                      " toca la direccion " +
                      ui::dir(direccion_ + static_cast<unsigned long long>(datos_.size()) *
                                               sizeof(int)) +
                      ", que ya esta fuera del bloque. C++ te dejaria hacerlo sin avisar: "
                      "primero hay que crecer (reservar, copiar, liberar_viejo).",
                  "  ", true);
      return;
    }
    datos_.push_back(v);
    std::cout << ui::gris("  datos[" + std::to_string(datos_.size() - 1) + "] = " +
                          std::to_string(v) + "   (direccion " +
                          ui::dir(direccion_ + (datos_.size() - 1) * sizeof(int)) + ")")
              << "\n";
  }

  void reservar(int n) {
    if (n <= 0 || n > 64) {
      std::cout << ui::rojo("  Reserva entre 1 y 64 elementos.") << "\n";
      return;
    }
    if (n < static_cast<int>(datos_.size())) {
      std::cout << ui::rojo("  Ese bloque es mas pequeno que los datos que ya tienes: "
                            "perderias elementos.")
                << "\n";
      return;
    }
    if (hay_viejo_) {
      // Reservar otra vez sin haber liberado el anterior: se pierde su direccion.
      bytes_fugados_ += capacidad_vieja_ * static_cast<int>(sizeof(int));
      std::cout << ui::rojo("  FUGA: perdiste la unica referencia al bloque de " +
                            ui::dir(direccion_vieja_) + ".")
                << "\n";
      ui::parrafo("Todavia estaba reservado y ya nadie guarda su direccion, asi que nunca "
                  "podras liberarlo. Asi se acumulan las fugas en un bucle.",
                  "  ", true);
    }
    if (n < capacidad_ * 2) crecio_duplicando_ = false;

    hay_viejo_ = activo_;
    capacidad_vieja_ = capacidad_;
    direccion_vieja_ = direccion_;
    copiado_ = false;

    capacidad_ = n;
    direccion_ += static_cast<unsigned long long>(capacidad_vieja_) * sizeof(int) + 16;
    activo_ = true;
    std::cout << ui::verde("  Reservado un bloque de " + std::to_string(n) + " elementos (" +
                           std::to_string(n * static_cast<int>(sizeof(int))) + " bytes) en " +
                           ui::dir(direccion_))
              << "\n";
    ui::parrafo("El bloque nuevo esta vacio: los datos siguen en el viejo hasta que los "
                "copies. Y el viejo sigue reservado hasta que lo liberes.",
                "  ", true);
  }

  void copiar() {
    if (!hay_viejo_) {
      std::cout << ui::rojo("  No hay ningun bloque viejo del que copiar.") << "\n";
      return;
    }
    copiado_ = true;
    std::cout << ui::verde("  Copiados " + std::to_string(datos_.size()) +
                           " elementos del bloque viejo al nuevo.")
              << "\n"
              << ui::gris("  (esto es lo que hace std::vector al crecer, y por eso crecer "
                          "cuesta O(n))")
              << "\n";
  }

  void liberar_viejo() {
    if (!hay_viejo_) {
      std::cout << ui::rojo("  No hay ningun bloque viejo pendiente.") << "\n";
      return;
    }
    if (!copiado_) {
      perdio_datos_ = true;
      std::cout << ui::rojo("  Liberaste el bloque viejo sin copiar sus datos.") << "\n";
      ui::parrafo("Los elementos que habia ahi se pierden: el bloque nuevo se queda con "
                  "basura. Orden correcto: reservar, copiar y solo entonces liberar_viejo.",
                  "  ", true);
    } else {
      std::cout << ui::verde("  Liberado el bloque viejo (" + ui::dir(direccion_vieja_) + ").")
                << "\n";
    }
    hay_viejo_ = false;
  }

  void liberar() {
    if (!activo_) {
      std::cout << ui::rojo("  DOBLE LIBERACION: ese bloque ya estaba liberado.") << "\n";
      ++desbordamientos_;
      return;
    }
    activo_ = false;
    std::cout << ui::verde("  Liberado el bloque actual (" + ui::dir(direccion_) + ").") << "\n";
  }

  int evaluar() {
    if (activo_) bytes_fugados_ += capacidad_ * static_cast<int>(sizeof(int));
    if (hay_viejo_) bytes_fugados_ += capacidad_vieja_ * static_cast<int>(sizeof(int));

    const std::vector<int> objetivo = {10, 20, 30, 40, 50};
    const bool datos_ok = (datos_ == objetivo) && !perdio_datos_;
    const bool sin_fugas = bytes_fugados_ == 0;
    const bool sin_desbordes = desbordamientos_ == 0;

    const auto marca = [](bool ok) { return ok ? ui::verde("[ok]  ") : ui::rojo("[no]  "); };
    std::vector<std::string> lineas;
    lineas.push_back(marca(datos_ok) + "guardar 10, 20, 30, 40 y 50");
    lineas.push_back(marca(sin_fugas) + "no dejar fugas de memoria");
    lineas.push_back(marca(sin_desbordes) + "no escribir fuera del bloque");
    lineas.push_back(marca(crecio_duplicando_) + "crecer duplicando la capacidad");
    lineas.push_back("");
    if (!sin_fugas)
      lineas.push_back(ui::rojo("  Fuga total: " + std::to_string(bytes_fugados_) + " bytes."));
    if (!sin_desbordes)
      lineas.push_back(ui::rojo("  Accesos fuera del bloque: " +
                                std::to_string(desbordamientos_)));

    int puntos = 0;
    if (datos_ok) puntos += 10;
    if (sin_fugas) puntos += 8;
    puntos += sin_desbordes ? 7 : std::max(0, 7 - desbordamientos_ * 2);
    if (crecio_duplicando_) puntos += 5;
    lineas.push_back("");
    lineas.push_back(ui::fuerte("  Puntos del taller: " + std::to_string(puntos) + " / 30"));
    ui::caja("Evaluacion del taller", lineas);

    ui::caja("Por que std::vector duplica en vez de crecer de a uno",
             {"Creciendo de a 1, insertar n elementos copia 1+2+...+(n-1): O(n^2).",
              "Duplicando, cada elemento se copia unas 2 veces en promedio: O(n)",
              "amortizado. Ese es todo el truco de push_back.",
              "",
              ui::gris("  std::vector<int> v;  v.reserve(5);  v.push_back(10);")});
    return puntos;
  }

  void procesar(const std::string& linea) {
    std::istringstream is(util::a_minusculas(linea));
    std::string cmd;
    is >> cmd;
    if (cmd.empty()) return;

    if (cmd == "agregar" || cmd == "push" || cmd == "agrega") {
      int v = 0;
      if (!(is >> v)) { std::cout << ui::rojo("  Uso: agregar VALOR") << "\n"; return; }
      agregar(v);
    } else if (cmd == "reservar" || cmd == "new" || cmd == "crecer") {
      int n = 0;
      if (!(is >> n)) { std::cout << ui::rojo("  Uso: reservar N") << "\n"; return; }
      reservar(n);
    } else if (cmd == "copiar" || cmd == "copia") {
      copiar();
    } else if (cmd == "liberar_viejo" || cmd == "liberarviejo") {
      liberar_viejo();
    } else if (cmd == "liberar" || cmd == "delete") {
      liberar();
    } else if (cmd == "ver" || cmd == "mapa" || cmd == "estado") {
      ver();
    } else if (cmd == "ayuda" || cmd == "help") {
      mostrar_mision();
    } else {
      std::cout << ui::rojo("  Comando desconocido. Escribe 'ayuda'.") << "\n";
    }
  }

  int bucle_interactivo() {
    std::string linea;
    while (true) {
      std::cout << ui::cian("arreglo> ") << std::flush;
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
    const std::vector<std::string> guion = {
        "agregar 10", "agregar 20",    "agregar 30",     "reservar 4",  "copiar",
        "liberar_viejo", "agregar 30", "agregar 40",     "ver",         "reservar 8",
        "copiar",     "liberar_viejo", "agregar 50",     "ver",         "liberar"};
    for (const std::string& c : guion) {
      std::cout << ui::cian("arreglo> ") << c << "\n";
      procesar(c);
    }
    ui::parrafo("(la demostracion intenta a proposito agregar un tercer elemento en un bloque "
                "de capacidad 2 para que veas el desbordamiento antes de crecer)",
                "  ", true);
    return evaluar();
  }
};

}  // namespace

Nivel nivel_arreglos(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "Arreglos dinamicos";
  nivel.lema = "cuando el tamano se decide en ejecucion";
  nivel.descripcion =
      "int datos[8] exige saber el tamano al compilar. Cuando no lo sabes, el bloque se "
      "pide en ejecucion con new[] y a partir de ahi todo corre por tu cuenta: cuantos "
      "elementos hay, cuando crecer y cuando liberar.";

  const int n = util::entero_en_rango(azar, 5, 12);

  {
    Reto r;
    r.titulo = "sizeof no sabe";
    r.escena = [n]() {
      ui::codigo({"int estatico[" + std::to_string(n) + "];",
                  "int* dinamico = new int[" + std::to_string(n) + "];"});
      ui::caja("", {ui::gris("sizeof(estatico) = ") +
                        std::to_string(n * static_cast<int>(sizeof(int))) +
                        ui::gris(" bytes: el tipo conoce el tamano"),
                    ui::gris("sizeof(dinamico) = ") + ui::fuerte("?")});
    };
    r.enunciado = "Cuanto vale sizeof(dinamico), siendo dinamico un int*?";
    r.respuesta = std::to_string(sizeof(int*));
    r.verificar = verif::numero(static_cast<long long>(sizeof(int*)));
    r.pista = "dinamico no es un arreglo: es un puntero. Mide lo que mide una direccion.";
    // Se calcula con variables para que el compilador no avise de la division
    // que este reto ensena precisamente a no escribir (-Wsizeof-pointer-div).
    const std::size_t tam_puntero = sizeof(int*);
    const std::size_t tam_elemento = sizeof(int);
    r.explicacion = "Este es el error clasico: sizeof(puntero)/sizeof(int) da " +
                    std::to_string(tam_puntero / tam_elemento) +
                    ", no el numero de elementos. Un arreglo dinamico no sabe cuantos "
                    "elementos tiene: el tamano lo guardas tu, o usas std::vector.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Como sabe delete[] cuantos son";
    r.escena = []() {
      ui::codigo({"int* p = new int[100];", "delete[] p;   // libera los 100, sin decirselo"});
      ui::caja("De donde sale ese 100?",
               {"a) delete[] recorre la memoria hasta encontrar un cero",
                "b) el asignador guarda el tamano en unos bytes de cabecera, justo antes "
                "del bloque",
                "c) el compilador lo recuerda por ti en una tabla global"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"cabecera", "en la cabecera", "metadatos"});
    r.pista = "Alguien tiene que apuntarlo en algun lado, y ese alguien es el asignador.";
    r.explicacion = "Por eso el puntero que pasas a delete[] debe ser exactamente el que "
                    "devolvio new[]: si lo mueves, apuntas a un sitio donde no hay cabecera. "
                    "Y por eso mil bloques diminutos gastan mucha mas memoria de la pedida.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "El precio de crecer";
    r.escena = []() {
      ui::caja("Un arreglo que crece duplicando su capacidad",
               {ui::gris("capacidad 1 -> 2 -> 4 -> 8, copiando todo en cada paso"),
                "",
                ui::gris("  1 -> 2 : copia 1 elemento"),
                ui::gris("  2 -> 4 : copia 2 elementos"),
                ui::gris("  4 -> 8 : copia 4 elementos")});
    };
    r.enunciado = "Cuantas copias de elementos cuesta en total llegar de capacidad 1 a 8?";
    r.respuesta = "7";
    r.verificar = verif::numero(7);
    r.pista = "1 + 2 + 4.";
    r.explicacion = "Siempre queda por debajo del numero de elementos x 2: por eso push_back "
                    "es O(1) amortizado. Creciendo de a uno serian 1+2+...+7 = 28 copias, y "
                    "el costo total pasaria a ser O(n^2).";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "La fuga al crecer";
    r.escena = []() {
      ui::codigo({"int* viejo = new int[4];", "int* nuevo = new int[8];",
                  "for (int i = 0; i < 4; ++i) nuevo[i] = viejo[i];",
                  "viejo = nuevo;   // <-- y el bloque de 4 que estaba ahi?"});
    };
    r.enunciado = "Cuantos bytes se fugan en ese fragmento?";
    r.respuesta = std::to_string(4 * sizeof(int));
    r.verificar = verif::numero(static_cast<long long>(4 * sizeof(int)));
    r.pista = "El bloque viejo tenia 4 ints y ya nadie guarda su direccion.";
    r.explicacion = "Falta delete[] viejo antes de reasignar el puntero. Es la fuga mas "
                    "comun al implementar estructuras que crecen: cada crecimiento pierde "
                    "el bloque anterior.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Matriz dinamica";
    r.escena = []() {
      ui::codigo({"int** m = new int*[3];              // 3 punteros a fila",
                  "for (int f = 0; f < 3; ++f)",
                  "    m[f] = new int[4];              // cada fila, por separado"});
      ui::caja("", {ui::gris("m -> [ptr][ptr][ptr]  y cada ptr -> una fila de 4 ints"),
                    ui::gris("las filas se piden por separado: no tienen por que quedar "
                             "pegadas entre si")});
    };
    r.enunciado = "Cuantas llamadas a delete[] hacen falta para liberar todo?";
    r.respuesta = "4";
    r.verificar = verif::numero(4);
    r.pista = "Una por cada fila, y una mas por el arreglo de punteros.";
    r.explicacion = "Y en ese orden: primero las filas, despues m. Si liberas m primero, "
                    "pierdes los punteros a las filas y ya no puedes liberarlas.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "La matriz de un solo bloque";
    r.escena = []() {
      ui::codigo({"int* m = new int[3 * 4];            // un unico bloque contiguo",
                  "// el elemento (fila, columna) esta en m[fila * 4 + columna]"});
      ui::caja("", {ui::gris("una sola reserva, un solo delete[], y todo contiguo:"),
                    ui::gris("mejor para la cache que un arreglo de punteros a filas")});
    };
    r.enunciado = "En que indice de m esta el elemento de la fila 2, columna 3?";
    r.respuesta = "11";
    r.verificar = verif::numero(11);
    r.pista = "fila * numero_de_columnas + columna = 2 * 4 + 3.";
    r.explicacion = "Es la misma cuenta que hace el compilador con int m[3][4]. Con un solo "
                    "bloque tienes contiguidad real y una sola reserva que liberar.";
    nivel.retos.push_back(r);
  }
  {
    // Capacidad real de esta implementacion de std::vector, medida al vuelo.
    std::vector<int> v;
    std::vector<std::size_t> capacidades;
    for (int i = 0; i < 5; ++i) {
      v.push_back(i);
      capacidades.push_back(v.capacity());
    }
    const long long final_cap = static_cast<long long>(capacidades.back());
    Reto r;
    r.titulo = "Lo que hace vector por dentro";
    r.escena = [capacidades]() {
      std::vector<std::string> lineas{
          ui::gris("capacidad real medida en esta ejecucion, tras cada push_back:")};
      for (std::size_t i = 0; i < capacidades.size(); ++i) {
        lineas.push_back("  push_back #" + std::to_string(i + 1) + ":  tamano " +
                         std::to_string(i + 1) + ", capacidad " +
                         ui::fuerte(std::to_string(capacidades[i])));
      }
      lineas.push_back("");
      lineas.push_back(ui::gris("cada salto de capacidad reservo un bloque nuevo, copio los "
                                "elementos y libero el viejo"));
      ui::caja("std::vector<int> v;  cinco push_back", lineas);
    };
    r.enunciado = "Cual es la capacidad despues del quinto push_back?";
    r.respuesta = std::to_string(final_cap);
    r.verificar = verif::numero(final_cap);
    r.pista = "Esta en la ultima linea de la tabla: la capacidad crece a saltos, no de a uno.";
    r.explicacion = "tamano es cuantos elementos hay; capacidad es para cuantos hay sitio "
                    "reservado. reserve() evita las copias intermedias cuando ya sabes "
                    "cuantos vas a necesitar.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Taller: hazte tu propio vector";
    r.puntos = 30;
    r.taller = true;
    r.enunciado = "Haz crecer el arreglo a mano hasta guardar 10, 20, 30, 40 y 50.";
    r.respuesta = "reservar / copiar / liberar_viejo / agregar ... / liberar / listo";
    r.verificar = [](const std::string&) { return true; };
    r.ejecutar_taller = [](bool demo) {
      TallerArreglo taller;
      return taller.ejecutar(demo);
    };
    nivel.retos.push_back(r);
  }

  return nivel;
}
