#!/usr/bin/env python3
"""Junta la pagina, los estilos, el codigo y el motor en un unico HTML.

El resultado (web/memlab.html) no pide ningun archivo mas: el modulo
WebAssembly viaja dentro del propio JavaScript, asi que se puede abrir con
doble clic desde el disco, sin servidor y sin conexion.
"""
import pathlib
import re

AQUI = pathlib.Path(__file__).resolve().parent

pagina = (AQUI / "index.html").read_text(encoding="utf-8")
estilos = (AQUI / "estilo.css").read_text(encoding="utf-8")
aplicacion = (AQUI / "app.js").read_text(encoding="utf-8")
motor = (AQUI / "wasm" / "memlab_unico.js").read_text(encoding="utf-8")

# Los </script> que pueda haber dentro del texto romperian la etiqueta que los
# contiene: se parten en dos trozos equivalentes para el navegador.
proteger = lambda texto: texto.replace("</script>", "<\\/script>")

pagina = pagina.replace(
    '<link rel="stylesheet" href="estilo.css">',
    f"<style>\n{estilos}\n</style>")
# El reemplazo se pasa como funcion: el codigo del motor lleva barras
# invertidas que re.sub interpretaria como secuencias de escape.
incrustado = (f"<script>\n{proteger(motor)}\n</script>\n"
              f"<script>\n{proteger(aplicacion)}\n</script>")
pagina, sustituciones = re.subn(
    r'<script src="wasm/memlab\.js"></script>\s*<script src="app\.js"></script>',
    lambda _: incrustado,
    pagina)
if sustituciones != 1:
    raise SystemExit("no encontre las etiquetas <script> de index.html")

destino = AQUI / "memlab.html"
destino.write_text(pagina, encoding="utf-8")
print(f"escrito {destino} ({destino.stat().st_size / 1024:.0f} KB)")
