// Ejemplo 3: escribir fuera de un arreglo (heap buffer overflow).
// C++ no comprueba limites. AddressSanitizer atrapa muchos de estos accesos,
// pero no todos: si el desbordamiento aterriza justo dentro de otro bloque
// valido, nadie se entera y solo queda un dato corrompido.
//
//   sin sanitizador:  g++ -std=c++17 -g ejemplos/03_desbordamiento.cpp -o /tmp/des
//   con sanitizador:  g++ -std=c++17 -g -fsanitize=address ejemplos/03_desbordamiento.cpp -o /tmp/des
//   ./tmp/des
#include <iostream>

int main() {
  int* a = new int[4]{1, 2, 3, 4};
  int* b = new int[4]{10, 20, 30, 40};

  const long distancia = (b - a);
  std::cout << "a esta en " << a << ", b esta en " << b << "\n";
  std::cout << "entre ambos hay " << distancia << " ints de distancia\n\n";

  // Caso 1: el indice se pasa tanto que cae dentro del bloque vecino.
  std::cout << "b[0] antes:    " << b[0] << "\n";
  a[distancia] = 999;   // sigue siendo memoria valida... pero no es de 'a'
  std::cout << "b[0] despues:  " << b[0]
            << "   <- corrompimos otro objeto sin que nadie protestara\n\n";

  // Caso 2: justo un elemento mas alla del final.
  std::cout << "ahora escribimos en a[4], un solo paso fuera del bloque...\n";
  std::cout.flush();
  a[4] = 123;           // aqui AddressSanitizer si aborta el programa
  std::cout << "...y sin sanitizador el programa sigue como si nada.\n";

  delete[] a;
  delete[] b;
}
