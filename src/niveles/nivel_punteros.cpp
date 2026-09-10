// Nivel 4: punteros. Direcciones reales de este proceso, aritmetica y desreferencia.
#include <cstdint>
#include <iostream>
#include <memory>

#include "niveles.hpp"
#include "ui.hpp"
#include "util.hpp"

namespace {

struct Datos {
  int x = 0;
  int nuevo = 0;
  int arreglo[8] = {0};
};

std::uintptr_t entero(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }

}  // namespace

Nivel nivel_punteros(std::mt19937& azar) {
  Nivel nivel;
  nivel.nombre = "Punteros";
  nivel.lema = "una variable que guarda una direccion";
  nivel.descripcion =
      "Un puntero es una variable normal cuyo contenido es la direccion de otra. Todas "
      "las direcciones de este nivel son reales: cambian en cada ejecucion porque el "
      "sistema operativo aleatoriza el mapa de memoria (ASLR).";

  const auto valor = [&azar]() { return util::entero_en_rango(azar, 100, 999); };
  auto d = std::make_shared<Datos>();
  d->x = valor();
  d->nuevo = valor();
  for (int i = 0; i < 8; ++i) d->arreglo[i] = valor();

  const auto escena_x = [d]() {
    ui::caja("int x = " + std::to_string(d->x) + ";   int* p = &x;",
             {
                 ui::gris("&x  (donde vive x)      = ") + ui::cian(ui::dir(&d->x)),
                 ui::gris("p   (lo que guarda p)   = ") + ui::cian(ui::dir(&d->x)),
                 ui::gris("&p  (donde vive p)      = ") +
                     ui::magenta("otra direccion distinta: el puntero tambien ocupa memoria"),
                 "",
                 ui::gris("bytes de x:"),
             });
    std::cout << ui::volcado(&d->x, sizeof(d->x), 8, 0, sizeof(d->x)) << "\n";
  };

  {
    Reto r;
    r.titulo = "Desreferenciar";
    r.escena = escena_x;
    r.enunciado = "Que imprime  std::cout << *p;  ?";
    r.respuesta = std::to_string(d->x);
    r.verificar = verif::numero(d->x);
    r.pista = "p guarda la direccion de x; el operador * va a esa direccion y lee el valor.";
    r.explicacion = "& pregunta 'donde vive'; * responde 'que hay ahi'. Son operaciones inversas.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Tamano de un puntero";
    r.escena = escena_x;
    r.enunciado = "Cuanto vale sizeof(int*) en esta maquina de 64 bits?";
    r.respuesta = std::to_string(sizeof(int*));
    r.verificar = verif::numero(static_cast<long long>(sizeof(int*)));
    r.pista = "Una direccion de 64 bits necesita 64/8 bytes.";
    r.explicacion = "Todos los punteros a datos miden lo mismo (" +
                    std::to_string(sizeof(int*)) +
                    " bytes) sin importar el tamano de lo apuntado: sizeof(char*) == sizeof(double*).";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Escribir a traves del puntero";
    r.escena = [d]() {
      ui::codigo({"int x = " + std::to_string(d->x) + ";", "int* p = &x;",
                  "*p = " + std::to_string(d->nuevo) + ";", "std::cout << x;"});
    };
    r.enunciado = "Que imprime ese programa?";
    r.respuesta = std::to_string(d->nuevo);
    r.verificar = verif::numero(d->nuevo);
    r.pista = "*p = ... escribe en la memoria de x, no en el puntero.";
    r.explicacion = "x y *p son dos nombres para el mismo byte de memoria (aliasing). Por eso "
                    "los punteros permiten que una funcion modifique variables de quien la llama.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Puntero a puntero";
    r.escena = [d]() {
      ui::codigo({"int x  = " + std::to_string(d->x) + ";", "int* p  = &x;", "int** pp = &p;"});
      ui::caja("Tres eslabones",
               {ui::gris("pp  ->  p  ->  x"), ui::gris("**pp lee dos veces: primero p, luego x")});
    };
    r.enunciado = "Que imprime  std::cout << **pp;  ?";
    r.respuesta = std::to_string(d->x);
    r.verificar = verif::numero(d->x);
    r.pista = "*pp es p; **pp es *p.";
    r.explicacion = "Cada * quita un nivel de indireccion. Un int** es tan solo un puntero mas, "
                    "que ocupa los mismos " + std::to_string(sizeof(int**)) + " bytes.";
    nivel.retos.push_back(r);
  }
  {
    const std::uintptr_t base = entero(&d->arreglo[0]);
    const std::uintptr_t esperado = entero(&d->arreglo[1]);
    Reto r;
    r.titulo = "Aritmetica de punteros";
    r.escena = [d, base]() {
      ui::caja("int datos[8];   int* p = &datos[0];",
               {ui::gris("p = ") + ui::cian(ui::dir(base)),
                ui::gris("sizeof(int) = ") + std::to_string(sizeof(int)) + ui::gris(" bytes")});
      std::cout << ui::volcado(d->arreglo, sizeof(d->arreglo), 8, 0, sizeof(int)) << "\n";
    };
    r.enunciado = "Que direccion tiene p + 1? (en hex)";
    r.respuesta = ui::dir(esperado);
    r.verificar = verif::direccion(esperado);
    r.pista = "p + 1 no suma 1 byte: suma sizeof(int) = " + std::to_string(sizeof(int)) +
              " bytes a " + ui::dir(base) + ".";
    r.explicacion = "La aritmetica de punteros se mide en elementos, no en bytes. El compilador "
                    "multiplica por sizeof(T) por ti.";
    nivel.retos.push_back(r);
  }
  {
    const std::uintptr_t base = entero(&d->arreglo[0]);
    Reto r;
    r.titulo = "Cambiar de escala";
    r.escena = [base]() {
      ui::caja("Dos punteros a la misma direccion, distinto tipo",
               {ui::gris("int*  p = &datos[0];   p = ") + ui::cian(ui::dir(base)),
                ui::gris("char* c = (char*)p;    c = ") + ui::cian(ui::dir(base)),
                "",
                ui::gris("apuntan al mismo byte, pero avanzan a pasos distintos")});
    };
    r.enunciado = "Que direccion tiene c + 1, siendo c un char*? (en hex)";
    r.respuesta = ui::dir(base + 1);
    r.verificar = verif::direccion(base + 1);
    r.pista = "sizeof(char) es 1, asi que el paso es de un byte.";
    r.explicacion = "El tipo del puntero define el tamano del paso y como se interpretan los "
                    "bytes leidos. Convertir a char* es la forma de recorrer memoria byte a byte.";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Restar punteros";
    r.escena = [d]() {
      ui::caja("int datos[8];",
               {ui::gris("int* p = &datos[2];   p = ") + ui::cian(ui::dir(&d->arreglo[2])),
                ui::gris("int* q = &datos[6];   q = ") + ui::cian(ui::dir(&d->arreglo[6])),
                "",
                ui::gris("diferencia en bytes: ") +
                    std::to_string(entero(&d->arreglo[6]) - entero(&d->arreglo[2]))});
    };
    r.enunciado = "Cuanto vale q - p?";
    r.respuesta = "4";
    r.verificar = verif::numero(4);
    r.pista = "El resultado se mide en elementos: divide la diferencia de bytes entre "
              "sizeof(int).";
    r.explicacion = "Restar punteros da un ptrdiff_t en elementos. Solo esta definido si ambos "
                    "apuntan al mismo arreglo (o uno mas alla del final).";
    nivel.retos.push_back(r);
  }
  {
    Reto r;
    r.titulo = "Punteros peligrosos";
    r.escena = []() {
      ui::caja("Tres punteros que no se pueden desreferenciar",
               {"a) uno nulo: int* p = nullptr;",
                "b) uno sin inicializar: int* p;  // contiene basura",
                "c) uno colgante: apunta a memoria ya liberada"});
      ui::caja("Que ocurre al hacer  *p  con p == nullptr ?",
               {"a) devuelve 0", "b) comportamiento indefinido (en la practica, un segfault)",
                "c) el compilador siempre lo detecta y da error"});
    };
    r.enunciado = "Responde a, b o c.";
    r.respuesta = "b";
    r.verificar = verif::opcion('b', {"comportamiento indefinido", "undefined behavior", "segfault"});
    r.pista = "El estandar no promete nada; el sistema operativo suele matar el proceso.";
    r.explicacion = "El nulo es el mejor de los tres: falla de inmediato y es comprobable con "
                    "if (p). El colgante y el no inicializado pueden parecer funcionar durante "
                    "mucho tiempo y corromper datos en silencio.";
    nivel.retos.push_back(r);
  }

  return nivel;
}
