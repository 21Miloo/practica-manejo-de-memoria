#!/usr/bin/env python3
"""Convierte la salida con colores ANSI de MemLab en una imagen SVG.

Sirve para generar las capturas de la version de terminal que aparecen en el
README, sin depender de una herramienta externa de capturas.

Uso:
    ./memlab --nivel 1 --semilla 7 < respuestas.txt | python3 docs/ansi_a_svg.py salida.svg
"""
import re
import sys

# Paleta: los mismos tonos que usa la version web.
COLORES = {
    "31": "#ff7b72", "32": "#56d364", "33": "#e3b341", "34": "#79c0ff",
    "35": "#d2a8ff", "36": "#56d4dd", "90": "#8b98a8", "1": "#ffffff",
}
FONDO = "#0d1117"
TEXTO = "#d7e0ea"
ANCHO_CARACTER = 8.4
ALTO_LINEA = 18.5
MARGEN = 16

SECUENCIA = re.compile(r"\033\[([0-9;]*)m")


def escapar(texto):
    return texto.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def trozos(linea):
    """Parte una linea en (texto, color, negrita) siguiendo los codigos ANSI."""
    salida, posicion, color, negrita = [], 0, None, False
    for marca in SECUENCIA.finditer(linea):
        if marca.start() > posicion:
            salida.append((linea[posicion:marca.start()], color, negrita))
        codigos = [c for c in marca.group(1).split(";") if c] or ["0"]
        for codigo in codigos:
            if codigo == "0":
                color, negrita = None, False
            elif codigo == "1":
                negrita = True
            elif codigo in COLORES:
                color = COLORES[codigo]
        posicion = marca.end()
    if posicion < len(linea):
        salida.append((linea[posicion:], color, negrita))
    return salida


def main():
    destino = sys.argv[1] if len(sys.argv) > 1 else "salida.svg"
    lineas = sys.stdin.read().rstrip("\n").split("\n")

    columnas = max((len(SECUENCIA.sub("", l)) for l in lineas), default=80)
    ancho = int(columnas * ANCHO_CARACTER) + MARGEN * 2
    alto = int(len(lineas) * ALTO_LINEA) + MARGEN * 2

    partes = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{ancho}" height="{alto}" '
        f'viewBox="0 0 {ancho} {alto}" font-family="ui-monospace, Menlo, Consolas, monospace" '
        f'font-size="13">',
        f'<rect width="{ancho}" height="{alto}" rx="8" fill="{FONDO}"/>',
    ]
    for numero, linea in enumerate(lineas):
        y = MARGEN + numero * ALTO_LINEA + 13
        spans, columna = [], 0
        for texto, color, negrita in trozos(linea):
            if texto:
                x = MARGEN + columna * ANCHO_CARACTER
                estilo = f' fill="{color or TEXTO}"'
                if negrita:
                    estilo += ' font-weight="bold"'
                # textLength fija el ancho exacto de cada trozo: sin esto los
                # caracteres de dibujo de cajas (que no miden lo mismo que una
                # letra) descuadran los bordes.
                ancho_trozo = len(texto) * ANCHO_CARACTER
                spans.append(
                    f'<tspan x="{x:.1f}"{estilo} xml:space="preserve" '
                    f'textLength="{ancho_trozo:.1f}" lengthAdjust="spacingAndGlyphs">'
                    f'{escapar(texto)}</tspan>')
                columna += len(texto)
        if spans:
            partes.append(f'<text y="{y:.1f}">{"".join(spans)}</text>')
    partes.append("</svg>")

    with open(destino, "w", encoding="utf-8") as archivo:
        archivo.write("\n".join(partes))
    print(f"escrito {destino} ({ancho}x{alto})")


if __name__ == "__main__":
    main()
