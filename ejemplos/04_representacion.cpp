// Ejemplo 4: como se ven los datos en memoria (sin errores, solo observacion).
//   g++ -std=c++17 -g ejemplos/04_representacion.cpp -o /tmp/repr && /tmp/repr
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>

struct Registro { char etiqueta; int valor; char bandera; };
struct Compacto { int valor; char etiqueta; char bandera; };

void volcar(const char* nombre, const void* p, std::size_t n) {
  const unsigned char* b = static_cast<const unsigned char*>(p);
  std::printf("%-12s %p  ", nombre, p);
  for (std::size_t i = 0; i < n; ++i) std::printf("%02x ", b[i]);
  std::printf("\n");
}

int main() {
  std::uint32_t v = 0x12345678u;
  volcar("uint32_t", &v, sizeof(v));
  std::printf("             el primer byte es 0x%02x -> %s\n",
              *reinterpret_cast<unsigned char*>(&v),
              (*reinterpret_cast<unsigned char*>(&v) == 0x78) ? "little endian" : "big endian");

  float f = -2.25f;
  std::uint32_t bits;
  std::memcpy(&bits, &f, sizeof(bits));
  std::printf("\nfloat %.2f -> 0x%08x  signo=%u exponente=%d mantisa=0x%06x\n", f, bits,
              bits >> 31, static_cast<int>((bits >> 23) & 0xFF) - 127, bits & 0x7FFFFF);

  Registro r;
  std::memset(&r, 0xAA, sizeof(r));   // el relleno quedara visible como aa
  r.etiqueta = 'A'; r.valor = 1; r.bandera = 'Z';
  std::printf("\nsizeof(Registro)=%zu  offsetof(valor)=%zu\n", sizeof(Registro),
              offsetof(Registro, valor));
  volcar("Registro", &r, sizeof(r));
  std::printf("sizeof(Compacto)=%zu  (mismos campos, otro orden)\n", sizeof(Compacto));

  int arreglo[4] = {11, 22, 33, 44};
  std::printf("\narreglo contiguo:\n");
  for (int i = 0; i < 4; ++i)
    std::printf("  &arreglo[%d] = %p   (inicio + %zu bytes)\n", i,
                static_cast<void*>(&arreglo[i]),
                reinterpret_cast<std::uintptr_t>(&arreglo[i]) -
                    reinterpret_cast<std::uintptr_t>(arreglo));
}
