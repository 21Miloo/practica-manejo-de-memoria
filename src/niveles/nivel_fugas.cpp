// Nivel 8: los dos errores clasicos de memoria. Fugas (memoria que se pide y
// no se devuelve) y desbordamientos (escribir fuera de lo reservado).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

// --- Taller: un marco de pila que se puede aplastar -----------------------

constexpr int kTamBuffer = 8;
constexpr int kInicioCanario = 8;
constexpr int kTamCanario = 4;
constexpr int kInicioRetorno = 12;
constexpr int kTamRetorno = 8;
constexpr int kTotal = kInicioRetorno + kTamRetorno;

class TallerBuffer {
 public:
  int ejecutar(bool demo) {
    reiniciar(false);
    mostrar_mision();
    ver();
    if (demo) return correr_demostracion();
    return bucle_interactivo();
  }

 private:
  unsigned char marco_[kTotal] = {0};
  unsigned char canario_original_[kTamCanario] = {0x55, 0xaa, 0x55, 0xaa};
  unsigned char retorno_original_[kTamRetorno] = {0x40, 0x11, 0x64, 0x55,
                                                  0x00, 0x00, 0x00, 0x00};
  bool copio_cabiendo_ = false;
  bool provoco_desbordamiento_ = false;
  bool uso_version_segura_ = false;
  int desbordamientos_ = 0;

  void mostrar_mision() {
    ui::caja("Taller: aplasta un marco de pila (y luego hazlo bien)",
             {
                 "Asi queda la memoria de una funcion con un buffer local:",
                 "",
                 "   char buffer[8];   <- lo unico que te pertenece",
                 "   [canario 4 bytes] <- valor centinela que pone el compilador",
                 "   [retorno 8 bytes] <- a donde vuelve la funcion al terminar",
                 "",
                 ui::fuerte("Mision:"),
                 "  1. copia una cadena que quepa",
                 "  2. provoca un desbordamiento y mira que se corrompe",
                 "  3. reinicia y copia esa misma cadena con la version segura",
                 "  4. termina con el canario y el retorno intactos",
                 "",
                 ui::fuerte("Comandos:"),
                 "  copiar TEXTO         strcpy(buffer, TEXTO): copia sin mirar el tamano",
                 "  copiar_seguro TEXTO  copia como mucho 7 caracteres y el terminador",
                 "  escribir I VALOR     buffer[I] = VALOR (0 a 255)",
                 "  reiniciar            vuelve a entrar en la funcion: marco limpio",
                 "  ver                  dibuja el marco byte a byte",
                 "  listo                termina el taller y lo evalua",
             });
  }

  void reiniciar(bool avisar) {
    std::memset(marco_, 0, sizeof(marco_));
    std::memcpy(marco_ + kInicioCanario, canario_original_, kTamCanario);
    std::memcpy(marco_ + kInicioRetorno, retorno_original_, kTamRetorno);
    if (avisar) {
      std::cout << ui::verde("  Marco reiniciado: canario y direccion de retorno restaurados.")
                << "\n";
    }
  }

  bool canario_intacto() const {
    return std::memcmp(marco_ + kInicioCanario, canario_original_, kTamCanario) == 0;
  }
  bool retorno_intacto() const {
    return std::memcmp(marco_ + kInicioRetorno, retorno_original_, kTamRetorno) == 0;
  }

  std::string tramo(int desde, int cuantos, bool intacto) const {
    std::string hex;
    char buf[8];
    for (int i = desde; i < desde + cuantos; ++i) {
      std::snprintf(buf, sizeof(buf), "%02x ", marco_[i]);
      hex += buf;
    }
    return intacto ? ui::verde(hex) : ui::rojo(hex);
  }

  void ver() {
    std::string ascii;
    for (int i = 0; i < kTamBuffer; ++i) {
      const unsigned char c = marco_[i];
      ascii += (c >= 32 && c < 127) ? static_cast<char>(c) : '.';
    }
    // La cabecera se genera con el mismo ancho por columna que los bytes para
    // que los indices queden justo encima de su byte.
    std::string cabecera = "indice ";
    char celda[8];
    for (int i = 0; i < kTotal; ++i) {
      if (i == kInicioCanario || i == kInicioRetorno) cabecera += "| ";
      std::snprintf(celda, sizeof(celda), "%2d ", i);
      cabecera += celda;
    }
    std::vector<std::string> lineas{
        ui::gris(cabecera),
        "buffer " + tramo(0, kTamBuffer, true) + ui::gris("| ") +
            tramo(kInicioCanario, kTamCanario, canario_intacto()) + ui::gris("| ") +
            tramo(kInicioRetorno, kTamRetorno, retorno_intacto()),
        "",
        ui::gris("buffer como texto: '") + ascii + ui::gris("'"),
        ui::gris("canario: ") + (canario_intacto() ? ui::verde("intacto")
                                                   : ui::rojo("APLASTADO")),
        ui::gris("retorno: ") + (retorno_intacto() ? ui::verde("intacto")
                                                   : ui::rojo("APLASTADO"))};
    ui::caja("Marco de la funcion", lineas);
  }

  void copiar(const std::string& texto, bool seguro) {
    const int necesarios = static_cast<int>(texto.size()) + 1;  // + el '\0'
    if (seguro) {
      uso_version_segura_ = true;
      const int copiables = kTamBuffer - 1;
      const int n = std::min(static_cast<int>(texto.size()), copiables);
      std::memset(marco_, 0, kTamBuffer);
      std::memcpy(marco_, texto.data(), static_cast<std::size_t>(n));
      marco_[n] = 0;
      std::cout << ui::verde("  Copiados " + std::to_string(n) +
                             " caracteres y el terminador, dentro del buffer.")
                << "\n";
      if (n < static_cast<int>(texto.size())) {
        ui::parrafo("La cadena no cabia entera (" + std::to_string(necesarios) +
                        " bytes con terminador, para un buffer de " +
                        std::to_string(kTamBuffer) +
                        "): se trunco. Perder texto es mucho mejor que aplastar el marco.",
                    "  ", true);
      }
      return;
    }

    // strcpy: copia hasta el terminador, sin mirar el tamano del destino.
    for (int i = 0; i < necesarios && i < kTotal; ++i) {
      marco_[i] = (i < static_cast<int>(texto.size()))
                      ? static_cast<unsigned char>(texto[i])
                      : 0;
    }
    if (necesarios <= kTamBuffer) {
      copio_cabiendo_ = true;
      std::cout << ui::verde("  Copiados " + std::to_string(necesarios) +
                             " bytes: caben en el buffer de " + std::to_string(kTamBuffer) + ".")
                << "\n";
      return;
    }

    ++desbordamientos_;
    provoco_desbordamiento_ = true;
    const int fuera = necesarios - kTamBuffer;
    std::cout << ui::rojo("  DESBORDAMIENTO DE BUFFER.") << "\n";
    ui::parrafo("La cadena necesita " + std::to_string(necesarios) +
                    " bytes (contando el terminador) y el buffer solo tiene " +
                    std::to_string(kTamBuffer) + ": se escribieron " + std::to_string(fuera) +
                    " bytes fuera. strcpy no recibe el tamano del destino, asi que no puede "
                    "comprobar nada; simplemente sigue escribiendo.",
                "  ", true);
    ver();
    if (!canario_intacto()) {
      ui::parrafo("El canario ya no vale lo que valia: al volver de la funcion el programa "
                  "lo comprueba, ve que cambio y aborta con 'stack smashing detected'. Esa "
                  "comprobacion es justo lo que impide que esto se convierta en una "
                  "vulnerabilidad explotable.",
                  "  ", true);
    }
    if (!retorno_intacto()) {
      ui::parrafo("Y llegaste hasta la direccion de retorno: sobreescribirla es la base del "
                  "ataque clasico de desbordamiento de pila, porque decide a donde salta el "
                  "programa cuando la funcion termina.",
                  "  ", true);
    }
  }

  void escribir(int indice, int valor) {
    if (valor < 0 || valor > 255) {
      std::cout << ui::rojo("  El valor de un byte va de 0 a 255.") << "\n";
      return;
    }
    if (indice < 0 || indice >= kTotal) {
      std::cout << ui::rojo("  Fuera del marco simulado (indices 0 a " +
                            std::to_string(kTotal - 1) + ").")
                << "\n";
      return;
    }
    marco_[indice] = static_cast<unsigned char>(valor);
    if (indice >= kTamBuffer) {
      ++desbordamientos_;
      provoco_desbordamiento_ = true;
      std::cout << ui::rojo("  Escribiste en el indice " + std::to_string(indice) +
                            ", fuera del buffer de " + std::to_string(kTamBuffer) + " bytes.")
                << "\n";
      ui::parrafo(indice < kInicioRetorno
                      ? "Ese byte es del canario, no tuyo."
                      : "Ese byte es de la direccion de retorno, no tuyo.",
                  "  ", true);
    } else {
      std::cout << ui::gris("  buffer[" + std::to_string(indice) + "] = " +
                            std::to_string(valor))
                << "\n";
    }
  }

  int evaluar() {
    const bool termino_limpio = canario_intacto() && retorno_intacto();
    const auto marca = [](bool ok) { return ok ? ui::verde("[ok]  ") : ui::rojo("[no]  "); };
    std::vector<std::string> lineas{
        marca(copio_cabiendo_) + "copiar una cadena que quepa en el buffer",
        marca(provoco_desbordamiento_) + "provocar un desbordamiento y verlo",
        marca(uso_version_segura_) + "usar la copia con limite de tamano",
        marca(termino_limpio) + "terminar con el canario y el retorno intactos"};
    lineas.push_back("");
    if (!termino_limpio)
      lineas.push_back(ui::rojo("  El marco quedo corrompido: 'reiniciar' lo deja limpio."));

    int puntos = 0;
    if (copio_cabiendo_) puntos += 7;
    if (provoco_desbordamiento_) puntos += 7;
    if (uso_version_segura_) puntos += 8;
    if (termino_limpio) puntos += 8;
    lineas.push_back("");
    lineas.push_back(ui::fuerte("  Puntos del taller: " + std::to_string(puntos) + " / 30"));
    ui::caja("Evaluacion del taller", lineas);

    ui::caja("Como se evita esto de verdad",
             {"  std::string s = texto;            // crece sola, no hay tamano fijo",
              "  std::vector<char> v(n);           // conoce su tamano",
              "  v.at(i)                           // comprueba limites y lanza",
              "  snprintf(dst, sizeof(dst), ...)   // si tienes que usar C",
              "",
              ui::gris("Y para encontrarlos: g++ -fsanitize=address,undefined"),
              ui::gris("(los ejemplos de la carpeta ejemplos/ ya estan preparados asi)")});
    return puntos;
  }

  void procesar(const std::string& linea) {
    // El comando se normaliza, pero el texto a copiar se respeta tal cual.
    const std::size_t espacio = linea.find(' ');
    const std::string cmd =
        util::a_minusculas(espacio == std::string::npos ? linea : linea.substr(0, espacio));
    std::string resto =
        espacio == std::string::npos ? "" : util::recortar(linea.substr(espacio + 1));
    if (resto.size() >= 2 && resto.front() == '"' && resto.back() == '"') {
      resto = resto.substr(1, resto.size() - 2);
    }

    if (cmd == "copiar" || cmd == "strcpy" || cmd == "copia") {
      if (resto.empty()) { std::cout << ui::rojo("  Uso: copiar TEXTO") << "\n"; return; }
      copiar(resto, false);
    } else if (cmd == "copiar_seguro" || cmd == "seguro" || cmd == "copiarseguro") {
      if (resto.empty()) { std::cout << ui::rojo("  Uso: copiar_seguro TEXTO") << "\n"; return; }
      copiar(resto, true);
    } else if (cmd == "escribir" || cmd == "escribe") {
      std::istringstream is(resto);
      int i = 0, v = 0;
      if (!(is >> i >> v)) { std::cout << ui::rojo("  Uso: escribir INDICE VALOR") << "\n"; return; }
      escribir(i, v);
    } else if (cmd == "reiniciar" || cmd == "reset") {
      reiniciar(true);
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
      if (!ui::leer_linea("buffer> ", linea)) break;
      linea = util::recortar(linea);
      const std::string norm = util::normalizar(linea);
      if (norm == "listo" || norm == "fin" || norm == "done" || norm == "salir") break;
      procesar(linea);
    }
    return evaluar();
  }

  int correr_demostracion() {
    const std::vector<std::string> guion = {"copiar hola", "ver",
                                            "copiar desbordamiento", "reiniciar",
                                            "copiar_seguro desbordamiento", "ver"};
    for (const std::string& c : guion) {
      std::cout << ui::cian("buffer> ") << c << "\n";
      procesar(c);
    }
    return evaluar();
  }
};

}  // namespace

Nivel nivel_fugas(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "Fugas y desbordamientos";
  nivel.lema = "los dos errores que mas caros salen";
  nivel.descripcion =
      "Una fuga es memoria que pediste y nunca devolviste: el programa engorda hasta "
      "morir. Un desbordamiento es escribir fuera de lo que reservaste: pisas datos "
      "ajenos. El primero se nota tarde; el segundo, a veces nunca.";

  const int n = util::entero_en_rango(azar, 3, 9);
  const int m = util::entero_en_rango(azar, 10, 40);

  {
    const long long fugados = static_cast<long long>(n) * static_cast<long long>(m) *
                              static_cast<long long>(sizeof(int));
    Reto r;
    r.titulo = "Contar la fuga";
    r.escena = [n, m]() {
      ui::codigo({"for (int i = 0; i < " + std::to_string(n) + "; ++i) {",
                  "    int* bloque = new int[" + std::to_string(m) + "];",
                  "    usar(bloque);",
                  "}   // <- 'bloque' muere aqui; la memoria no"});
    };
    r.enunciado = "Cuantos bytes se fugan en total al terminar el bucle?";
    r.respuesta = std::to_string(fugados);
    r.verificar = verif::numero(fugados);
    r.pista = std::to_string(n) + " vueltas x " + std::to_string(m) + " ints x " +
              std::to_string(sizeof(int)) + " bytes.";
    r.explicacion = "En cada vuelta se pierde la unica referencia al bloque anterior. Un bucle "
                    "asi dentro de un servidor que corre semanas termina agotando la RAM: eso "
                    "es lo que se ve como 'el proceso crece y crece'.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Cual no es una fuga";
    r.escena = []() {
      ui::caja("Tres fragmentos parecidos",
               {"a) int* p = new int(1); p = new int(2); delete p;",
                "b) int* p = new int(1); delete p; p = new int(2); delete p;",
                "c) int* p = new int[5]; return;   // sin delete[]"});
    };
    r.enunciado = "Cual de los tres NO tiene fuga? Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"el b", "la b"});
    r.pista = "Busca el fragmento donde cada new tiene su delete antes de perder el puntero.";
    r.explicacion = "En (a) el primer bloque se pierde al reasignar p. En (c) nunca se libera. "
                    "Solo (b) libera antes de reutilizar el puntero. La regla: cada new necesita "
                    "exactamente un delete, y hay que hacerlo mientras todavia tienes la "
                    "direccion.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "El error de uno";
    r.escena = [m]() {
      ui::codigo({"int* datos = new int[" + std::to_string(m) + "];",
                  "for (int i = 0; i <= " + std::to_string(m) + "; ++i)   // <= en vez de <",
                  "    datos[i] = 0;"});
    };
    r.enunciado = "Cuantos elementos se escriben fuera del bloque?";
    r.respuesta = "1";
    r.verificar = verif::numero(1);
    r.pista = "Los indices validos van de 0 a " + std::to_string(m - 1) + ".";
    r.explicacion = "El clasico error de uno (off-by-one). Ese unico int de mas cae justo "
                    "encima de la cabecera del siguiente bloque del asignador, y el programa "
                    "suele fallar mucho despues, en un delete que no tiene nada que ver.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "strcpy no pregunta";
    r.escena = []() {
      ui::codigo({"char buffer[8];",
                  "strcpy(buffer, \"desbordamiento\");   // 14 caracteres"});
      ui::caja("", {ui::gris("strcpy copia hasta el terminador '\\0' y no recibe el tamano "
                             "del destino"),
                    ui::gris("la cadena ocupa 14 caracteres mas 1 byte de terminador")});
    };
    r.enunciado = "Cuantos bytes se escriben fuera de buffer?";
    r.respuesta = "7";
    r.verificar = verif::numero(7);
    r.pista = "15 bytes escritos (14 + terminador) menos los 8 que caben.";
    r.explicacion = "Ese es todo el problema de strcpy, gets y compania: la funcion no puede "
                    "saber cuanto espacio hay. Por eso existen snprintf, strncpy y, mejor "
                    "todavia, std::string.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Que hay justo despues";
    r.escena = []() {
      ui::codigo({"void leer_nombre() {", "    char buffer[8];",
                  "    strcpy(buffer, entrada_del_usuario);   // sin comprobar", "}"});
      ui::caja("Que suele haber en la pila justo despues de un arreglo local?",
               {"a) memoria libre que no le importa a nadie",
                "b) el canario del compilador y la direccion de retorno de la funcion",
                "c) una copia de seguridad del arreglo"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"el canario", "canario", "direccion de retorno"});
    r.pista = "Piensa a donde tiene que volver la funcion cuando termina.";
    r.explicacion = "Por eso un desbordamiento de pila no solo corrompe datos: si alcanza la "
                    "direccion de retorno puede desviar la ejecucion. El canario (-fstack-protector, "
                    "activo por defecto) se comprueba al salir y aborta con 'stack smashing "
                    "detected'.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Desbordar la cuenta, no el buffer";
    r.escena = []() {
      ui::codigo({"unsigned int n = 0;", "// ... n se queda en 0 por un error previo ...",
                  "int* p = new int[n - 1];   // cuanto es n - 1?"});
      ui::caja("", {ui::gris("n es un unsigned de 32 bits: no puede ser negativo"),
                    ui::gris("la resta se calcula modulo 2^32")});
    };
    r.enunciado = "Cuanto vale n - 1 cuando n vale 0?";
    r.respuesta = "4294967295";
    r.verificar = verif::numero(4294967295LL);
    r.pista = "2^32 - 1. Da la vuelta al maximo, igual que el 255 + 1 del nivel 3.";
    r.explicacion = "Aqui el desbordamiento de entero se convierte en uno de memoria: la "
                    "reserva es enorme y falla, o el calculo del tamano da un numero pequeno y "
                    "el buffer queda corto. Muchisimas vulnerabilidades empiezan con una "
                    "multiplicacion de tamanos que se desborda.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Las herramientas";
    r.escena = []() {
      ui::caja("Con que se encuentran estos errores?",
               {"a) leyendo el codigo con mucha atencion, que para eso somos programadores",
                "b) compilando con -fsanitize=address,undefined y ejecutando las pruebas",
                "c) no se pueden detectar: es comportamiento indefinido"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"sanitizer", "sanitizador", "asan", "valgrind"});
    r.pista = "Hay herramientas que instrumentan el programa y vigilan cada acceso.";
    r.explicacion = "AddressSanitizer detecta fugas, desbordamientos y usos despues de liberar "
                    "en tiempo de ejecucion; valgrind hace algo parecido sin recompilar. Ojo: "
                    "solo ven lo que el programa realmente ejecuta, asi que hacen falta pruebas "
                    "que pasen por el codigo sospechoso. En la carpeta ejemplos/ hay cinco "
                    "programas listos para probarlos.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Taller: aplasta un marco de pila";
    r.puntos = 30;
    r.taller = true;
    r.enunciado = "Desborda el buffer a proposito, mira que se rompe y luego hazlo bien.";
    r.respuesta = "copiar hola / copiar desbordamiento / reiniciar / copiar_seguro ... / listo";
    r.verificar = [](const std::string&) { return true; };
    r.ejecutar_taller = [](bool demo) {
      TallerBuffer taller;
      return taller.ejecutar(demo);
    };
    nivel.retos.push_back(r);
  }

  return nivel;
}
