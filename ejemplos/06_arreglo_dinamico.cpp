// Ejemplo 6: un arreglo dinamico que crece, hecho a mano y con direcciones
// reales. Es la version de verdad del taller del nivel 7.
//   g++ -std=c++17 -g -fsanitize=address ejemplos/06_arreglo_dinamico.cpp -o /tmp/arr && /tmp/arr
#include <iostream>
#include <vector>

// Un vector minimo: solo lo justo para ver el mecanismo.
class ArregloDinamico {
 public:
  explicit ArregloDinamico(int capacidad_inicial = 1)
      : datos_(new int[capacidad_inicial]), tam_(0), capacidad_(capacidad_inicial) {}

  ~ArregloDinamico() { delete[] datos_; }   // RAII: sin esto, fuga segura

  // Sin copia: si se copiara el puntero, dos objetos liberarian el mismo bloque
  // (doble liberacion). Esta es la "regla de tres/cinco" de C++.
  ArregloDinamico(const ArregloDinamico&) = delete;
  ArregloDinamico& operator=(const ArregloDinamico&) = delete;

  void agregar(int v) {
    if (tam_ == capacidad_) crecer(capacidad_ * 2);
    datos_[tam_++] = v;
  }

  int tam() const { return tam_; }
  int capacidad() const { return capacidad_; }
  const int* datos() const { return datos_; }

 private:
  void crecer(int nueva_capacidad) {
    std::cout << "  crecer: " << capacidad_ << " -> " << nueva_capacidad
              << "   (copiando " << tam_ << " elementos)\n";
    int* nuevo = new int[nueva_capacidad];              // 1. reservar
    for (int i = 0; i < tam_; ++i) nuevo[i] = datos_[i];  // 2. copiar
    delete[] datos_;                                     // 3. liberar el viejo
    datos_ = nuevo;
    capacidad_ = nueva_capacidad;
    std::cout << "          bloque nuevo en " << static_cast<void*>(datos_) << "\n";
  }

  int* datos_;
  int tam_;
  int capacidad_;
};

int main() {
  std::cout << "--- arreglo dinamico hecho a mano ---\n";
  ArregloDinamico a;
  for (int i = 1; i <= 8; ++i) {
    a.agregar(i * 10);
    std::cout << "agregado " << i * 10 << ": tam=" << a.tam()
              << " capacidad=" << a.capacidad() << " en "
              << static_cast<const void*>(a.datos()) << "\n";
  }

  std::cout << "\n--- lo mismo con std::vector ---\n";
  std::vector<int> v;
  for (int i = 1; i <= 8; ++i) {
    v.push_back(i * 10);
    std::cout << "push_back " << i * 10 << ": tam=" << v.size()
              << " capacidad=" << v.capacity() << " en "
              << static_cast<const void*>(v.data()) << "\n";
  }
  std::cout << "\nFijate como cambia la direccion en cada crecimiento: por eso un\n"
               "puntero a un elemento deja de ser valido cuando el arreglo crece.\n";
}
