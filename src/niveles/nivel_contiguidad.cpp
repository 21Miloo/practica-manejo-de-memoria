// Nivel 5: contiguidad. Arreglos, matrices, alineacion y relleno (padding).
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

// Un struct con relleno visible: char, int, char no caben apretados.
struct Registro {
  char etiqueta;
  int valor;
  char bandera;
};

// Los mismos campos, ordenados de mayor a menor alineacion.
struct Compacto {
  int valor;
  char etiqueta;
  char bandera;
};

struct Datos {
  int datos[8] = {0};
  int matriz[3][4] = {{0}};
  Registro registro{};
};

std::uintptr_t entero(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }

}  // namespace

Nivel nivel_contiguidad(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "Contiguidad";
  nivel.lema = "todo junto y en orden";
  nivel.descripcion =
      "Un arreglo no es una lista de cajas sueltas: es un bloque continuo de bytes. De "
      "ahi salen el indexado en tiempo constante, el rendimiento de la cache y tambien "
      "el relleno que el compilador inserta dentro de los structs.";

  const auto valor = [&azar]() { return util::entero_en_rango(azar, 10, 99); };
  auto d = std::make_shared<Datos>();
  for (int i = 0; i < 8; ++i) d->datos[i] = valor();
  for (int f = 0; f < 3; ++f)
    for (int c = 0; c < 4; ++c) d->matriz[f][c] = valor();

  // Rellenamos el struct con 0xAA antes de asignar los campos: los bytes que
  // sigan valiendo 0xAA en el volcado son exactamente el relleno.
  std::memset(&d->registro, 0xAA, sizeof(d->registro));
  d->registro.etiqueta = 'A';
  d->registro.valor = valor();
  d->registro.bandera = 'Z';

  const auto escena_arreglo = [d]() {
    std::vector<std::string> celdas_valores, celdas_indices;
    for (int i = 0; i < 8; ++i) {
      celdas_valores.push_back(std::to_string(d->datos[i]));
      celdas_indices.push_back("[" + std::to_string(i) + "]");
    }
    ui::caja("int datos[8];",
             {ui::gris("inicio  = ") + ui::cian(ui::dir(&d->datos[0])),
              ui::gris("sizeof  = ") + std::to_string(sizeof(d->datos)) + ui::gris(" bytes"),
              "",
              ui::gris(ui::celdas(celdas_indices)), ui::fuerte(ui::celdas(celdas_valores))});
    std::cout << ui::volcado(d->datos, sizeof(d->datos), 8, 0, 0) << "\n";
  };

  {
    Reto r;
    r.titulo = "Un bloque, no ocho cajas";
    r.escena = escena_arreglo;
    r.enunciado = "Cuantos bytes ocupa en total  int datos[8];  ?";
    r.respuesta = std::to_string(sizeof(d->datos));
    r.verificar = verif::numero(static_cast<long long>(sizeof(d->datos)));
    r.pista = "8 elementos x sizeof(int).";
    r.explicacion = "Un arreglo no guarda longitud, ni punteros, ni cabecera: son los elementos "
                    "pegados uno detras de otro y nada mas.";
    nivel.retos.push_back(r);
  }
  {
    const std::uintptr_t esperado = entero(&d->datos[3]);
    Reto r;
    r.titulo = "Calcular una direccion";
    r.escena = escena_arreglo;
    r.enunciado = "Que direccion tiene &datos[3]? (en hex)";
    r.respuesta = ui::dir(esperado);
    r.verificar = verif::direccion(esperado);
    r.pista = "inicio + 3 * sizeof(int) = " + ui::dir(entero(&d->datos[0])) + " + " +
              std::to_string(3 * sizeof(int)) + " bytes.";
    r.explicacion = "Esta formula (inicio + indice * tamano) es todo lo que hace datos[i]. Por "
                    "eso el acceso por indice es O(1) y por eso no hay comprobacion de limites.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Azucar sintactico";
    r.escena = []() {
      ui::caja("El compilador traduce el indexado a aritmetica de punteros",
               {ui::gris("datos[i]   se convierte en   *(datos + i)"),
                ui::gris("y como la suma es conmutativa, i[datos] tambien compila...")});
    };
    r.enunciado = "En bytes, cuanto suma el compilador a 'datos' para llegar a datos[5]?";
    r.respuesta = std::to_string(5 * sizeof(int));
    r.verificar = verif::numero(static_cast<long long>(5 * sizeof(int)));
    r.pista = "5 elementos x sizeof(int).";
    r.explicacion = "Al escribir datos[5] pides el elemento 5, no el byte 5: el escalado por "
                    "sizeof(int) lo pone el compilador.";
    nivel.retos.push_back(r);
  }
  {
    const std::uintptr_t base = entero(&d->matriz[0][0]);
    const std::uintptr_t esperado = entero(&d->matriz[1][2]);
    const long long en_elementos =
        static_cast<long long>((esperado - base) / sizeof(int));
    Reto r;
    r.titulo = "Una matriz tambien es una fila";
    r.escena = [d, base]() {
      std::vector<std::string> lineas{
          ui::gris("&matriz[0][0] = ") + ui::cian(ui::dir(base)),
          ui::gris("sizeof(matriz) = ") + std::to_string(sizeof(d->matriz)) + ui::gris(" bytes"),
          "",
          ui::gris("en memoria las filas van una detras de otra (orden por filas):")};
      std::string fila;
      for (int f = 0; f < 3; ++f) {
        std::vector<std::string> celdas;
        for (int c = 0; c < 4; ++c) celdas.push_back(std::to_string(d->matriz[f][c]));
        lineas.push_back(ui::gris("fila " + std::to_string(f) + " ") + ui::celdas(celdas));
      }
      ui::caja("int matriz[3][4];", lineas);
    };
    r.enunciado = "Cuantos ints hay entre &matriz[0][0] y &matriz[1][2]?";
    r.respuesta = std::to_string(en_elementos);
    r.verificar = verif::numero(en_elementos);
    r.pista = "Salta una fila completa (4 elementos) y luego 2 columnas.";
    r.explicacion = "matriz[f][c] esta en el desplazamiento f * columnas + c. C++ usa orden por "
                    "filas, asi que recorrer por filas es contiguo y recorrer por columnas salta.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "El relleno que no pediste";
    r.escena = [d]() {
      ui::caja("struct Registro { char etiqueta; int valor; char bandera; };",
               {
                   ui::gris("campos:            1 + 4 + 1 = 6 bytes de datos utiles"),
                   ui::gris("sizeof(Registro) = ") + ui::fuerte(std::to_string(sizeof(Registro))) +
                       ui::gris(" bytes reales"),
                   ui::gris("alignof(int) = ") + std::to_string(alignof(int)) +
                       ui::gris(": 'valor' debe empezar en una direccion multiplo de ") +
                       std::to_string(alignof(int)),
                   "",
                   ui::gris("el struct se lleno con 0xAA antes de asignar los campos:"),
                   ui::gris("todo byte que siga en aa es relleno (padding)."),
               });
      std::cout << ui::volcado(&d->registro, sizeof(Registro), 8, 0, 0) << "\n";
    };
    r.enunciado = "Cuanto vale sizeof(Registro)?";
    r.respuesta = std::to_string(sizeof(Registro));
    r.verificar = verif::numero(static_cast<long long>(sizeof(Registro)));
    r.pista = "Cuenta tambien los bytes 0xAA del volcado: son relleno, pero ocupan.";
    r.explicacion = "El compilador alinea cada campo y ademas rellena el final para que un "
                    "arreglo de structs mantenga alineado a todos sus elementos.";
    nivel.retos.push_back(r);
  }
  {
    const long long desplazamiento = static_cast<long long>(offsetof(Registro, valor));
    Reto r;
    r.titulo = "Donde empieza cada campo";
    r.enunciado = "Cuanto vale offsetof(Registro, valor), en bytes desde el inicio del struct?";
    r.respuesta = std::to_string(desplazamiento);
    r.verificar = verif::numero(desplazamiento);
    r.pista = "'etiqueta' ocupa 1 byte, pero 'valor' no puede empezar en el byte 1: necesita "
              "una direccion multiplo de " + std::to_string(alignof(int)) + ".";
    r.explicacion = "offsetof (de <cstddef>) te dice exactamente donde cae cada campo. Es lo que "
                    "necesitas cuando serializas un struct o lo mapeas sobre un buffer.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Reordenar para ahorrar";
    r.escena = []() {
      ui::codigo({"struct Compacto { int valor; char etiqueta; char bandera; };",
                  "// los mismos tres campos, en otro orden"});
      ui::caja("", {ui::gris("sizeof(Registro) = ") + std::to_string(sizeof(Registro)) +
                    ui::gris(" bytes")});
    };
    r.enunciado = "Cuanto vale sizeof(Compacto)?";
    r.respuesta = std::to_string(sizeof(Compacto));
    r.verificar = verif::numero(static_cast<long long>(sizeof(Compacto)));
    r.pista = "Pon el int al principio y los dos char juntos al final; luego redondea al "
              "multiplo de " + std::to_string(alignof(Compacto)) + " mas cercano.";
    r.explicacion = "Mismos datos, " +
                    std::to_string(sizeof(Registro) - sizeof(Compacto)) +
                    " bytes menos, solo por ordenar los campos de mayor a menor alineacion. "
                    "En un arreglo de un millon de elementos eso se nota.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Por que importa la contiguidad";
    r.escena = []() {
      ui::caja("La memoria no se lee de a un byte",
               {ui::gris("La CPU trae la RAM a la cache en lineas de 64 bytes."),
                ui::gris("Pedir un int trae de paso sus 15 vecinos."),
                "",
                "Por que recorrer un vector<int> suele ser mas rapido que una lista enlazada,",
                "aunque ambas hagan N operaciones?",
                "",
                "a) porque el vector tiene menos elementos",
                "b) porque cada acceso al vector aprovecha datos ya traidos a la cache",
                "c) porque las listas enlazadas usan mas CPU al sumar"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"por la cache", "localidad", "localidad espacial"});
    r.pista = "Piensa en cuantas lineas de cache distintas toca cada recorrido.";
    r.explicacion = "Se llama localidad espacial. Los nodos de una lista estan dispersos: cada "
                    "salto puede costar un fallo de cache, cientos de veces mas caro que sumar.";
    nivel.retos.push_back(r);
  }

  return nivel;
}
