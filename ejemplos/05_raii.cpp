// Ejemplo 5: la version correcta de los ejemplos 1 y 2, con RAII.
// No hay new ni delete a la vista y no puede haber fugas.
//   g++ -std=c++17 -g -fsanitize=address ejemplos/05_raii.cpp -o /tmp/raii && /tmp/raii
#include <iostream>
#include <memory>
#include <vector>

bool hay_error() { return true; }

void procesar() {
  auto datos = std::make_unique<int[]>(1000);   // se libera solo
  datos[0] = 7;
  if (hay_error()) return;                      // el destructor corre igual
}

int main() {
  for (int i = 0; i < 3; ++i) procesar();

  std::vector<int> v{1, 2, 3};
  v.push_back(4);                               // crece y copia por ti
  std::cout << "v tiene " << v.size() << " elementos y reserva para "
            << v.capacity() << "\n";
  std::cout << "datos contiguos desde " << static_cast<void*>(v.data()) << "\n";
  std::cout << "Sin fugas: el sanitizador no reporta nada.\n";
}
