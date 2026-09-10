// Ejemplo 1: una fuga de memoria.
// Compilar y ejecutar con el detector de fugas:
//   g++ -std=c++17 -g -fsanitize=address ejemplos/01_fuga.cpp -o /tmp/fuga && /tmp/fuga
#include <iostream>

bool hay_error() { return true; }

void procesar() {
  int* datos = new int[1000];   // 4000 bytes pedidos al monticulo
  datos[0] = 7;
  if (hay_error()) return;      // salida temprana: nadie libera 'datos'
  delete[] datos;               // esta linea nunca se ejecuta
}

int main() {
  for (int i = 0; i < 3; ++i) procesar();
  std::cout << "Termine 'bien'... pero acabo de perder 12000 bytes.\n";
  std::cout << "AddressSanitizer los reporta como 'detected memory leaks'.\n";
}
