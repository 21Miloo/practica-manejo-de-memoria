// MemLab web: conecta el motor C++ (compilado a WebAssembly) con la pagina.
//
// El motor no sabe nada de HTML: emite lineas de texto con marcas de control
// que aqui se convierten en nodos del DOM. El texto nunca se interpreta como
// HTML (se inserta con textContent), asi que no hay forma de inyectar marcado.
import crearMemLab from './wasm/memlab.js';

// Marcas de estructura; deben coincidir con las de include/ui.hpp.
const MARCA = {
  colorInicio: '\x02',
  colorTexto: '\x03',
  colorFin: '\x04',
  caja: '\x05',
  fin: '\x06',
  titulo: '\x07',
  codigo: '\x08',
  mono: '\x0b',
  separador: '\x0e',
  estado: '\x10',
  finPartida: '\x11',
  pregunta: '\x12',
};

const flujo = document.getElementById('flujo');
const campo = document.getElementById('campo');
const prompt = document.getElementById('prompt');
const formulario = document.getElementById('formulario');
const botonEnviar = document.getElementById('boton-enviar');
const botonNueva = document.getElementById('boton-nueva');
const selectorNivel = document.getElementById('selector-nivel');
const campoSemilla = document.getElementById('campo-semilla');
const etiquetaNivel = document.getElementById('etiqueta-nivel');
const etiquetaPuntos = document.getElementById('etiqueta-puntos');
const barra = document.getElementById('barra-relleno');
const atajos = document.getElementById('atajos');

// --- traduccion de una linea del motor a nodos del DOM --------------------

// Convierte "\x02CODIGO\x03texto\x04" en <span class="c-CODIGO">texto</span>,
// dejando el resto como texto plano.
function nodosDeTexto(linea) {
  const trozo = document.createDocumentFragment();
  let i = 0;
  while (i < linea.length) {
    const inicio = linea.indexOf(MARCA.colorInicio, i);
    if (inicio === -1) {
      trozo.append(document.createTextNode(linea.slice(i)));
      break;
    }
    if (inicio > i) trozo.append(document.createTextNode(linea.slice(i, inicio)));

    const sep = linea.indexOf(MARCA.colorTexto, inicio);
    const fin = linea.indexOf(MARCA.colorFin, sep);
    if (sep === -1 || fin === -1) {   // marca incompleta: se muestra tal cual
      trozo.append(document.createTextNode(linea.slice(inicio)));
      break;
    }
    const span = document.createElement('span');
    span.className = 'c-' + linea.slice(inicio + 1, sep);
    span.append(nodosDeTexto(linea.slice(sep + 1, fin)));   // colores anidados
    trozo.append(span);
    i = fin + 1;
  }
  return trozo;
}

// --- construccion del flujo ----------------------------------------------

let bloqueAbierto = null;   // caja, bloque de codigo o bloque monoespaciado

function agregar(nodo) {
  (bloqueAbierto ?? flujo).append(nodo);
  window.scrollTo({ top: document.body.scrollHeight, behavior: 'smooth' });
}

function abrirCaja(encabezado) {
  const caja = document.createElement('section');
  caja.className = 'caja';
  if (encabezado.trim() !== '') {
    const cab = document.createElement('div');
    cab.className = 'caja-encabezado';
    cab.append(nodosDeTexto(encabezado));
    caja.append(cab);
  }
  const cuerpo = document.createElement('div');
  cuerpo.className = 'caja-cuerpo';
  caja.append(cuerpo);
  flujo.append(caja);
  bloqueAbierto = cuerpo;
}

function abrirPreformateado(clase) {
  const pre = document.createElement('pre');
  pre.className = clase;
  flujo.append(pre);
  bloqueAbierto = pre;
}

function procesarLinea(linea) {
  const marca = linea.charAt(0);
  const resto = linea.slice(1);

  switch (marca) {
    case MARCA.estado:
      actualizarEstado(resto);
      return;
    case MARCA.finPartida:
      terminarPartida();
      return;
    case MARCA.pregunta: {
      bloqueAbierto = null;
      const p = document.createElement('div');
      p.className = 'pregunta';
      p.append(nodosDeTexto(resto));
      flujo.append(p);
      window.scrollTo({ top: document.body.scrollHeight, behavior: 'smooth' });
      return;
    }
    case MARCA.titulo: {
      bloqueAbierto = null;
      const h = document.createElement('h2');
      h.className = 'titulo';
      h.append(nodosDeTexto(resto));
      flujo.append(h);
      return;
    }
    case MARCA.caja:
      bloqueAbierto = null;
      abrirCaja(resto);
      return;
    case MARCA.codigo:
      bloqueAbierto = null;
      abrirPreformateado('codigo');
      return;
    case MARCA.mono:
      bloqueAbierto = null;
      abrirPreformateado('mono');
      return;
    case MARCA.fin:
      bloqueAbierto = null;
      return;
    case MARCA.separador: {
      bloqueAbierto = null;
      const hr = document.createElement('hr');
      hr.className = 'separador';
      flujo.append(hr);
      return;
    }
    default:
      break;
  }

  if (bloqueAbierto instanceof HTMLPreElement) {
    bloqueAbierto.append(nodosDeTexto(linea), document.createTextNode('\n'));
    return;
  }

  const div = document.createElement('div');
  div.className = bloqueAbierto ? 'fila' : 'linea';
  div.append(nodosDeTexto(linea));
  agregar(div);
}

// El motor escribe por lineas, pero print() puede entregarlas sueltas: se
// acumulan aqui para no perder ninguna.
function escribir(texto) {
  for (const linea of String(texto).split('\n')) procesarLinea(linea);
}

// --- progreso -------------------------------------------------------------

function actualizarEstado(json) {
  let e;
  try { e = JSON.parse(json); } catch { return; }
  etiquetaNivel.textContent = `Nivel ${e.nivel} · ${e.nombreNivel} · reto ${e.reto}/${e.retos}`;
  etiquetaPuntos.textContent = `${e.puntos} / ${e.maximo} puntos`;
  barra.style.width = e.maximo > 0 ? `${(e.puntos / e.maximo) * 100}%` : '0%';
}

// --- entrada --------------------------------------------------------------

let resolverEntrada = null;

function terminarPartida() {
  resolverEntrada = null;
  bloqueAbierto = null;
  campo.disabled = true;
  botonEnviar.disabled = true;
  atajos.replaceChildren();
  prompt.textContent = 'fin';
  const aviso = document.createElement('div');
  aviso.className = 'linea c-90';
  aviso.textContent = 'Partida terminada. Pulsa "Nueva partida" para volver a empezar.';
  flujo.append(aviso);
}

function pedirLinea(textoPrompt) {
  prompt.textContent = textoPrompt.trim();
  ponerAtajos(textoPrompt.trim());
  campo.disabled = false;
  botonEnviar.disabled = false;
  campo.focus();
  return new Promise((resolve) => { resolverEntrada = resolve; });
}

function enviar(texto) {
  if (!resolverEntrada) return;
  const eco = document.createElement('div');
  eco.className = 'eco';
  eco.textContent = `${prompt.textContent} ${texto}`;
  bloqueAbierto = null;
  flujo.append(eco);

  const responder = resolverEntrada;
  resolverEntrada = null;
  campo.value = '';
  campo.disabled = true;
  botonEnviar.disabled = true;
  responder(texto);
}

// Botones rapidos: dependen de quien este pidiendo la linea.
const ATAJOS = {
  'memlab>': ['pista', 'saltar', 'mapa', 'ayuda'],
  'heap>': ['reservar 4', 'mapa', 'liberar 1', 'ayuda', 'listo'],
  'arreglo>': ['ver', 'reservar 4', 'copiar', 'liberar_viejo', 'ayuda', 'listo'],
  'buffer>': ['ver', 'copiar hola', 'reiniciar', 'ayuda', 'listo'],
};

function ponerAtajos(textoPrompt) {
  atajos.replaceChildren();
  for (const comando of ATAJOS[textoPrompt] ?? []) {
    const boton = document.createElement('button');
    boton.type = 'button';
    boton.textContent = comando;
    boton.addEventListener('click', () => {
      // Los comandos con argumento se dejan escritos para poder ajustarlos.
      if (comando.includes(' ')) { campo.value = comando; campo.focus(); }
      else enviar(comando);
    });
    atajos.append(boton);
  }
}

formulario.addEventListener('submit', (evento) => {
  evento.preventDefault();
  if (!campo.disabled) enviar(campo.value);
});

// --- arranque -------------------------------------------------------------

// Cada partida corre en su propia instancia del modulo: una ejecucion que se
// quedo suspendida esperando entrada no se puede reanudar ni reutilizar, asi
// que se abandona y se crea otra. 'generacion' descarta la salida de las
// instancias viejas.
let generacion = 0;

async function crearInstancia(miGeneracion) {
  return crearMemLab({
    print: (t) => { if (miGeneracion === generacion) escribir(t); },
    printErr: (t) => console.error(t),
    pedirLinea: (p) => (miGeneracion === generacion
      ? pedirLinea(p)
      : new Promise(() => {})),   // instancia abandonada: se queda dormida
  });
}

async function nuevaPartida() {
  if (resolverEntrada) {          // suelta la partida anterior
    const responder = resolverEntrada;
    resolverEntrada = null;
    responder('salir');
  }
  generacion += 1;
  const miGeneracion = generacion;

  flujo.replaceChildren();
  bloqueAbierto = null;
  campo.disabled = true;
  botonEnviar.disabled = true;
  etiquetaNivel.textContent = 'Cargando WebAssembly...';

  const modulo = await crearInstancia(miGeneracion);
  if (miGeneracion !== generacion) return;

  if (selectorNivel.options.length === 1) {
    const total = modulo.ccall('memlab_total_niveles', 'number', [], []);
    for (let i = 1; i <= total; i += 1) {
      const opcion = document.createElement('option');
      opcion.value = String(i);
      opcion.textContent = `Nivel ${i}`;
      selectorNivel.append(opcion);
    }
  }

  const semilla = Number(campoSemilla.value) || 0;
  const nivel = Number(selectorNivel.value) || 0;
  modulo.ccall('memlab_jugar', null, ['number', 'number'], [semilla, nivel]);
}

// Mantiene la barra de progreso pegada justo debajo de la cabecera.
function medirCabecera() {
  const alto = document.querySelector('.cabecera').offsetHeight;
  document.documentElement.style.setProperty('--alto-cabecera', `${alto}px`);
}
window.addEventListener('resize', medirCabecera);
medirCabecera();

botonNueva.addEventListener('click', () => { nuevaPartida(); });
nuevaPartida().catch((e) => {
  etiquetaNivel.textContent = 'No se pudo cargar el modulo WebAssembly';
  console.error(e);
});
