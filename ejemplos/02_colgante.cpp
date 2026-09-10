// Ejemplo 2: un puntero colgante (use after free).
// Sin sanitizador el programa no se queja: puede imprimir 42, puede imprimir
// basura (glibc reutiliza esos bytes para sus propias listas de bloques libres)
// o puede fallar. Nada de eso es fiable: es comportamiento indefinido.
//   g++ -std=c++17 -g ejemplos/02_colgante.cpp -o /tmp/colgante && /tmp/colgante
//   g++ -std=c++17 -g -fsanitize=address ejemplos/02_colgante.cpp -o /tmp/c2 && /tmp/c2
#include <iostream>

int main() {
  int* p = new int(42);
  std::cout << "antes de delete: " << *p << "  (direccion " << p << ")\n";

  delete p;
  // p sigue guardando la misma direccion, pero ese bloque ya no es tuyo.
  std::cout << "despues de delete: " << *p << "  <- comportamiento indefinido\n";

  // La costumbre que evita el problema:
  p = nullptr;
  if (p == nullptr) std::cout << "ahora p es nullptr y se puede comprobar con un if\n";
}
