```
███████╗██╗   ██╗███╗   ██╗███████╗██╗     ███████╗███████╗████████╗
██╔════╝██║   ██║████╗  ██║██╔════╝██║     ██╔════╝██╔════╝╚══██╔══╝
█████╗  ██║   ██║██╔██╗ ██║█████╗  ██║     █████╗  █████╗     ██║
██╔══╝  ██║   ██║██║╚██╗██║██╔══╝  ██║     ██╔══╝  ██╔══╝     ██║
██║     ╚██████╔╝██║ ╚████║██║     ███████╗███████╗███████╗   ██║
╚═╝      ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚══════╝╚══════╝╚══════╝   ╚═╝
```

<div align="center">

### Batalla Naval incremental construida sprint a sprint en C++

`Programación Avanzada` · `Módulo C++` · `Proyecto Integrador`

`C++11` · `Consola` · `7/7 Sprints completados`

</div>

---

## `$` Tabla de contenido

```
├── 01. Descripción general
├── 02. Contexto del proyecto
├── 03. Reglas del juego
├── 04. Estructura del repositorio
├── 05. Progreso por sprint
├── 06. Compilación y ejecución
├── 07. Menú principal
├── 08. Comandos disponibles en partida
├── 09. Arquitectura y estructuras de datos
├── 10. Archivos generados por el programa
├── 11. Gestión de memoria dinámica
├── 12. Rúbrica de evaluación
└── 13. Autor
```

---

## `01.` Descripción general

**FunFleet** es un juego de consola inspirado en la Batalla Naval clásica, desarrollado de forma **incremental** a lo largo de siete sprints. Cada entrega conserva y amplía las funcionalidades de la anterior, aplicando progresivamente los temas vistos en el módulo de C++: tipos de datos, cadenas estilo `char[]`, archivos de texto, archivos binarios, apuntadores, arreglos de estructuras y memoria dinámica.

> El objetivo no es replicar la Batalla Naval al pie de la letra, sino usar su mecánica como excusa para implementar, capa sobre capa, cada tema del curso.

---

## `02.` Contexto del proyecto

```
┌──────────────────────────────────────────────────────────────┐
│  La flota FunFleet patrulla un archipiélago lleno de         │
│  tormentas... y de bugs por corregir.                        │
│                                                                │
│  Misión: construir, versión por versión, un sistema de       │
│  batalla naval capaz de administrar un tablero, ubicar       │
│  barcos, registrar disparos, reportar impactos y persistir   │
│  la información de la partida.                               │
└──────────────────────────────────────────────────────────────┘
```

**Objetivo general:** diseñar e implementar una aplicación de consola en C++ que permita cargar un tablero, registrar disparos, administrar barcos, generar reportes y persistir información usando archivos de texto, archivos binarios, apuntadores, estructuras y memoria dinámica.

---

## `03.` Reglas del juego

| Parámetro | Valor |
|---|---|
| Tamaño del tablero | `10 x 10` |
| Coordenadas visibles al usuario | `1..10` (internamente `0..9`) |
| Total de barcos en la flota | `9` |
| Resultados posibles de un disparo | `Agua` · `Tocado` · `Hundido` |
| Fin de la partida | Todos los barcos hundidos **o** turnos agotados |

**Composición de la flota**

| Código | Nombre | Casillas | Cantidad |
|---|---|---|---|
| `P` | Portaaviones | 4 | 1 |
| `S` | Submarino | 3 | 1 |
| `A1` / `A2` | Acorazado | 3 | 2 |
| `D1` / `D2` / `D3` | Destructor | 2 | 3 |
| `F1` / `F2` | Fragata | 1 | 2 |
| `~` | Agua | — | — |

Los barcos se ubican horizontal o verticalmente, **sin superposición** y sin salirse del tablero. Una coordenada fuera de rango o ya disparada se reporta como error **y no consume turno**.

**Ejemplo de tablero de flota**

```
     1  2  3  4  5  6  7  8  9  10
 1   P  P  P  P  ~  ~  ~  ~  ~  ~
 2   ~  ~  ~  ~  ~  ~  ~  S  ~  ~
 3   ~  ~  A2 A2 A2 ~  ~  S  ~  ~
 4   ~  ~  ~  F1 ~  ~  ~  S  ~  ~
 5   ~  ~  ~  ~  ~  A1 A1 A1 ~  ~
 6   ~  ~  ~  ~  ~  ~  ~  ~  ~  D3
 7   D1 D1 ~  ~  ~  ~  ~  ~  ~  D3
 8   ~  ~  ~  ~  ~  ~  ~  ~  ~  ~
 9   ~  ~  ~  ~  D2 D2 ~  ~  ~  ~
10   ~  ~  ~  ~  ~  ~  ~  ~  F2 ~
```

---

## `04.` Estructura del repositorio

```
avanzadaBatallaNaval/
│
├── sprint1.cpp     → Base del juego, tipos de datos y conversiones
├── sprint2.cpp     → Cadenas de caracteres tipo char
├── sprint3.cpp     → Archivos de texto (acceso secuencial)
├── sprint4.cpp     → Archivos binarios (acceso aleatorio e indexado)
├── sprint5.cpp     → Apuntadores
├── sprint6.cpp     → Arreglos de estructuras recorridos con apuntadores
├── sprint7.cpp     → Memoria dinámica e integración final ★ versión completa
└── README.md       → Este documento
```

Cada archivo `sprintN.cpp` es una entrega **autocontenida y compilable**, que conserva todo lo aprobado en los sprints previos. `sprint7.cpp` es la integración final del proyecto.

---

## `05.` Progreso por sprint

| # | Tema central | Estado |
|---|---|---|
| 1 | Base del juego, tipos de datos y conversiones | `✔ COMPLETADO` |
| 2 | Cadenas de caracteres tipo `char` | `✔ COMPLETADO` |
| 3 | Archivos de texto con acceso secuencial | `✔ COMPLETADO` |
| 4 | Archivos binarios con acceso aleatorio e indexado | `✔ COMPLETADO` |
| 5 | Apuntadores | `✔ COMPLETADO` |
| 6 | Arreglos de estructuras recorridos con apuntadores | `✔ COMPLETADO` |
| 7 | Memoria dinámica e integración final | `✔ COMPLETADO` |

```
[■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■] 7/7 sprints — 100%
```

---

## `06.` Compilación y ejecución

```bash
# Clonar el repositorio
$ git clone https://github.com/miguelkd1/avanzadaBatallaNaval.git
$ cd avanzadaBatallaNaval

# Compilar la entrega final (sprint7.cpp)
$ g++ -std=c++11 -o funfleet sprint7.cpp

# Ejecutar
$ ./funfleet
```

> En Windows (MinGW): `g++ -std=c++11 -o funfleet.exe sprint7.cpp` y luego `funfleet.exe`.

---

## `07.` Menú principal

```
=== BATALLA NAVAL - MENU PRINCIPAL ===
1.  Jugar con flota manual (colocar o cargar y disparar)
2.  Jugar con flota aleatoria
3.  Cargar flota desde "fleet-grid.txt"
4.  Ver mi flota
5.  Exportar flota actual a "ai-fleet-grid.txt"
6.  Generar reporte "funfleet-report.txt"
7.  Guardar partida ("funfleet-save.dat")
8.  Cargar partida ("funfleet-save.dat")
9.  Consultar disparo por turno ("shots.dat")
10. Ver estadísticas con apuntadores
11. Comparar memoria estática vs dinámica
12. Salir
```

---

## `08.` Comandos disponibles en partida

Una vez dentro de la fase de disparos, el jugador interactúa mediante comandos de texto:

| Comando | Acción |
|---|---|
| `DISPARAR` | Solicita coordenadas `X` `Y` y ejecuta el disparo |
| `TABLERO` | Muestra el tablero de disparos (agua / tocado / hundido) |
| `REPORTE` | Imprime el resumen de la partida en curso |
| `CONSULTAR_DISPARO` | Busca un disparo específico dentro de `shots.dat` por número de turno |
| `SELECCIONAR` | Selecciona un barco por código (`P`, `S`, `A1`...) usando apuntadores |
| `FLOTA_PTR` | Muestra el estado completo de la flota vía apuntadores |
| `MEMORIA` | Compara el consumo de memoria estática vs. dinámica del tablero e historial |
| `GUARDAR` | Exporta el reporte y la flota a archivos de texto |
| `GUARDAR_PARTIDA` | Serializa la partida completa a `funfleet-save.dat` |
| `CARGAR_PARTIDA` | Restaura una partida guardada previamente |
| `SALIR` | Termina la partida y exporta el reporte final |
| `FLOTA_DEV` | *(modo desarrollador)* revela las posiciones reales de la flota — solo para depuración |

---

## `09.` Arquitectura y estructuras de datos

**Entidades principales**

```cpp
struct Barco {
    char tipo[20];
    char codigo[3];
    int  tamanio;
    int  vidaActual;
    bool estaHundido;
    int  posXInicio, posYInicio;
    bool esHorizontal;
};

struct NodoDisparo {
    Disparo dato;
    NodoDisparo* siguiente;   // lista enlazada del historial de disparos
};

struct PartidaBinaria {
    char firma[4];            // "FF7" — validación del archivo de guardado
    char nombreJugador[50];
    BarcoBinario barcos[9];
    char gridBarcos[10][10][3];
    char gridDisparos[10][10];
    // ...contadores de turnos, aciertos, fallos, historial
};
```

- **Tableros dinámicos:** `gridBarcos` y `gridDisparos` se crean en tiempo de ejecución con `new` como una matriz de `string**`, y se liberan con `delete[]` al finalizar el programa.
- **Historial de disparos:** implementado como **lista enlazada simple** (`NodoDisparo`), en lugar de un arreglo de tamaño fijo — crece dinámicamente según los turnos jugados.
- **Selección de barcos por apuntadores:** `barcosSeleccionadosPtr[]` referencia directamente a los elementos de `barcosColocados[]` para consultar y modificar su estado sin copiarlos.
- **Catálogo de flota:** tabla constante `CATALOGO_FLOTA[9]` con código, nombre y tamaño de cada tipo de barco.

---

## `10.` Archivos generados por el programa

| Archivo | Tipo | Contenido |
|---|---|---|
| `fleet-grid.txt` | Texto | Flota de entrada cargada manualmente por el usuario |
| `ai-fleet-grid.txt` | Texto | Exportación de la flota actual del jugador |
| `funfleet-report.txt` | Texto | Reporte final de la partida (aciertos, fallos, precisión) |
| `shots.dat` | Binario indexado | Registro de cada disparo, accesible por número de turno |
| `funfleet-save.dat` | Binario | Estado completo de la partida (guardar / cargar) |

---

## `11.` Gestión de memoria dinámica

El comando `MEMORIA` compara en tiempo real el costo de una implementación **estática** frente a la **dinámica** usada en el proyecto:

```
=== MEMORIA ESTATICA vs DINAMICA APLICADA AL TABLERO ===
Estatica (string grid[10][10]): su tamano se fija al compilar, existe
  durante todo el programa y no se puede liberar ni cambiar de tamano.
Dinamica (string** grid = new ...): se crea en ejecucion con new, se usa
  a traves de apuntadores y se devuelve al sistema con delete[].
```

Además, el programa lleva su propia contabilidad de memoria (`reservasMemoria` / `liberacionesMemoria`) y libera automáticamente todos los recursos al salir mediante `atexit(liberarMemoria)`, garantizando cero fugas al finalizar.

---

## `12.` Rúbrica de evaluación

- Los 7 sprints tienen el **mismo peso** dentro de la nota final (`≈14.29%` cada uno sobre 100 puntos).
- Cada entrega se evalúa por: cumplimiento funcional, uso correcto del tema de clase, integración con sprints anteriores, manejo de errores y calidad del código.
- Toda entrega debe **compilar** y demostrar que conserva las funcionalidades aprobadas en sprints anteriores.

---

## `13.` Autor

```
$ whoami
Andrés — Ingeniería de Sistemas, Pontificia Universidad Javeriana (Bogotá)
```

<div align="center">

`Proyecto Integrador C++ — FunFleet` · Programación Avanzada

</div>
