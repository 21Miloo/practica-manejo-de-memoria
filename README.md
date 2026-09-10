# MemLab — práctica de manejo de memoria en C++

Juego de terminal para entender, jugando, cómo funciona la memoria en C++:
bits y bytes, representación de valores, punteros, contigüidad y el montículo.

Lo que distingue a MemLab de un cuestionario: **todo lo que muestra es memoria
real de su propio proceso**. Las direcciones, los volcados hexadecimales, los
tamaños y los desplazamientos se calculan en tiempo de ejecución con `sizeof`,
`alignof`, `offsetof` y aritmética de punteros sobre variables vivas. Si lo
ejecutas en otra máquina o dos veces seguidas, los números cambian — y esa
también es una lección.

```
┌──────────────────────────────────────────────────────────────────────────┐
│ struct Registro { char etiqueta; int valor; char bandera; };             │
├──────────────────────────────────────────────────────────────────────────┤
│ campos:            1 + 4 + 1 = 6 bytes de datos utiles                   │
│ sizeof(Registro) = 12 bytes reales                                       │
│ alignof(int) = 4: 'valor' debe empezar en una direccion multiplo de 4    │
│                                                                          │
│ el struct se lleno con 0xAA antes de asignar los campos:                 │
│ todo byte que siga en aa es relleno (padding).                           │
└──────────────────────────────────────────────────────────────────────────┘
0x55be69e41c20  │ 41 aa aa aa 56 00 00 00 │ A...V...
0x55be69e41c28  │ 5a aa aa aa             │ Z...
```

## Compilar y jugar

Con `make`:

```bash
make            # compila el juego -> ./memlab
make jugar      # compila y arranca una partida
make pruebas    # compila y ejecuta las pruebas automáticas
make demo       # recorre todos los retos mostrando las respuestas
```

O con CMake:

```bash
cmake -B build && cmake --build build && ./build/memlab
```

Solo hace falta un compilador con C++17. No hay dependencias externas.

## Opciones

| Opción | Para qué sirve |
| --- | --- |
| `--nivel N` | juega solo el nivel N (1 a 6) |
| `--semilla N` | fija la semilla: la misma partida, los mismos números |
| `--demo` | recorre todos los retos mostrando y verificando las respuestas |
| `--lista` | muestra los niveles disponibles |
| `--sin-color` | desactiva los colores ANSI |
| `--sin-guardado` | no escribe el archivo `.memlab_progreso` |

Durante la partida se puede escribir `pista` (cuesta la mitad de los puntos),
`saltar`, `mapa` para redibujar la escena, `ayuda` o `salir`. Las respuestas
numéricas se aceptan en decimal (`42`), hexadecimal (`0x2a`) o binario
(`0b101010`); cuando el reto pide un patrón de bits basta con escribirlo tal
cual (`0010 1010`).

## Los seis niveles

1. **Bit a bit** — leer un byte, consultar, encender y apagar bits con
   máscaras, desplazamientos, XOR y conteo de unos.
2. **Bytes en fila** — `sizeof`, volcado hexadecimal, *endianness* detectado en
   tiempo de ejecución, reconstruir un valor desde sus bytes, mirar la memoria
   con un `unsigned char*`.
3. **El significado de los bits** — aritmética modular sin signo, complemento a
   dos, el mismo patrón leído como `int8_t` y como `unsigned char`, y el
   desglose de un `float` en signo, exponente y mantisa (IEEE‑754).
4. **Punteros** — `&` y `*`, tamaño de un puntero, escritura a través de un
   puntero, punteros dobles, aritmética de punteros con direcciones reales,
   resta de punteros y por qué un `char*` avanza distinto que un `int*`.
5. **Contigüidad** — el arreglo como bloque continuo, cálculo de direcciones,
   `datos[i]` como `*(datos + i)`, matrices en orden por filas, alineación y
   relleno de `struct` (con el relleno visible en el volcado), y por qué la
   caché premia recorrer memoria contigua.
6. **El montículo** — pila contra montículo comparando direcciones reales,
   `new[]`/`delete[]`, fugas, punteros colgantes, RAII, y un **taller
   interactivo**: un asignador simulado donde tú reservas, escribes y liberas
   bloques mientras el juego detecta fugas, usos después de liberar, dobles
   liberaciones y accesos fuera de rango.

```
heap> reservar 4
  Reservados 16 bytes en 0x55a3c0001000 -> bloque #1
heap> escribir 1 9 99
  FUERA DE RANGO.
  El bloque #1 tiene indices 0..3. Escribir en [9] toca la direccion
  0x55a3c0001024, fuera del bloque. C++ no comprueba limites: nadie te
  avisa, y el error aparece cuando se corrompe lo que hubiera al lado.
```

## Ejemplos para experimentar con sanitizadores

En `ejemplos/` hay cinco programas cortos que provocan (o evitan) errores de
memoria de verdad. Se compilan con AddressSanitizer y UndefinedBehaviorSanitizer:

```bash
make ejemplos
./build/ejemplos/01_fuga            # AddressSanitizer: 12000 bytes leaked
./build/ejemplos/02_colgante        # uso después de liberar
./build/ejemplos/03_desbordamiento  # escribir fuera de un arreglo
./build/ejemplos/04_representacion  # observar bytes, padding y IEEE-754
./build/ejemplos/05_raii            # la versión correcta, sin new ni delete
```

Vale la pena ejecutar el 02 y el 03 **también sin sanitizador** para ver lo
importante: sin herramientas, un error de memoria a menudo no se nota. El 03
además muestra un caso que ni AddressSanitizer detecta, porque el
desbordamiento aterriza justo dentro del bloque vecino.

## Estructura

```
include/            juego.hpp (retos y verificadores), ui.hpp, util.hpp, niveles.hpp
src/
  main.cpp          opciones de línea de comandos
  juego.cpp         motor: puntuación, comandos, bucle de retos
  ui.cpp            cajas, colores, volcado hexadecimal, dibujo de bits
  util.cpp          parseo de números en varias bases y normalización de texto
  niveles/          un archivo por nivel
ejemplos/           programas para practicar con sanitizadores
tests/pruebas.cpp   pruebas automáticas sin dependencias
```

Cada reto declara su respuesta canónica junto a su verificador, y las pruebas
comprueban —con varias semillas— que la respuesta que el juego revela es
aceptada por su propio verificador. Así, si un nivel calcula mal un valor, las
pruebas fallan en vez de enseñarle algo incorrecto a quien juega.

```bash
make pruebas
# Pruebas de MemLab
# TODO BIEN: 2571/2571 comprobaciones correctas
```
