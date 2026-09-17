// Código elaborado por David Coy Vélez, Miguel Ángel Martinez y Andrés Santiago Sabogal Meza
#include <iostream>
#include <cstring>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <map>     // SPRINT 6 (nice to have): map<codigo, Barco*> para comparar con busqueda lineal
using namespace std;
// ============================================================
//                    CONSTANTES GLOBALES
// ============================================================
const int TAM_TABLERO = 10;
const int COORD_MIN = 1;
const int COORD_MAX = 10;
const int TAM_NOMBRE = 50;
const int TAM_COMANDO = 20;
const int TAM_TIPO_BARCO = 20;
// SPRINT 4: limite maximo de turnos (disparos validos) por partida.
// Se muestra al jugador al iniciar la fase de disparos.
const int MAX_TURNOS = 60;
// -- Nombres de los archivos de texto del proyecto (Sprint 3) --
const char ARCHIVO_FLOTA_ENTRADA[] = "fleet-grid.txt";      // Se carga (si existe)
const char ARCHIVO_FLOTA_SALIDA[]  = "ai-fleet-grid.txt";   // Se exporta
const char ARCHIVO_REPORTE[]       = "funfleet-report.txt"; // Reporte basico
// -- Nombres de los archivos binarios del proyecto (Sprint 4) --
const char ARCHIVO_DISPAROS[]         = "shots.dat";         // Registro de disparos
const char ARCHIVO_PARTIDA_BINARIA[]  = "funfleet-save.dat"; // Estado guardado de la partida

struct Barco {
	char tipo[TAM_TIPO_BARCO];      // "Portaaviones", "Submarino", "Acorazado 1", ...
	char codigo[3];                  // "P", "S", "A1", "D3", "F2" (identificador en el tablero)
	int tamanio;                     // Cuantas casillas ocupa (4, 3, 2, 1)
	int vidaActual;                  // Cuantas casillas aun no han sido disparadas
	bool estaHundido;                // true si vidaActual == 0
	int posXInicio, posYInicio;      // Coordenada inicial (usuario: 1-10)
	bool esHorizontal;               // true = horizontal, false = vertical
};
/*
* TipoBarco / CATALOGO_FLOTA
* Catalogo fijo de la flota oficial de FunFleet. Sirve para tres cosas:
*   - Saber el TAMAÑO obligatorio de cada barco.
*   - Saber la CANTIDAD permitida (cada codigo aparece una sola vez).
*   - Validar los codigos leidos desde el archivo de texto.
*/
struct TipoBarco {
	char codigo[3];                 // Codigo unico usado en el tablero y en el .txt
	char nombre[TAM_TIPO_BARCO];    // Nombre legible para los mensajes
	int tamanio;                    // Casillas obligatorias del barco
};
const int TOTAL_BARCOS_FLOTA = 9;
const TipoBarco CATALOGO_FLOTA[TOTAL_BARCOS_FLOTA] = {
	{"P",  "Portaaviones", 4},
	{"S",  "Submarino",    3},
	{"A1", "Acorazado 1",  3},
	{"A2", "Acorazado 2",  3},
	{"D1", "Destructor 1", 2},
	{"D2", "Destructor 2", 2},
	{"D3", "Destructor 3", 2},
	{"F1", "Fragata 1",    1},
	{"F2", "Fragata 2",    1}
};
/*
* SPRINT 4: RegistroDisparo
* Estructura de tamaño fijo para "shots.dat". Solo usa tipos simples
* (int, char, arreglos fijos de char), nunca string ni STL, para poder
* escribirse y leerse directamente en binario con write()/read().
*/
struct RegistroDisparo {
	int turno;              // Numero de turno (1, 2, 3, ...)
	int x;                   // Coordenada X (1-10, vista del usuario)
	int y;                   // Coordenada Y (1-10, vista del usuario)
	char resultado;          // 'A' = Agua, 'T' = Tocado, 'H' = Hundido
	char codigoBarco[3];     // Codigo del barco impactado, o "--" si fue agua
};
/*
* SPRINT 4: BarcoBinario / PartidaBinaria
* Estructuras de tamaño fijo para "funfleet-save.dat". Los tableros se
* guardan como arreglos fijos de char (no string) usando los simbolos
* ~ (agua), B (barco), O (fallo), X (tocado) y H (hundido).
*/
struct BarcoBinario {
	char codigo[3];
	char tipo[TAM_TIPO_BARCO];
	int tamanio;
	int vidaActual;
	bool estaHundido;
	int posXInicio;
	int posYInicio;
	bool esHorizontal;
};
struct PartidaBinaria {
	char nombreJugador[TAM_NOMBRE];                    // char[50], segun requisito
	char tableroCombate[TAM_TABLERO][TAM_TABLERO];      // '~','B','O','X','H'
	int turnosRealizados;
	int aciertosGuardados;
	int fallosGuardados;
	int barcosHundidosGuardados;
	int cantidadBarcosGuardados;
	BarcoBinario barcos[TOTAL_BARCOS_FLOTA];
	// SPRINT 6: el historial viaja dentro de la partida guardada. Antes
	// solo vivia en memoria, y al cargar una partida se perdian todos
	// los disparos anteriores. Se usa RegistroDisparo (tamano fijo, ya
	// definido arriba) y MAX_TURNOS, porque nunca hay mas disparos que
	// turnos permitidos.
	int cantidadHistorialGuardado;
	RegistroDisparo historial[MAX_TURNOS];
};

/*
* SPRINT 6 - struct Disparo
* Version EN MEMORIA de un disparo 
* Se usa para armar un HISTORIAL recorrible con apuntadores durante
* la partida, sin tener que leer el archivo binario. Conserva
* coordenadas, resultado y barco afectado, en el orden de juego.
*/
struct Disparo {
	int turno;               // Numero de turno (1, 2, 3, ...)
	int x;                    // Coordenada X (1-10)
	int y;                    // Coordenada Y (1-10)
	char resultado;           // 'A' = Agua, 'T' = Tocado, 'H' = Hundido
	char codigoBarco[3];      // Codigo del barco impactado, o "--" si fue agua
};

/*
* SPRINT 6 - struct Partida
* Resumen/estado de la partida en un instante dado: NO reemplaza las
* variables globales existentes (nombreJugador, disparosRealizados,
* etc.), sino que las agrupa en una sola estructura para poder
* mostrar una "captura de estado" completa
* con una sola variable, construida a partir de los arreglos de
* apuntadores de la flota.
*/
struct Partida {
	char nombreJugador[TAM_NOMBRE];
	int turnosRealizados;
	int totalBarcos;
	int barcosActivos;
	int barcosHundidos;
	int vidaTotalRestante;
};

// ============================================================
//                    VARIABLES GLOBALES
// ============================================================
bool tableroInicializado = false;
string tablero[TAM_TABLERO][TAM_TABLERO];
string tablerojugador[TAM_TABLERO][TAM_TABLERO];  // Tablero oculto del jugador (ve barcos)
char nombreJugador[TAM_NOMBRE];
bool jugadorRegistrado = false;
// Array de barcos colocados en el tablero
Barco barcosColocados[TOTAL_BARCOS_FLOTA];
int cantidadBarcosColocados = 0;
// Control de CANTIDAD: cada barco del catalogo se puede colocar una sola vez
bool barcoYaColocado[TOTAL_BARCOS_FLOTA];
// Estadisticas del juego
int disparosRealizados = 0;
int aciertosTotales = 0;
int fallosTotales = 0;
int barcosHundidos = 0;
// Fase del juego
bool faseColocacionCompleta = false;

// ============================================================
//         SPRINT 6: HISTORIAL Y ARREGLOS DE APUNTADORES
// ============================================================
// Historial de disparos EN MEMORIA (arreglo de struct Disparo).
// Tamano fijo = MAX_TURNOS, porque no puede haber mas disparos
// validos que turnos permitidos en una partida.
const int MAX_HISTORIAL = MAX_TURNOS;
Disparo historialDisparos[MAX_HISTORIAL];
int cantidadHistorial = 0;

// Arreglo de APUNTADORES (Barco*, no Barco) para los barcos que el
// jugador ha marcado como "seleccionados" con el comando SELECCIONAR.
// Cada posicion guarda la direccion real de un elemento de
// barcosColocados: no son copias, por eso si ese barco cambia de
// estado despues (por ejemplo se hunde), se ve reflejado al
// recorrer este mismo arreglo.
Barco* barcosSeleccionadosPtr[TOTAL_BARCOS_FLOTA];
int cantidadBarcosSeleccionadosPtr = 0;

// ============================================================
//                   FUNCIONES DE UTILIDAD BASICA
// ============================================================
void limpiarBufferEntrada() {
	cin.clear();
	cin.ignore(1000, '\n');
}
int leerOpcionValida(int min, int max, const string& mensaje) {
	string entrada;
	int opcion;
	char caracterExtra;
	bool entradaValida = false;
	do {
		cout << mensaje;
		if (!getline(cin, entrada)) {
			cout << "\nERROR: No hay mas entrada disponible. Cerrando el programa." << endl;
			exit(0);
		}
		if (entrada.empty()) {
			cout << "Entrada invalida. Debe ingresar un numero entero." << endl;
			continue;
		}
		stringstream conversor(entrada);
		if (!(conversor >> opcion)) {
			cout << "Entrada invalida. Debe ingresar un numero entero." << endl;
			continue;
		}
		if (conversor >> caracterExtra) {
			cout << "Entrada invalida. No se permiten letras, puntos ni caracteres adicionales." << endl;
			continue;
		}
		if (opcion < min || opcion > max) {
			cout << "Opcion fuera de rango (" << min << "-" << max << ")." << endl;
			continue;
		}
		entradaValida = true;
	} while (!entradaValida);
	return opcion;
}
/*
* leerCoordenada()
* Lee una coordenada individual (X o Y) del usuario, validando que sea numerica
* y este en el rango 1-10. Si hay error, vuelve a pedir.
*/
int leerCoordenada(const string& mensaje) {
	string entrada;
	int coordenada;
	char caracterExtra;
	bool coordenadaValida = false;
	while (!coordenadaValida) {
		cout << mensaje;
		if (!getline(cin, entrada)) {
			cout << "\nERROR: No hay mas entrada disponible. Cerrando el programa." << endl;
			exit(0);
		}
		if (entrada.empty()) {
			cout << "ERROR: Debes ingresar un numero entero." << endl;
			continue;
		}
		stringstream conversor(entrada);
		if (!(conversor >> coordenada)) {
			cout << "ERROR: Debes ingresar un numero entero." << endl;
			continue;
		}
		if (conversor >> caracterExtra) {
			cout << "ERROR: La coordenada debe contener solamente un numero entero." << endl;
			continue;
		}
		if (coordenada < COORD_MIN || coordenada > COORD_MAX) {
			cout << "ERROR: La coordenada debe estar entre 1 y 10." << endl;
			continue;
		}
		coordenadaValida = true;
	}
	return coordenada;
}
/*
* leerDirectionHorizontal()
* Pregunta al usuario si desea colocar el barco de forma horizontal (1)
* o vertical (2). Valida que sea una opcion valida.
*/
bool leerDirectionHorizontal() {
	int opcion = leerOpcionValida(1, 2, "Direccion: 1. Horizontal / 2. Vertical: ");
	return opcion == 1;
}
void aMayusculas(char cadena[]) {
	for (int i = 0; cadena[i] != '\0'; i++) {
		cadena[i] = static_cast<char>(toupper(static_cast<unsigned char>(cadena[i])));
	}
}
void leerComando(char comando[], int tam) {
	string entrada;
	cout << "\nIngrese un comando (DISPARAR, TABLERO, REPORTE, GUARDAR, "
	     << "GUARDAR_PARTIDA, CARGAR_PARTIDA, CONSULTAR_DISPARO, SALIR"
	     << " | FLOTA_DEV | FLOTA_PTR, SELECCIONAR): ";
	if (!getline(cin, entrada)) {
		cout << "\nERROR: No hay mas entrada disponible. Cerrando el programa." << endl;
		exit(0);
	}
	// Evitar desbordamiento del arreglo de caracteres.
	if (entrada.length() >= static_cast<size_t>(tam)) {
		entrada = entrada.substr(0, tam - 1);
	}
	strcpy(comando, entrada.c_str());
	cout << endl;
}
/*
* existeArchivo()
* Retorna true si el archivo se puede abrir para lectura.
*/
bool existeArchivo(const char nombreArchivo[]) {
	ifstream archivo(nombreArchivo);
	bool existe = archivo.is_open();
	archivo.close();
	return existe;
}
/*
* buscarTipoPorCodigo()
* Busca un codigo ("P", "A2", "F1"...) dentro del catalogo de la flota.
* Retorna el indice del tipo o -1 si el codigo no existe.
*/
int buscarTipoPorCodigo(const string& codigo) {
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		if (codigo == CATALOGO_FLOTA[i].codigo) {
			return i;
		}
	}
	return -1;
}
/*
* codigoAnchoDos()
* Devuelve el codigo con exactamente 2 caracteres ("P" -> "P ", "A1" -> "A1")
* para que las columnas del tablero queden alineadas al imprimir.
*/
string codigoAnchoDos(const string& codigo) {
	string resultado = codigo;
	while (resultado.length() < 2) {
		resultado += " ";
	}
	return resultado;
}
/*
* quitarEspacios()
* Elimina los espacios de una celda del tablero ("A1" -> "A1", "P " -> "P").
* Se usa al ESCRIBIR el archivo de texto, para no guardar relleno.
*/
string quitarEspacios(const string& texto) {
	string limpio = "";
	for (int i = 0; i < (int)texto.length(); i++) {
		if (texto[i] != ' ') {
			limpio += texto[i];
		}
	}
	return limpio;
}
/*
* esLineaVacia()
* Indica si una linea del archivo solo tiene espacios en blanco.
*/
bool esLineaVacia(const string& linea) {
	for (int i = 0; i < (int)linea.length(); i++) {
		if (!isspace(static_cast<unsigned char>(linea[i]))) {
			return false;
		}
	}
	return true;
}
// ============================================================
//                 FUNCIONES DE INICIALIZACION
// ============================================================
void inicializarTablero() {
	tableroInicializado = true;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			tablero[fila][columna] = "~ ";
			tablerojugador[fila][columna] = "~ ";
		}
	}
}
void inicializarBarcosColocados() {
	cantidadBarcosColocados = 0;
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		barcoYaColocado[i] = false;
	}
}
/*
* todosBarcosHundidos()
* Retorna true si todos los barcos colocados han sido hundidos.
*/
bool todosBarcosHundidos() {
	if (cantidadBarcosColocados == 0) {
		return false;
	}
	return barcosHundidos == cantidadBarcosColocados;
}
/*
* SPRINT 4: reiniciarArchivoDisparos()
* Deja "shots.dat" vacio (0 registros). Se usa al iniciar una partida
* nueva para no mezclar los disparos con los de una partida anterior.
*/
void reiniciarArchivoDisparos() {
	ofstream archivo(ARCHIVO_DISPAROS, ios::binary | ios::trunc);
	if (!archivo.is_open()) {
		cout << "ADVERTENCIA: No se pudo reiniciar el archivo \"" << ARCHIVO_DISPAROS << "\"." << endl;
		return;
	}
	archivo.close();
}
/*
* reiniciarPartida()
* Deja el juego como recien iniciado: tablero limpio, flota vacia
* y estadisticas en cero. Se usa al terminar una partida.
*/
void reiniciarPartida() {
	inicializarTablero();
	inicializarBarcosColocados();
	disparosRealizados = 0;
	aciertosTotales = 0;
	fallosTotales = 0;
	barcosHundidos = 0;
	faseColocacionCompleta = false;

	// SPRINT 6: el historial en memoria y la lista de seleccionados
	// tambien pertenecen a la partida que termino, asi que se limpian
	// igual que el resto de estadisticas.
	cantidadHistorial = 0;
	cantidadBarcosSeleccionadosPtr = 0;
}
// ============================================================
//                   FUNCIONES DE TABLERO
// ============================================================
/*
* celdaVisibleParaJugador()
* Traduce el contenido REAL de una casilla del tablero de combate a lo
* que el jugador tiene permitido ver. Un barco que todavia no ha sido
* disparado ("B ") se muestra como agua ("~ "): si se mostrara la B, el
* comando TABLERO revelaria la flota entera y el juego perderia sentido.
* Los estados ya descubiertos (O fallo, X tocado, H hundido) se muestran
* tal cual, porque el jugador ya se los gano disparando.
*/
string celdaVisibleParaJugador(const string& celdaReal) {
	if (celdaReal == "B ") {
		return "~ ";
	}
	return celdaReal;
}
void mostrarTablero() {
	if (tableroInicializado) {
		cout << "\n=== TABLERO DE COMBATE ===" << endl;
		cout << "    ";
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			cout << "C" << (columna + 1) << "  ";
		}
		cout << endl;
		for (int fila = 0; fila < TAM_TABLERO; fila++) {
			if (fila + 1 < 10) cout << " " << "F" << (fila + 1) << " ";
			else cout << "F" << (fila + 1) << " ";
			for (int columna = 0; columna < TAM_TABLERO; columna++) {
				cout << " " << celdaVisibleParaJugador(tablero[fila][columna]) << " ";
			}
			cout << endl;
		}
	} else {
		cout << "El tablero no se ha inicializado." << endl;
	}
}
/*
* mostrarTableroJugador()
* Version del tablero que muestra los barcos del jugador (P, S, A1, A2,
* D1, D2, D3, F1, F2) asi como las posiciones que ha disparado.
*/
void mostrarTableroJugador() {
	if (tableroInicializado) {
		cout << "\n=== TU TABLERO ===" << endl;
		cout << "    ";
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			cout << "C" << (columna + 1) << "  ";
		}
		cout << endl;
		for (int fila = 0; fila < TAM_TABLERO; fila++) {
			if (fila + 1 < 10) cout << " " << "F" << (fila + 1) << " ";
			else cout << "F" << (fila + 1) << " ";
			for (int columna = 0; columna < TAM_TABLERO; columna++) {
				cout << " " << tablerojugador[fila][columna] << " ";
			}
			cout << endl;
		}
	}
}
// ============================================================
//   SPRINT 5 -  FUNCIONES CON APUNTADORES
// ============================================================
// primeros dos entregables (quiten este comentario cuando ya esten los 6 entregables)
//   1) Al menos tres funciones con parametros por apuntador.
//   2) Recorrido de gridBarcos y gridDisparos usando apuntadores.
//
// En este proyecto, "gridBarcos" equivale a tablerojugador (el
// tablero donde se ven los barcos propios) y "gridDisparos"
// equivale a tablero (el tablero de combate con los resultados
// ~ agua, B barco sin disparar, O fallo, X tocado, H hundido).
// Ambos son matrices string[TAM_TABLERO][TAM_TABLERO], y en C++
// una matriz asi esta almacenada como un bloque continuo de
// TAM_TABLERO*TAM_TABLERO strings, por lo que se puede recorrer
// con un puntero string* y aritmetica de punteros (puntero++)
// en vez de usar los indices [fila][columna].
//
// Estas funciones SOLO LEEN tablero y tablerojugador para calcular
// estadisticas nuevas; no modifican ni reemplazan nada de lo que
// ya hacian los Sprints 1 a 4.
// ============================================================

/*
* SPRINT 5 - contarCasillasFlotaPtr()
* Recorre gridBarcos (tablerojugador) con un apuntador a string y
* cuenta cuantas casillas tienen parte de un barco y cuantas son
* agua. Los dos resultados se devuelven POR APUNTADOR en
* "totalConBarco" y "totalAgua" (parametros por apuntador).
*/
void contarCasillasFlotaPtr(string* gridBarcos, int totalCasillas,
                             int* totalConBarco, int* totalAgua) {
	*totalConBarco = 0;
	*totalAgua = 0;
	string* actual = gridBarcos;              // apuntador al inicio de gridBarcos
	for (int i = 0; i < totalCasillas; i++) {
		if (*actual != "~ ") {
			(*totalConBarco)++;
		} else {
			(*totalAgua)++;
		}
		actual++;                              // aritmetica de apuntadores: siguiente casilla
	}
}
/*
* SPRINT 5 - contarResultadosDisparosPtr()
* Recorre gridDisparos (tablero) con un apuntador a string y cuenta
* fallos (O), tocados (X) y hundidos (H). Los tres contadores son
* parametros por apuntador y se llenan dentro de la funcion.
*/
void contarResultadosDisparosPtr(string* gridDisparos, int totalCasillas,
                                  int* totalFallos, int* totalTocados,
                                  int* totalHundidos) {
	*totalFallos = 0;
	*totalTocados = 0;
	*totalHundidos = 0;
	string* fin = gridDisparos + totalCasillas; // "una posicion despues" de la ultima casilla
	for (string* actual = gridDisparos; actual < fin; actual++) {
		if (*actual == "O ") {
			(*totalFallos)++;
		} else if (*actual == "X ") {
			(*totalTocados)++;
		} else if (*actual == "H ") {
			(*totalHundidos)++;
		}
	}
}
/*
* SPRINT 5 - contarCasillasSinDispararPtr()
* Tercera funcion con parametro por apuntador: recorre gridDisparos
* (tablero) con apuntador y cuenta, en "totalSinDisparar" (por
* apuntador), cuantas casillas siguen sin ser disparadas (agua "~ "
* o barco todavia oculto "B ").
*/
void contarCasillasSinDispararPtr(string* gridDisparos, int totalCasillas,
                                   int* totalSinDisparar) {
	*totalSinDisparar = 0;

	string* fin = gridDisparos + totalCasillas;
	for (string* actual = gridDisparos; actual < fin; actual++) {
		if (*actual == "~ " || *actual == "B ") {
			(*totalSinDisparar)++;
		}
	}
}
/*
* SPRINT 5 - resumenFlotaPtr()
* ARITMETICA DE APUNTADORES SOBRE UN ARREGLO.
*
* Recorre barcosColocados (9 structs Barco guardados uno detras de
* otro en memoria) SIN usar indices [i]: avanza con un apuntador.
*
*   flota + cantidad  -> "una posicion despues del ultimo". Solo se
*                        usa para comparar; nunca se lee.
*   actual++          -> avanza sizeof(Barco) bytes de una (48 en
*                        este programa), no 1 byte: el compilador
*                        escala el salto al tamano del tipo.
*   actual->campo     -> forma corta de (*actual).campo
*
* Los tres resultados salen POR APUNTADOR, igual que en las otras
* funciones del Sprint 5.
*/
void resumenFlotaPtr(Barco* flota, int cantidad,
                     int* hundidos, int* aFlote, int* vidaRestante) {
	*hundidos = 0;
	*aFlote = 0;
	*vidaRestante = 0;

	Barco* fin = flota + cantidad;

	for (Barco* actual = flota; actual < fin; actual++) {
		if (actual->estaHundido) {
			(*hundidos)++;
		} else {
			(*aFlote)++;
		}
		*vidaRestante += actual->vidaActual;
	}
}
/*
* SPRINT 5 - contarSinDispararPorFilaPtr()
* ENTREGABLE 5: APUNTADOR A ARREGLO.
*
* gridDisparos (tablero) esta guardado en memoria como un bloque de
* TAM_TABLERO arreglos de TAM_TABLERO strings, uno detras del otro
* (una fila detras de la otra). Por eso el tipo correcto para
* recorrerlo FILA POR FILA no es un doble apuntador (string**), sino
* un APUNTADOR A ARREGLO: string (*)[TAM_TABLERO].
*
*   gridDisparos            -> apunta a la fila actual (un arreglo
*                              completo de TAM_TABLERO strings).
*   (*gridDisparos)[col]    -> casilla "col" de esa fila.
*   gridDisparos++          -> avanza TAM_TABLERO strings de una vez,
*                              es decir, salta a la fila siguiente.
*
* Llena "resultadoPorFila", con una casilla por fila, indicando
* cuantas casillas de esa fila siguen sin ser disparadas (agua "~ "
* o barco todavia oculto "B "). Sirve para mostrarle al jugador una
* pista real de en que filas todavia queda mucho por explorar.
*/
void contarSinDispararPorFilaPtr(string (*gridDisparos)[TAM_TABLERO], int totalFilas,
                                  int resultadoPorFila[]) {
	for (int fila = 0; fila < totalFilas; fila++) {
		resultadoPorFila[fila] = 0;
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			if ((*gridDisparos)[columna] == "~ " || (*gridDisparos)[columna] == "B ") {
				resultadoPorFila[fila]++;
			}
		}
		gridDisparos++; // aritmetica sobre apuntador a arreglo: pasa a la fila siguiente
	}
}
/*
* SPRINT 5 - mostrarEstadisticasConApuntadores()
* Reune las tres funciones anteriores para mostrar en pantalla un
* resumen de gridBarcos y gridDisparos calculado completamente con
* recorridos por apuntador. Se llama desde una nueva opcion del
* menu principal (ver Sprint 5 en main()) para poder evidenciarla
* sin tocar el flujo ya aprobado de los sprints anteriores.
*/
void mostrarEstadisticasConApuntadores() {
	if (!tableroInicializado) {
		cout << "\nEl tablero no se ha inicializado todavia." << endl;
		return;
	}
	int totalCasillas = TAM_TABLERO * TAM_TABLERO;
	// &tablerojugador[0][0] y &tablero[0][0]: direccion de la primera
	// casilla de cada matriz, usada como apuntador de arranque.
	int casillasConBarco = 0, casillasAgua = 0;
	contarCasillasFlotaPtr(&tablerojugador[0][0], totalCasillas,
	                       &casillasConBarco, &casillasAgua);
	int fallos = 0, tocados = 0, hundidosCasillas = 0;
	contarResultadosDisparosPtr(&tablero[0][0], totalCasillas,
	                            &fallos, &tocados, &hundidosCasillas);
	int sinDisparar = 0;
	contarCasillasSinDispararPtr(&tablero[0][0], totalCasillas, &sinDisparar);
	cout << "\n=== ESTADISTICAS CON APUNTADORES (SPRINT 5) ===" << endl;
	cout << "gridBarcos (tablerojugador) -> casillas con barco: " << casillasConBarco
	     << " | casillas de agua: " << casillasAgua << endl;
	cout << "gridDisparos (tablero) -> fallos (O): " << fallos
	     << " | tocados (X): " << tocados
	     << " | hundidos (H): " << hundidosCasillas
	     << " | sin disparar aun: " << sinDisparar << endl;
	// Mismo tipo de recorrido, pero sobre el ARREGLO de estructuras.
	int hundidos = 0, aFlote = 0, vidaRestante = 0;
	resumenFlotaPtr(barcosColocados, cantidadBarcosColocados,
	                &hundidos, &aFlote, &vidaRestante);
	cout << "flota (arreglo de Barco) -> hundidos: " << hundidos
	     << " | a flote: " << aFlote
	     << " | casillas de vida restantes: " << vidaRestante << endl;
	// ENTREGABLE 5: recorrido de gridDisparos fila por fila con un
	// apuntador a arreglo (string (*)[TAM_TABLERO]).
	int filasSinDisparar[TAM_TABLERO];
	contarSinDispararPorFilaPtr(tablero, TAM_TABLERO, filasSinDisparar);
	cout << "gridDisparos por fila (apuntador a arreglo) -> casillas sin disparar: ";
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		cout << "F" << (fila + 1) << "=" << filasSinDisparar[fila] << " ";
	}
	cout << endl;
}
/*
* SPRINT 5 - actualizarTurnoPtr()
* Actualiza el contador de turnos usando PASO POR REFERENCIA CON
* APUNTADORES: la funcion no recibe copias de los numeros, recibe
* sus DIRECCIONES (int*), y por eso puede modificar las variables
* originales que viven fuera de ella.
*
*   (*turnosUsados)++   los parentesis son obligatorios: sin ellos
*                       el ++ le pegaria al APUNTADOR y no al valor.
*
* Como no depende de ninguna variable global, sirve para cualquier
* contador que se le pase, no solo para disparosRealizados.
*/
void actualizarTurnoPtr(int* turnosUsados, int* turnosRestantes) {
	(*turnosUsados)++;
	*turnosRestantes = MAX_TURNOS - *turnosUsados;
}

// ============================================================
//   SPRINT 6 - ARREGLOS DE ESTRUCTURAS RECORRIDOS CON APUNTADORES
// ============================================================
// Entregables de este sprint>
//   - struct Disparo y struct Partida (definidas arriba, junto a
//     las demas estructuras).
//   - Arreglo de estructuras para la flota: ya existia desde
//     sprints anteriores (barcosColocados[TOTAL_BARCOS_FLOTA]).
//   - Recorrido de flota e historial usando apuntadores.
//   - Arreglo de apuntadores para barcos activos, hundidos o
//     seleccionados.
// ============================================================

/*
* SPRINT 6 - registrarDisparoEnHistorial()
* Agrega un struct Disparo al arreglo historialDisparos (en RAM),
* respetando el orden de juego. Es el equivalente en memoria de
* guardarDisparoBinario() (Sprint 4), que guarda lo mismo en
* "shots.dat"; aqui ademas queda en memoria para poder recorrerse
* con apuntadores sin leer el archivo.
*/
void registrarDisparoEnHistorial(int turno, int x, int y, char resultado,
                                  const char codigoBarco[]) {
	if (cantidadHistorial >= MAX_HISTORIAL) {
		return; // no deberia pasar: MAX_HISTORIAL = MAX_TURNOS
	}
	Disparo* nuevo = &historialDisparos[cantidadHistorial]; // apuntador al nuevo slot
	nuevo->turno = turno;
	nuevo->x = x;
	nuevo->y = y;
	nuevo->resultado = resultado;
	strncpy(nuevo->codigoBarco, codigoBarco, 2);
	nuevo->codigoBarco[2] = '\0';
	cantidadHistorial++;
}

/*
* SPRINT 6 - mostrarDetalleFlotaPtr()
* RECORRIDO DE FLOTA CON APUNTADOR: recorre el arreglo de
* estructuras barcosColocados (Barco[]) usando un apuntador Barco*
* (sin indices []), mostrando codigo, nombre, tamano, vida y estado
* de cada barco.
*/
void mostrarDetalleFlotaPtr(Barco* flota, int cantidad) {
	cout << "\n--- FLOTA (struct Barco[], recorrido por apuntador) ---" << endl;
	if (cantidad == 0) {
		cout << "Todavia no hay barcos colocados." << endl;
		return;
	}
	Barco* fin = flota + cantidad;
	for (Barco* actual = flota; actual < fin; actual++) {
		cout << actual->codigo << " " << actual->tipo
		     << " | tamano: " << actual->tamanio
		     << " | vida: " << actual->vidaActual
		     << " | estado: " << (actual->estaHundido ? "HUNDIDO" : "ACTIVO") << endl;
	}
}

/*
* SPRINT 6 - mostrarHistorialPtr()
* RECORRIDO DE HISTORIAL CON APUNTADOR: recorre el arreglo de
* estructuras historialDisparos (Disparo[]) usando un apuntador
* Disparo*, mostrando cada jugada en el orden en que ocurrio.
*/
void mostrarHistorialPtr(Disparo* historial, int cantidad) {
	cout << "\n--- HISTORIAL DE DISPAROS (struct Disparo[], por apuntador) ---" << endl;
	if (cantidad == 0) {
		cout << "Aun no se ha registrado ningun disparo en esta partida." << endl;
		return;
	}
	Disparo* fin = historial + cantidad;
	for (Disparo* actual = historial; actual < fin; actual++) {
		string resultadoTexto = (actual->resultado == 'A') ? "Agua"
		                       : (actual->resultado == 'T') ? "Tocado"
		                       : (actual->resultado == 'H') ? "Hundido"
		                       : "Desconocido";
		cout << "Turno " << actual->turno << ": (" << actual->x << "," << actual->y
		     << ") -> " << resultadoTexto;
		if (actual->resultado != 'A') {
			cout << " [" << actual->codigoBarco << "]";
		}
		cout << endl;
	}
}

/*
* SPRINT 6 - clasificarFlotaPorEstadoPtr()
* ENTREGABLE ojo ARREGLO DE APUNTADORES. Recorre la flota con un
* apuntador Barco* y, en vez de copiar cada barco, guarda su
* DIRECCION en "activos" o "hundidos" segun su estado. Como son
* apuntadores al barco real (no copias), cualquier cambio posterior
* del barco (por ejemplo que se hunda) se refleja automaticamente
* la proxima vez que se recorran estos arreglos.
*/
void clasificarFlotaPorEstadoPtr(Barco* flota, int cantidad,
                                  Barco* activos[], int* cantidadActivos,
                                  Barco* hundidos[], int* cantidadHundidos) {
	*cantidadActivos = 0;
	*cantidadHundidos = 0;

	Barco* fin = flota + cantidad;
	for (Barco* actual = flota; actual < fin; actual++) {
		if (actual->estaHundido) {
			hundidos[*cantidadHundidos] = actual;
			(*cantidadHundidos)++;
		} else {
			activos[*cantidadActivos] = actual;
			(*cantidadActivos)++;
		}
	}
}

/*
* SPRINT 6 - mostrarArregloPtrBarcos()
* Recorre un ARREGLO DE APUNTADORES (Barco* arregloPtr[]) -distinto
* de un arreglo de estructuras- mostrando el codigo del barco al que
* apunta cada posicion.
*/
void mostrarArregloPtrBarcos(const string& titulo, Barco* arregloPtr[], int cantidad) {
	cout << titulo << " (" << cantidad << "): ";
	if (cantidad == 0) {
		cout << "(ninguno)";
	}
	for (int i = 0; i < cantidad; i++) {
		cout << arregloPtr[i]->codigo;
		if (i < cantidad - 1) cout << ", ";
	}
	cout << endl;
}

/*
* SPRINT 6 - seleccionarBarcoPtr()
* Busca en barcosColocados el barco con el codigo dado. Si existe y
* todavia no estaba en la lista, agrega su APUNTADOR (no una copia)
* al arreglo global barcosSeleccionadosPtr. La logica es generica:
* funciona igual para cualquier codigo de la flota, sin condicionales
* repetidos por cada barco especifico.
*/
bool seleccionarBarcoPtr(const char codigo[]) {
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		if (strcmp(barcosColocados[i].codigo, codigo) == 0) {
			Barco* candidato = &barcosColocados[i];

			for (int j = 0; j < cantidadBarcosSeleccionadosPtr; j++) {
				if (barcosSeleccionadosPtr[j] == candidato) {
					cout << "Ese barco ya estaba en la lista de seleccionados." << endl;
					return false;
				}
			}

			if (cantidadBarcosSeleccionadosPtr >= TOTAL_BARCOS_FLOTA) {
				cout << "ERROR: Ya no hay espacio para mas barcos seleccionados." << endl;
				return false;
			}

			barcosSeleccionadosPtr[cantidadBarcosSeleccionadosPtr] = candidato;
			cantidadBarcosSeleccionadosPtr++;
			cout << "Barco [" << candidato->codigo << "] agregado a seleccionados." << endl;
			return true;
		}
	}
	cout << "ERROR: No existe ningun barco colocado con codigo \"" << codigo << "\"." << endl;
	return false;
}

/*
* SPRINT 6 - construirEstadoPartida()
* Arma un struct Partida (definido arriba) a partir del estado
* actual del juego y de los conteos ya calculados por
* clasificarFlotaPorEstadoPtr(). Es la "captura de estado" que pide
* la evidencia requerida del sprint.
*/
Partida construirEstadoPartida(int cantidadActivos, int cantidadHundidosFlota) {
	Partida estado;
	strncpy(estado.nombreJugador, nombreJugador, TAM_NOMBRE - 1);
	estado.nombreJugador[TAM_NOMBRE - 1] = '\0';
	estado.turnosRealizados = disparosRealizados;
	estado.totalBarcos = cantidadBarcosColocados;
	estado.barcosActivos = cantidadActivos;
	estado.barcosHundidos = cantidadHundidosFlota;

	int vidaRestante = 0;
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		vidaRestante += barcosColocados[i].vidaActual;
	}
	estado.vidaTotalRestante = vidaRestante;

	return estado;
}

void mostrarEstadoPartida(const Partida& estado) {
	cout << "\n--- ESTADO DE PARTIDA (struct Partida) ---" << endl;
	cout << "Jugador: " << estado.nombreJugador << endl;
	cout << "Turnos jugados: " << estado.turnosRealizados << endl;
	cout << "Barcos totales: " << estado.totalBarcos
	     << " | activos: " << estado.barcosActivos
	     << " | hundidos: " << estado.barcosHundidos << endl;
	cout << "Vida total restante en la flota: " << estado.vidaTotalRestante << endl;
}

/*
* SPRINT 6 - construirMapaBarcosPorCodigoPtr()
* Compara la busqueda lineal manual que ya se usaba desde sprints
* anteriores (por ejemplo en buscarTipoPorCodigo o
* encontrarBarcoEnPosicion, que recorren el arreglo de a uno, O(n))
* con un std::map<string, Barco*>, que ordena los codigos por arbol
* La ventaja real del map aparece cuando se
* construye una sola vez y se consulta muchas veces; aqui se usa
* solo como demostracion justificada, sin reemplazar la logica ya
* aprobada de los sprints anteriores.
*/
map<string, Barco*> construirMapaBarcosPorCodigoPtr(Barco* flota, int cantidad) {
	map<string, Barco*> mapaCodigos;
	Barco* fin = flota + cantidad;
	for (Barco* actual = flota; actual < fin; actual++) {
		mapaCodigos[string(actual->codigo)] = actual;
	}
	return mapaCodigos;
}

/*
* SPRINT 6 - mostrarEstadoCompletoPtr()
* flota e historial recorridos por apuntador, clasificacion en
* arreglos de apuntadores (activos/hundidos/seleccionados), el
* struct Partida con el resumen, y la comparacion nice-to-have
* con std::map. Se invoca desde el comando FLOTA_PTR.
*/
void mostrarEstadoCompletoPtr() {
	if (cantidadBarcosColocados == 0) {
		cout << "\nTodavia no hay una flota colocada." << endl;
		return;
	}

	mostrarDetalleFlotaPtr(barcosColocados, cantidadBarcosColocados);
	mostrarHistorialPtr(historialDisparos, cantidadHistorial);

	Barco* activosPtr[TOTAL_BARCOS_FLOTA];
	Barco* hundidosPtr[TOTAL_BARCOS_FLOTA];
	int cantActivos = 0, cantHundidosFlota = 0;
	clasificarFlotaPorEstadoPtr(barcosColocados, cantidadBarcosColocados,
	                             activosPtr, &cantActivos, hundidosPtr, &cantHundidosFlota);

	cout << "\n--- CLASIFICACION (arreglos de apuntadores Barco*) ---" << endl;
	mostrarArregloPtrBarcos("Activos", activosPtr, cantActivos);
	mostrarArregloPtrBarcos("Hundidos", hundidosPtr, cantHundidosFlota);
	mostrarArregloPtrBarcos("Seleccionados", barcosSeleccionadosPtr, cantidadBarcosSeleccionadosPtr);

	Partida estado = construirEstadoPartida(cantActivos, cantHundidosFlota);
	mostrarEstadoPartida(estado);

	map<string, Barco*> mapaCodigos = construirMapaBarcosPorCodigoPtr(barcosColocados, cantidadBarcosColocados);
	cout << "\n--- NICE TO HAVE: std::map<codigo, Barco*> ---" << endl;
	cout << "Barcos indexados en el map: " << mapaCodigos.size() << endl;
	if (!mapaCodigos.empty()) {
		string primerCodigo = mapaCodigos.begin()->first;
		Barco* porMap = mapaCodigos[primerCodigo];
		cout << "Ejemplo de busqueda por map (\"" << primerCodigo << "\"): "
		     << porMap->tipo << " | vida: " << porMap->vidaActual << endl;
	}
}

// ============================================================
//            FUNCIONES DE FASE DE COLOCACION
// ============================================================

/*
* crearBarcoEnArray()
* Crea un objeto Barco con la informacion suministrada y lo agrega
* al array de barcosColocados.
*/
void crearBarcoEnArray(const char tipoBarco[], const char codigoBarco[],
                       int posX, int posY, int tamanio, bool esHorizontal) {
	Barco nuevoBarco;
	strcpy(nuevoBarco.tipo, tipoBarco);
	strcpy(nuevoBarco.codigo, codigoBarco);
	nuevoBarco.tamanio = tamanio;
	nuevoBarco.vidaActual = tamanio;
	nuevoBarco.estaHundido = false;
	nuevoBarco.posXInicio = posX;
	nuevoBarco.posYInicio = posY;
	nuevoBarco.esHorizontal = esHorizontal;
	barcosColocados[cantidadBarcosColocados] = nuevoBarco;
	cantidadBarcosColocados++;
}
/*
* colocarBarcoEnTablero()
* Marca en el tablero del jugador todas las casillas que ocupa el barco
* con su codigo de tipo (P, S, A1, A2, D1, D2, D3, F1, F2).
*/
void colocarBarcoEnTablero(const char codigoBarco[],
                            int posX, int posY, int tamanio, bool esHorizontal) {
	int fila = posY - 1;
	int col = posX - 1;
	string marcaBarco = codigoAnchoDos(codigoBarco);

	if (esHorizontal) {
		for (int i = 0; i < tamanio; i++) {
			tablerojugador[fila][col + i] = marcaBarco;
			tablero[fila][col + i] = "B ";
		}
	} else {
		for (int i = 0; i < tamanio; i++) {
			tablerojugador[fila + i][col] = marcaBarco;
			tablero[fila + i][col] = "B ";
		}
	}
}
/*
* validarColocacionBarco()
* Valida que el barco no salga del tablero (1-10) y que no se solape con
* otro barco ya colocado. Si "mostrarErrores" es false, no imprime nada
* (se usa para los intentos silenciosos de la flota aleatoria).
*/
bool validarColocacionBarco(int posX, int posY, int tamanio, bool esHorizontal,
                             bool mostrarErrores = true) {
	if (posX < COORD_MIN || posX > COORD_MAX || posY < COORD_MIN || posY > COORD_MAX) {
		if (mostrarErrores) cout << "ERROR: Las coordenadas deben estar entre 1 y 10." << endl;
		return false;
	}
	int fila = posY - 1;
	int col = posX - 1;
	if (esHorizontal) {
		if (col + tamanio > TAM_TABLERO) {
			if (mostrarErrores) cout << "ERROR: El barco se saldra del tablero hacia la derecha." << endl;
			return false;
		}
	} else {
		if (fila + tamanio > TAM_TABLERO) {
			if (mostrarErrores) cout << "ERROR: El barco se saldra del tablero hacia abajo." << endl;
			return false;
		}
	}
	if (esHorizontal) {
		for (int i = 0; i < tamanio; i++) {
			if (tablerojugador[fila][col + i] != "~ ") {
				if (mostrarErrores) {
					cout << "ERROR: Ya hay un barco en esa posicion (casilla "
					     << (col + i + 1) << ", " << (fila + 1) << ")." << endl;
				}
				return false;
			}
		}
	} else {
		for (int i = 0; i < tamanio; i++) {
			if (tablerojugador[fila + i][col] != "~ ") {
				if (mostrarErrores) {
					cout << "ERROR: Ya hay un barco en esa posicion (casilla "
					     << col + 1 << ", " << (fila + i + 1) << ")." << endl;
				}
				return false;
			}
		}
	}
	return true;
}
/*
* mostrarOpcionesBarcos()
* Muestra la flota completa indicando cuales barcos ya fueron colocados
* y cuales siguen pendientes.
*/
void mostrarOpcionesBarcos() {
	cout << "\n=== FLOTA FUNFLEET (9 BARCOS) ===" << endl;
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		cout << (i + 1) << ". " << CATALOGO_FLOTA[i].nombre
		     << " [" << CATALOGO_FLOTA[i].codigo << "] - "
		     << CATALOGO_FLOTA[i].tamanio << " casilla(s)   ";
		if (barcoYaColocado[i]) {
			cout << "-> YA COLOCADO";
		} else {
			cout << "-> PENDIENTE";
		}
		cout << endl;
	}
	cout << (TOTAL_BARCOS_FLOTA + 1) << ". Terminar colocacion" << endl;
	cout << "Barcos colocados: " << cantidadBarcosColocados
	     << "/" << TOTAL_BARCOS_FLOTA << endl;
}
/*
* intentarColocarBarco()
* Flujo completo para colocar un barco del catalogo: pide coordenadas,
* direccion, valida y si es correcto lo agrega al tablero y al array.
*/
bool intentarColocarBarco(int indiceTipo) {
	const TipoBarco& tipo = CATALOGO_FLOTA[indiceTipo];
	cout << "\n-- Colocando " << tipo.nombre << " [" << tipo.codigo
	     << "] de " << tipo.tamanio << " casilla(s) --" << endl;
	int posX = leerCoordenada("Ingrese la posicion X (1-10): ");
	int posY = leerCoordenada("Ingrese la posicion Y (1-10): ");
	bool esHorizontal = true;
	if (tipo.tamanio > 1) {
		esHorizontal = leerDirectionHorizontal();
	}
	if (!validarColocacionBarco(posX, posY, tipo.tamanio, esHorizontal)) {
		cout << "No se pudo colocar el barco. Intente nuevamente." << endl;
		return false;
	}
	colocarBarcoEnTablero(tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
	crearBarcoEnArray(tipo.nombre, tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
	cout << tipo.nombre << " colocado exitosamente!" << endl;
	return true;
}
/*
* faseColocacionBarcosCompleta()
* Menu interactivo donde el jugador coloca sus barcos manualmente antes
* de empezar la fase de disparos.
*/
void faseColocacionBarcosCompleta() {
	cout << "\n=== FASE DE COLOCACION DE BARCOS ===" << endl;
	cout << "Coloque sus barcos en el tablero. (Minimo 1 barco para empezar)." << endl;
	bool terminoColocacion = false;
	while (!terminoColocacion) {
		mostrarOpcionesBarcos();
		int opcion = leerOpcionValida(1, TOTAL_BARCOS_FLOTA + 1, "Seleccione una opcion: ");
		if (opcion == TOTAL_BARCOS_FLOTA + 1) {
			if (cantidadBarcosColocados == 0) {
				cout << "ERROR: Debe colocar al menos 1 barco antes de comenzar." << endl;
				continue;
			}
			cout << "\n Esta seguro de terminar? Tiene " << cantidadBarcosColocados
			     << " barco(s) colocado(s)." << endl;
			if (cantidadBarcosColocados < TOTAL_BARCOS_FLOTA) {
				cout << "AVISO: la flota completa son " << TOTAL_BARCOS_FLOTA
				     << " barcos. Le faltan "
				     << (TOTAL_BARCOS_FLOTA - cantidadBarcosColocados) << "." << endl;
				cout << "AVISO: una flota incompleta NO se puede volver a cargar desde "
				     << ARCHIVO_FLOTA_ENTRADA << "." << endl;
			}
			int confirmacion = leerOpcionValida(1, 2, "1. Si, empezar / 2. No, seguir colocando: ");
			if (confirmacion == 1) {
				terminoColocacion = true;
				faseColocacionCompleta = true;
				cout << "\nColocacion completada! Preparese para disparar." << endl;
			}
		} else {
			int indiceTipo = opcion - 1;
			if (barcoYaColocado[indiceTipo]) {
				cout << "ERROR: El " << CATALOGO_FLOTA[indiceTipo].nombre
				     << " [" << CATALOGO_FLOTA[indiceTipo].codigo
				     << "] ya fue colocado. Solo hay 1 de cada uno." << endl;
				continue;
			}
			if (intentarColocarBarco(indiceTipo)) {
				barcoYaColocado[indiceTipo] = true;
				mostrarTableroJugador();
			}
		}
	}
}
/*
* SPRINT 3/4: generarFlotaAleatoria()
* Genera los 9 barcos oficiales en posiciones y orientaciones
* aleatorias, evitando salidas del tablero y superposiciones.
* Reintenta cada barco hasta colocarlo; si algun barco no logra
* colocarse tras varios intentos, reinicia toda la flota e intenta
* de nuevo (muy poco probable en un tablero 10x10).
*/
void generarFlotaAleatoria() {
	bool exito = false;
	while (!exito) {
		inicializarTablero();
		inicializarBarcosColocados();
		exito = true;
		for (int i = 0; i < TOTAL_BARCOS_FLOTA && exito; i++) {
			const TipoBarco& tipo = CATALOGO_FLOTA[i];
			bool colocado = false;
			int intentos = 0;
			while (!colocado && intentos < 500) {
				intentos++;
				int posX = (rand() % TAM_TABLERO) + 1;
				int posY = (rand() % TAM_TABLERO) + 1;
				bool esHorizontal = (rand() % 2 == 0);
				if (validarColocacionBarco(posX, posY, tipo.tamanio, esHorizontal, false)) {
					colocarBarcoEnTablero(tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
					crearBarcoEnArray(tipo.nombre, tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
					barcoYaColocado[i] = true;
					colocado = true;
				}
			}
			if (!colocado) {
				exito = false; // No se pudo colocar este barco: se reinicia toda la flota
			}
		}
	}
	faseColocacionCompleta = true;
}
// ============================================================
//   ARCHIVOS DE TEXTO CON ACCESO SECUENCIAL (SPRINT 3)
// ============================================================
/*
* leerGridDesdeArchivo()
* PASO 1 de la carga: lee el archivo de texto de forma SECUENCIAL
* (linea por linea con getline) y guarda los 100 tokens en gridLeido.
*/
bool leerGridDesdeArchivo(const char nombreArchivo[], string gridLeido[][TAM_TABLERO]) {
	ifstream archivo(nombreArchivo);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo abrir el archivo \"" << nombreArchivo << "\"." << endl;
		cout << "       Verifique que este en la misma carpeta del programa." << endl;
		return false;
	}
	string linea;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		do {
			if (!getline(archivo, linea)) {
				cout << "ERROR: El archivo tiene menos de " << TAM_TABLERO
				     << " filas (falta la fila " << (fila + 1) << ")." << endl;
				archivo.close();
				return false;
			}
		} while (esLineaVacia(linea));
		stringstream separador(linea);
		string token;
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			if (!(separador >> token)) {
				cout << "ERROR: La fila " << (fila + 1) << " tiene menos de "
				     << TAM_TABLERO << " columnas." << endl;
				archivo.close();
				return false;
			}
			if (token != "~" && buscarTipoPorCodigo(token) == -1) {
				cout << "ERROR: Codigo desconocido \"" << token << "\" en la fila "
				     << (fila + 1) << ", columna " << (columna + 1) << "." << endl;
				cout << "       Codigos validos: ~ P S A1 A2 D1 D2 D3 F1 F2" << endl;
				archivo.close();
				return false;
			}
			gridLeido[fila][columna] = token;
		}
		string sobrante;
		if (separador >> sobrante) {
			cout << "ERROR: La fila " << (fila + 1) << " tiene mas de "
			     << TAM_TABLERO << " columnas." << endl;
			archivo.close();
			return false;
		}
	}
	while (getline(archivo, linea)) {
		if (!esLineaVacia(linea)) {
			cout << "ERROR: El archivo tiene mas de " << TAM_TABLERO
			     << " filas con datos." << endl;
			archivo.close();
			return false;
		}
	}
	archivo.close();
	return true;
}
/*
* validarGridDeFlota()
* PASO 2 de la carga: valida la CANTIDAD y el TAMAÑO de cada barco, que
* formen una linea recta y que no tengan huecos.
*/
bool validarGridDeFlota(string gridLeido[][TAM_TABLERO]) {
	bool todoCorrecto = true;
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		const TipoBarco& tipo = CATALOGO_FLOTA[i];
		int casillasEncontradas = 0;
		int filaMin = TAM_TABLERO, filaMax = -1;
		int colMin = TAM_TABLERO, colMax = -1;
		for (int fila = 0; fila < TAM_TABLERO; fila++) {
			for (int columna = 0; columna < TAM_TABLERO; columna++) {
				if (gridLeido[fila][columna] == tipo.codigo) {
					casillasEncontradas++;
					if (fila < filaMin) filaMin = fila;
					if (fila > filaMax) filaMax = fila;
					if (columna < colMin) colMin = columna;
					if (columna > colMax) colMax = columna;
				}
			}
		}
		if (casillasEncontradas == 0) {
			cout << "ERROR: Falta el barco " << tipo.nombre
			     << " [" << tipo.codigo << "] en el archivo." << endl;
			todoCorrecto = false;
			continue;
		}
		if (casillasEncontradas != tipo.tamanio) {
			cout << "ERROR: El barco " << tipo.nombre << " [" << tipo.codigo
			     << "] debe ocupar " << tipo.tamanio << " casilla(s) y ocupa "
			     << casillasEncontradas << "." << endl;
			todoCorrecto = false;
			continue;
		}
		bool esHorizontal = (filaMin == filaMax);
		bool esVertical = (colMin == colMax);
		if (!esHorizontal && !esVertical) {
			cout << "ERROR: El barco " << tipo.nombre << " [" << tipo.codigo
			     << "] no esta en linea recta (debe ser horizontal o vertical)." << endl;
			todoCorrecto = false;
			continue;
		}
		int largoOcupado;
		if (esHorizontal) {
			largoOcupado = colMax - colMin + 1;
		} else {
			largoOcupado = filaMax - filaMin + 1;
		}
		if (largoOcupado != tipo.tamanio) {
			cout << "ERROR: El barco " << tipo.nombre << " [" << tipo.codigo
			     << "] tiene casillas separadas. Deben ir juntas." << endl;
			todoCorrecto = false;
		}
	}
	return todoCorrecto;
}
/*
* aplicarGridAlJuego()
* PASO 3 de la carga: ya validado el archivo, se reconstruye el estado
* del juego (tableros + array de barcos) a partir del grid leido.
*/
void aplicarGridAlJuego(string gridLeido[][TAM_TABLERO]) {
	inicializarTablero();
	inicializarBarcosColocados();
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		const TipoBarco& tipo = CATALOGO_FLOTA[i];
		int filaMin = TAM_TABLERO, filaMax = -1;
		int colMin = TAM_TABLERO, colMax = -1;
		for (int fila = 0; fila < TAM_TABLERO; fila++) {
			for (int columna = 0; columna < TAM_TABLERO; columna++) {
				if (gridLeido[fila][columna] == tipo.codigo) {
					if (fila < filaMin) filaMin = fila;
					if (fila > filaMax) filaMax = fila;
					if (columna < colMin) colMin = columna;
					if (columna > colMax) colMax = columna;
				}
			}
		}
		bool esHorizontal = (filaMin == filaMax);
		int posX = colMin + 1;
		int posY = filaMin + 1;
		colocarBarcoEnTablero(tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
		crearBarcoEnArray(tipo.nombre, tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
		barcoYaColocado[i] = true;
	}
}
/*
* cargarFlotaDesdeArchivo()
* Carga manual de la flota desde un archivo de texto (Sprint 3).
* Encadena los tres pasos: leer -> validar -> aplicar.
*/
bool cargarFlotaDesdeArchivo(const char nombreArchivo[]) {
	cout << "\n=== CARGA DE FLOTA DESDE \"" << nombreArchivo << "\" ===" << endl;
	string gridLeido[TAM_TABLERO][TAM_TABLERO];
	if (!leerGridDesdeArchivo(nombreArchivo, gridLeido)) {
		cout << "La flota NO fue cargada." << endl;
		return false;
	}
	if (!validarGridDeFlota(gridLeido)) {
		cout << "La flota NO fue cargada porque el archivo tiene errores." << endl;
		return false;
	}
	aplicarGridAlJuego(gridLeido);
	disparosRealizados = 0;
	aciertosTotales = 0;
	fallosTotales = 0;
	barcosHundidos = 0;
	faseColocacionCompleta = true;
	cout << "Flota cargada correctamente: " << cantidadBarcosColocados
	     << " barcos validados." << endl;
	return true;
}
/*
* exportarFlotaAArchivo()
* Exporta la flota actual a un archivo de texto con el mismo formato de
* entrada (10 filas x 10 columnas separadas por espacios).
*/
bool exportarFlotaAArchivo(const char nombreArchivo[]) {
	if (cantidadBarcosColocados == 0) {
		cout << "ERROR: No hay barcos colocados. Primero coloque o cargue una flota." << endl;
		return false;
	}
	ofstream archivo(nombreArchivo);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo crear el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			archivo << quitarEspacios(tablerojugador[fila][columna]);
			if (columna < TAM_TABLERO - 1) {
				archivo << " ";
			}
		}
		archivo << endl;
	}
	archivo.close();
	cout << "Flota exportada correctamente a \"" << nombreArchivo << "\"." << endl;
	if (cantidadBarcosColocados < TOTAL_BARCOS_FLOTA) {
		cout << "AVISO: la flota exportada esta incompleta ("
		     << cantidadBarcosColocados << "/" << TOTAL_BARCOS_FLOTA
		     << "), no se podra recargar como flota valida." << endl;
	}
	return true;
}
/*
* prepararFlotaAntesDeJugar()
* Antes de disparar, decide de donde sale la flota manual:
*   - Si "fleet-grid.txt" ya existe, ofrece cargarlo.
*   - Si no existe (o el jugador prefiere), se coloca manualmente.
*/
void prepararFlotaAntesDeJugar() {
	bool flotaLista = false;
	if (existeArchivo(ARCHIVO_FLOTA_ENTRADA)) {
		cout << "\nSe encontro el archivo \"" << ARCHIVO_FLOTA_ENTRADA << "\"." << endl;
		int opcion = leerOpcionValida(1, 2,
			"1. Cargar la flota del archivo / 2. Colocar los barcos manualmente: ");
		if (opcion == 1) {
			flotaLista = cargarFlotaDesdeArchivo(ARCHIVO_FLOTA_ENTRADA);
			if (!flotaLista) {
				cout << "Se continuara con la colocacion manual." << endl;
			}
		}
	} else {
		cout << "\nNo se encontro \"" << ARCHIVO_FLOTA_ENTRADA
		     << "\", se colocaran los barcos manualmente." << endl;
	}
	if (!flotaLista) {
		faseColocacionBarcosCompleta();
	}
	int exportar = leerOpcionValida(1, 2,
		"\nDesea exportar esta flota a un archivo de texto? 1. Si / 2. No: ");
	if (exportar == 1) {
		exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
	}
}
/*
* exportarReporteAArchivo()
* Genera el reporte basico de la partida en un archivo de texto:
* jugador, estadisticas, estado de cada barco y los dos tableros.
*/
bool exportarReporteAArchivo(const char nombreArchivo[]) {
	ofstream archivo(nombreArchivo);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo crear el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	archivo << "============================================" << endl;
	archivo << "        FUNFLEET - REPORTE DE PARTIDA       " << endl;
	archivo << "============================================" << endl;
	archivo << "Jugador             : " << nombreJugador << endl;
	archivo << "Barcos en la flota  : " << cantidadBarcosColocados
	        << "/" << TOTAL_BARCOS_FLOTA << endl;
	archivo << "Turnos maximos      : " << MAX_TURNOS << endl;
	archivo << "Disparos realizados : " << disparosRealizados << endl;
	archivo << "Aciertos            : " << aciertosTotales << endl;
	archivo << "Fallos              : " << fallosTotales << endl;
	archivo << "Barcos hundidos     : " << barcosHundidos << endl;
	if (cantidadBarcosColocados > 0 && todosBarcosHundidos()) {
		archivo << "Resultado           : VICTORIA (flota hundida)" << endl;
	} else if (disparosRealizados >= MAX_TURNOS) {
		archivo << "Resultado           : DERROTA (turnos agotados)" << endl;
	} else {
		archivo << "Resultado           : PARTIDA EN CURSO" << endl;
	}
	archivo << endl;
	archivo << "--- ESTADO DE LA FLOTA ---" << endl;
	archivo << "CODIGO  BARCO           TAM  VIDA  ESTADO   INICIO(X,Y)  DIRECCION" << endl;
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		Barco& barco = barcosColocados[i];
		archivo << left << setw(8) << barco.codigo
		        << setw(16) << barco.tipo
		        << setw(5) << barco.tamanio
		        << setw(6) << barco.vidaActual
		        << setw(9) << (barco.estaHundido ? "HUNDIDO" : "A FLOTE")
		        << "(" << barco.posXInicio << "," << barco.posYInicio << ")"
		        << "        " << (barco.esHorizontal ? "Horizontal" : "Vertical")
		        << endl;
	}
	archivo << endl;
	archivo << "--- TABLERO DE LA FLOTA (posiciones) ---" << endl;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			archivo << tablerojugador[fila][columna] << " ";
		}
		archivo << endl;
	}
	archivo << endl;
	archivo << "--- TABLERO DE COMBATE (disparos) ---" << endl;
	archivo << "Leyenda: ~ agua | B barco | O fallo | X tocado | H hundido" << endl;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			archivo << tablero[fila][columna] << " ";
		}
		archivo << endl;
	}
	archivo.close();
	cout << "Reporte generado correctamente en \"" << nombreArchivo << "\"." << endl;
	return true;
}
// ============================================================
//   SPRINT 4: ARCHIVO BINARIO "funfleet-save.dat"
// ============================================================
/*
* guardarPartidaBinaria()
* Serializa el estado completo de la partida (jugador, tableros,
* estadisticas y flota) en una unica estructura de tamaño fijo y la
* escribe en binario con write(), usando seekp() para posicionarse
* al inicio del archivo antes de escribir.
*/
bool guardarPartidaBinaria(const char nombreArchivo[]) {
	if (cantidadBarcosColocados == 0) {
		cout << "ERROR: No hay una partida activa para guardar (no hay barcos colocados)." << endl;
		return false;
	}
	PartidaBinaria datos;
	memset(&datos, 0, sizeof(PartidaBinaria));
	strncpy(datos.nombreJugador, nombreJugador, TAM_NOMBRE - 1);
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			datos.tableroCombate[fila][columna] = tablero[fila][columna][0];
		}
	}
	datos.turnosRealizados = disparosRealizados;
	datos.aciertosGuardados = aciertosTotales;
	datos.fallosGuardados = fallosTotales;
	datos.barcosHundidosGuardados = barcosHundidos;
	datos.cantidadBarcosGuardados = cantidadBarcosColocados;
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		strcpy(datos.barcos[i].codigo, barcosColocados[i].codigo);
		strncpy(datos.barcos[i].tipo, barcosColocados[i].tipo, TAM_TIPO_BARCO - 1);
		datos.barcos[i].tamanio = barcosColocados[i].tamanio;
		datos.barcos[i].vidaActual = barcosColocados[i].vidaActual;
		datos.barcos[i].estaHundido = barcosColocados[i].estaHundido;
		datos.barcos[i].posXInicio = barcosColocados[i].posXInicio;
		datos.barcos[i].posYInicio = barcosColocados[i].posYInicio;
		datos.barcos[i].esHorizontal = barcosColocados[i].esHorizontal;
	}
	// SPRINT 6: copiar el historial recorriendolo con apuntadores.
	datos.cantidadHistorialGuardado = cantidadHistorial;
	Disparo* origen = historialDisparos;
	RegistroDisparo* destino = datos.historial;
	for (int i = 0; i < cantidadHistorial; i++, origen++, destino++) {
		destino->turno = origen->turno;
		destino->x = origen->x;
		destino->y = origen->y;
		destino->resultado = origen->resultado;
		strcpy(destino->codigoBarco, origen->codigoBarco);
	}
	ofstream archivo(nombreArchivo, ios::binary | ios::trunc);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo crear el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	archivo.seekp(0, ios::beg);
	archivo.write(reinterpret_cast<char*>(&datos), sizeof(PartidaBinaria));
	archivo.close();
	cout << "Partida guardada correctamente en \"" << nombreArchivo << "\"." << endl;
	return true;
}
/*
* cargarPartidaBinaria()
* Lee la estructura de "funfleet-save.dat" con seekg()/read() y
* reconstruye los tableros, la flota y las estadisticas del juego.
*/
bool cargarPartidaBinaria(const char nombreArchivo[]) {
	ifstream archivo(nombreArchivo, ios::binary);
	if (!archivo.is_open()) {
		cout << "ERROR: No existe el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	PartidaBinaria datos;
	archivo.seekg(0, ios::beg);
	archivo.read(reinterpret_cast<char*>(&datos), sizeof(PartidaBinaria));
	if (!archivo || archivo.gcount() != static_cast<streamsize>(sizeof(PartidaBinaria))) {
		cout << "ERROR: El archivo \"" << nombreArchivo << "\" esta danado o incompleto." << endl;
		archivo.close();
		return false;
	}
	archivo.close();
	if (datos.cantidadBarcosGuardados <= 0 || datos.cantidadBarcosGuardados > TOTAL_BARCOS_FLOTA ||
	    datos.cantidadHistorialGuardado < 0 || datos.cantidadHistorialGuardado > MAX_HISTORIAL) {
		cout << "ERROR: El archivo \"" << nombreArchivo << "\" contiene datos invalidos." << endl;
		return false;
	}
	strcpy(nombreJugador, datos.nombreJugador);
	inicializarTablero();
	inicializarBarcosColocados();
	cantidadBarcosColocados = datos.cantidadBarcosGuardados;
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		Barco b;
		strcpy(b.codigo, datos.barcos[i].codigo);
		strcpy(b.tipo, datos.barcos[i].tipo);
		b.tamanio = datos.barcos[i].tamanio;
		b.vidaActual = datos.barcos[i].vidaActual;
		b.estaHundido = datos.barcos[i].estaHundido;
		b.posXInicio = datos.barcos[i].posXInicio;
		b.posYInicio = datos.barcos[i].posYInicio;
		b.esHorizontal = datos.barcos[i].esHorizontal;
		barcosColocados[i] = b;
		int indiceTipo = buscarTipoPorCodigo(string(b.codigo));
		if (indiceTipo != -1) {
			barcoYaColocado[indiceTipo] = true;
		}
		// Reconstruir tablerojugador con el codigo real del barco (2 caracteres)
		int fila = b.posYInicio - 1;
		int col = b.posXInicio - 1;
		string marca = codigoAnchoDos(b.codigo);
		for (int k = 0; k < b.tamanio; k++) {
			if (b.esHorizontal) {
				tablerojugador[fila][col + k] = marca;
			} else {
				tablerojugador[fila + k][col] = marca;
			}
		}
	}
	// Reconstruir el tablero de combate a partir de los simbolos guardados
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			string valor = "";
			valor += datos.tableroCombate[fila][columna];
			valor += " ";
			tablero[fila][columna] = valor;
		}
	}
	disparosRealizados = datos.turnosRealizados;
	aciertosTotales = datos.aciertosGuardados;
	fallosTotales = datos.fallosGuardados;
	barcosHundidos = datos.barcosHundidosGuardados;
	faseColocacionCompleta = true;

	// SPRINT 6: restaurar el historial en memoria, en orden de jugada.
	cantidadHistorial = datos.cantidadHistorialGuardado;
	RegistroDisparo* leido = datos.historial;
	Disparo* restaurado = historialDisparos;
	for (int i = 0; i < cantidadHistorial; i++, leido++, restaurado++) {
		restaurado->turno = leido->turno;
		restaurado->x = leido->x;
		restaurado->y = leido->y;
		restaurado->resultado = leido->resultado;
		strcpy(restaurado->codigoBarco, leido->codigoBarco);
	}

	// Los seleccionados apuntaban a la flota anterior: se vacian.
	cantidadBarcosSeleccionadosPtr = 0;

	// SPRINT 4 + 6: reescribir "shots.dat" con el historial restaurado,
	// para que el registro k vuelva a ser el turno k+1. Si no, el siguiente
	// disparo quedaria en la posicion 0 y CONSULTAR_DISPARO devolveria
	// un turno equivocado.
	ofstream disparos(ARCHIVO_DISPAROS, ios::binary | ios::trunc);
	if (disparos.is_open()) {
		disparos.write(reinterpret_cast<char*>(datos.historial),
		               cantidadHistorial * sizeof(RegistroDisparo));
		disparos.close();
	} else {
		cout << "ADVERTENCIA: No se pudo reconstruir \"" << ARCHIVO_DISPAROS << "\"." << endl;
	}

	cout << "Partida cargada correctamente desde \"" << nombreArchivo << "\"." << endl;
	return true;
}

// ============================================================
//   SPRINT 4: ARCHIVO BINARIO "shots.dat"
// ============================================================

/*
* guardarDisparoBinario()
* Guarda un RegistroDisparo al final de "shots.dat" con write().
* Se llama inmediatamente despues de cada disparo valido.
*/
void guardarDisparoBinario(int turno, int x, int y, char resultado, const char codigoBarco[]) {
	RegistroDisparo registro;
	registro.turno = turno;
	registro.x = x;
	registro.y = y;
	registro.resultado = resultado;
	strncpy(registro.codigoBarco, codigoBarco, 2);
	registro.codigoBarco[2] = '\0';
	ofstream archivo(ARCHIVO_DISPAROS, ios::binary | ios::app);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo abrir \"" << ARCHIVO_DISPAROS << "\" para guardar el disparo." << endl;
		return;
	}
	archivo.write(reinterpret_cast<char*>(&registro), sizeof(RegistroDisparo));
	archivo.close();
}
/*
* consultarDisparoPorTurno()
* Consulta un disparo por numero de turno usando acceso ALEATORIO real:
* calcula la posicion con (turno - 1) * sizeof(RegistroDisparo), hace
* seekg() y lee el registro con read().
*/
void consultarDisparoPorTurno() {
	cout << "\n=== CONSULTAR_DISPARO: consulta de un turno en \"" << ARCHIVO_DISPAROS << "\" ===" << endl;
	ifstream archivo(ARCHIVO_DISPAROS, ios::binary);
	if (!archivo.is_open()) {
		cout << "ERROR: No existe el archivo \"" << ARCHIVO_DISPAROS
		     << "\" todavia. Juegue al menos un turno antes de consultar." << endl;
		return;
	}
	// Primero se mide el archivo. Sabiendo cuantos registros hay se puede
	// pedir el turno dentro de un rango real, en vez de aceptar cualquier
	// numero gigante y rechazarlo despues.
	archivo.seekg(0, ios::end);
	long tamanioArchivo = static_cast<long>(archivo.tellg());
	int totalRegistros = static_cast<int>(tamanioArchivo / sizeof(RegistroDisparo));
	if (totalRegistros == 0) {
		cout << "ERROR: Aun no se ha registrado ningun disparo en esta partida." << endl;
		archivo.close();
		return;
	}
	cout << "Turnos disponibles: 1 a " << totalRegistros << "." << endl;
	int turno = leerOpcionValida(1, totalRegistros, "Ingrese el numero de turno a consultar: ");
	long posicion = static_cast<long>(turno - 1) * static_cast<long>(sizeof(RegistroDisparo));
	archivo.seekg(posicion, ios::beg);
	RegistroDisparo registro;
	archivo.read(reinterpret_cast<char*>(&registro), sizeof(RegistroDisparo));
	if (!archivo) {
		cout << "ERROR: No se pudo leer el registro del turno " << turno << "." << endl;
		archivo.close();
		return;
	}
	archivo.close();
	string resultadoTexto;
	if (registro.resultado == 'A') resultadoTexto = "Agua";
	else if (registro.resultado == 'T') resultadoTexto = "Tocado";
	else if (registro.resultado == 'H') resultadoTexto = "Hundido";
	else resultadoTexto = "Desconocido";
	cout << "\n--- Turno " << registro.turno << " ---" << endl;
	cout << "Coordenadas    : (" << registro.x << ", " << registro.y << ")" << endl;
	cout << "Resultado      : " << resultadoTexto << endl;
	if (registro.resultado != 'A') {
		cout << "Barco impactado: " << registro.codigoBarco << endl;
	}
}
// ============================================================
//                   FUNCIONES DE DISPAROS
// ============================================================
/*
* encontrarBarcoEnPosicion()
* Busca en el array de barcosColocados si hay algun barco en la
* posicion (posX, posY). Retorna el indice del barco, o -1.
*/
int encontrarBarcoEnPosicion(int posX, int posY) {
	int fila = posY - 1;
	int col = posX - 1;
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		Barco& barco = barcosColocados[i];
		if (barco.esHorizontal) {
			if (fila == barco.posYInicio - 1 &&
			    col >= barco.posXInicio - 1 &&
			    col < barco.posXInicio - 1 + barco.tamanio) {
				return i;
			}
		} else {
			if (col == barco.posXInicio - 1 &&
			    fila >= barco.posYInicio - 1 &&
			    fila < barco.posYInicio - 1 + barco.tamanio) {
				return i;
			}
		}
	}
	return -1;
}
/*
* marcarBarcoHundido()
* Cambia a "H " todas las casillas del barco cuando ya no tiene vida.
*/
void marcarBarcoHundido(int indiceBarco) {
	Barco& barco = barcosColocados[indiceBarco];
	int filaInicio = barco.posYInicio - 1;
	int colInicio = barco.posXInicio - 1;

	for (int i = 0; i < barco.tamanio; i++) {
		if (barco.esHorizontal) {
			tablero[filaInicio][colInicio + i] = "H ";
		} else {
			tablero[filaInicio + i][colInicio] = "H ";
		}
	}
}
/*
* registrarDisparo()
* Reduce la vida del barco en 1. Si la vida llega a 0, marca el barco
* como hundido. Se invoca cuando un disparo acierta a un barco.
*/
void registrarDisparo(int indiceBarco, int posX, int posY) {
	// SPRINT 6: acceso por APUNTADOR (antes era una referencia Barco&).
	// barco->campo es la forma corta de (*barco).campo; la vida del
	// barco se lee y se modifica siempre a traves de este apuntador.
	Barco* barco = &barcosColocados[indiceBarco];
	if (barco->vidaActual > 0) {
		barco->vidaActual--;
	}
	int fila = posY - 1;
	int col = posX - 1;
	tablero[fila][col] = "X ";
	aciertosTotales++;
	cout << "TOCADO! Le diste al " << barco->tipo << " [" << barco->codigo << "]." << endl;
	if (barco->vidaActual == 0) {
		barco->estaHundido = true;
		barcosHundidos++;
		marcarBarcoHundido(indiceBarco);
		cout << barco->tipo << " HUNDIDO!!" << endl;
	} else {
		cout << "Vida restante del " << barco->tipo << ": " << barco->vidaActual << endl;
	}
}
/*
* procesarDisparo()
* Procesa un disparo en las coordenadas (posX, posY):
*   - Agua sin disparar: marca "O ", +1 fallo.
*   - Agua ya disparada / acierto repetido / barco hundido: error, no valido.
*   - Barco sin disparar: llama a registrarDisparo() para restar vida.
*
* Devuelve por referencia el resultado ('A'/'T'/'H') y el codigo del
* barco impactado (o "--" si fue agua), para poder guardarlo en shots.dat.
* Retorna true si el disparo fue valido (consume turno).
*/
bool procesarDisparo(int posX, int posY, char &resultadoDisparo, char codigoBarcoImpactado[3]) {
	if (posX < COORD_MIN || posX > COORD_MAX || posY < COORD_MIN || posY > COORD_MAX) {
		cout << "ERROR: Coordenadas fuera de rango (1-10)." << endl;
		return false;
	}
	int fila = posY - 1;
	int col = posX - 1;
	string estadoCasilla = tablero[fila][col];
	if (estadoCasilla == "~ ") {
		tablero[fila][col] = "O ";
		fallosTotales++;
		cout << "AGUA! El disparo no acerto nada." << endl;
		resultadoDisparo = 'A';
		strcpy(codigoBarcoImpactado, "--");
		return true;
	}
	if (estadoCasilla == "O ") {
		cout << "ERROR: Ya habias disparado a esa casilla (agua)." << endl;
		return false;
	}
	if (estadoCasilla == "B ") {
		int indiceBarco = encontrarBarcoEnPosicion(posX, posY);
		if (indiceBarco != -1) {
			registrarDisparo(indiceBarco, posX, posY);
			resultadoDisparo = barcosColocados[indiceBarco].estaHundido ? 'H' : 'T';
			strcpy(codigoBarcoImpactado, barcosColocados[indiceBarco].codigo);
			return true;
		}
	}
	if (estadoCasilla == "X ") {
		cout << "ERROR: Ya habias disparado a esa casilla (acierto previo)." << endl;
		return false;
	}
	if (estadoCasilla == "H ") {
		cout << "ERROR: Ya habias hundido ese barco en esa casilla." << endl;
		return false;
	}
	return false;
}
// ============================================================
//                   FUNCIONES DE COMANDOS
// ============================================================
void registrarJugador() {
	cout << "=== REGISTRO DE JUGADOR ===" << endl;
	cout << "Ingrese su nombre: ";
	cin.getline(nombreJugador, TAM_NOMBRE);
	cout << endl;
	if (strlen(nombreJugador) == 0) {
		strcpy(nombreJugador, "Jugador");
		cout << "No se ingreso un nombre, se asignara 'Jugador' por defecto." << endl;
	}
	jugadorRegistrado = true;
	cout << "Bienvenido, " << nombreJugador << "! Vamos a jugar FunFleet." << endl;
}
/*
* mostrarReporte()
* Muestra un resumen completo de la partida en pantalla.
*/
void mostrarReporte() {
	cout << "\n=== REPORTE DE PARTIDA ===" << endl;
	cout << "Jugador: " << nombreJugador << endl;
	cout << "Turnos maximos: " << MAX_TURNOS << endl;
	cout << "Disparos realizados: " << disparosRealizados << endl;
	cout << "Aciertos: " << aciertosTotales << endl;
	cout << "Fallos: " << fallosTotales << endl;
	cout << "Barcos hundidos: " << barcosHundidos << "/" << cantidadBarcosColocados << endl;
	if (cantidadBarcosColocados > 0 && todosBarcosHundidos()) {
		cout << "\n*** VICTORIA! Hundiste todos los barcos! ***" << endl;
	} else if (disparosRealizados >= MAX_TURNOS) {
		cout << "\n*** Se agotaron los turnos disponibles. ***" << endl;
	}
}
/*
* faseDisparos()
* Menu interactivo donde el jugador dispara contra los barcos del
* oponente. Termina cuando se hunde toda la flota, cuando se agotan
* los MAX_TURNOS turnos disponibles, o cuando el jugador sale.
*/
void faseDisparos() {
	cout << "\n=== FASE DE DISPAROS ===" << endl;
	cout << "Turnos maximos permitidos en esta partida: " << MAX_TURNOS << endl;
	cout << "Comienza la batalla! Dispara coordenadas para hundir barcos." << endl;
	char comando[TAM_COMANDO];
	char comandoMayus[TAM_COMANDO];
	bool seguirJugando = true;
	while (seguirJugando) {
		leerComando(comando, TAM_COMANDO);
		strcpy(comandoMayus, comando);
		aMayusculas(comandoMayus);
		if (strcmp(comandoMayus, "DISPARAR") == 0) {
			int posX = leerCoordenada("Ingrese la posicion X (1-10): ");
			int posY = leerCoordenada("Ingrese la posicion Y (1-10): ");
			char resultado;
			char codigoBarcoImpactado[3];
			if (procesarDisparo(posX, posY, resultado, codigoBarcoImpactado)) {
				// SPRINT 5: el turno ahora se sube por apuntador, no directamente.
				int turnosRestantes = 0;
				actualizarTurnoPtr(&disparosRealizados, &turnosRestantes);
				guardarDisparoBinario(disparosRealizados, posX, posY, resultado, codigoBarcoImpactado);
				// SPRINT 6: el mismo disparo tambien queda en el historial en
				// memoria (arreglo de struct Disparo), para poder recorrerlo
				// con apuntadores sin leer "shots.dat".
				registrarDisparoEnHistorial(disparosRealizados, posX, posY, resultado, codigoBarcoImpactado);
				if (todosBarcosHundidos()) {
					cout << "\n" << nombreJugador << ", GANASTE!! Hundiste todos los barcos!!" << endl;
					exportarReporteAArchivo(ARCHIVO_REPORTE);
					seguirJugando = false;
				} else if (disparosRealizados >= MAX_TURNOS) {
					cout << "\nSe agotaron los " << MAX_TURNOS
					     << " turnos disponibles. Fin de la partida, " << nombreJugador << "." << endl;
					exportarReporteAArchivo(ARCHIVO_REPORTE);
					seguirJugando = false;
				} else {
					cout << "Turno " << disparosRealizados << " de " << MAX_TURNOS
					     << " (" << turnosRestantes << " restantes)." << endl;
				}
			}
		} else if (strcmp(comandoMayus, "TABLERO") == 0) {
			mostrarTablero();
		} else if (strcmp(comandoMayus, "FLOTA_DEV") == 0) {
			// Opcion de desarrollador: revela la flota. Sirve para verificar
			// que la generacion aleatoria o la carga funcionaron, no para jugar.
			cout << "(OPCION DE DESARROLLADOR: esto revela las posiciones)" << endl;
			mostrarTableroJugador();
		} else if (strcmp(comandoMayus, "REPORTE") == 0) {
			mostrarReporte();
		} else if (strcmp(comandoMayus, "CONSULTAR_DISPARO") == 0) {
			consultarDisparoPorTurno();
		} else if (strcmp(comandoMayus, "FLOTA_PTR") == 0) {
			// SPRINT 6: evidencia en pantalla de flota, historial y
			// arreglos de apuntadores.
			mostrarEstadoCompletoPtr();
		} else if (strcmp(comandoMayus, "SELECCIONAR") == 0) {
			// SPRINT 6: agrega un barco a barcosSeleccionadosPtr por codigo.
			if (cantidadBarcosColocados == 0) {
				cout << "ERROR: Todavia no hay barcos colocados." << endl;
			} else {
				cout << "Ingrese el codigo del barco a seleccionar (ej. P, S, A1, D2, F1): ";
				string codigoIngresado;
				getline(cin, codigoIngresado);
				if (codigoIngresado.empty()) {
					cout << "ERROR: Debe ingresar un codigo." << endl;
				} else {
					// aMayusculas espera char[]; se pasa la direccion del primer
					// caracter del string (string es contiguo desde C++11).
					aMayusculas(&codigoIngresado[0]);
					seleccionarBarcoPtr(codigoIngresado.c_str());
				}
			}
		} else if (strcmp(comandoMayus, "GUARDAR") == 0) {
			exportarReporteAArchivo(ARCHIVO_REPORTE);
			exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
		} else if (strcmp(comandoMayus, "GUARDAR_PARTIDA") == 0) {
			guardarPartidaBinaria(ARCHIVO_PARTIDA_BINARIA);
		} else if (strcmp(comandoMayus, "CARGAR_PARTIDA") == 0) {
			cargarPartidaBinaria(ARCHIVO_PARTIDA_BINARIA);
		} else if (strcmp(comandoMayus, "SALIR") == 0) {
			cout << "Partida terminada. Gracias por jugar, " << nombreJugador << "." << endl;
			cout << "(Si querias continuarla despues, habia que usar GUARDAR_PARTIDA antes.)" << endl;
			exportarReporteAArchivo(ARCHIVO_REPORTE);
			seguirJugando = false;
		} else {
			cout << "Comando invalido: \"" << comando << "\"." << endl;
			cout << "Los comandos validos son: DISPARAR, TABLERO, REPORTE, GUARDAR, "
			     << "GUARDAR_PARTIDA, CARGAR_PARTIDA, CONSULTAR_DISPARO, "
			     << "FLOTA_PTR, SELECCIONAR, SALIR." << endl;
			cout << "(FLOTA_DEV existe, pero revela la flota: es solo para depurar.)" << endl;
		}
	}
}
// ============================================================
//                         MAIN
// ============================================================
int main() {
	srand(time(0));  // SPRINT 4: unica llamada a srand() de todo el programa
	// Fase 1: Registro de jugador
	registrarJugador();
	// Fase 2: Inicializar tablero
	inicializarTablero();
	inicializarBarcosColocados();
	// Fase 3: Menu principal
	int opcion;
	do {
		cout << "\n=== BATALLA NAVAL - MENU PRINCIPAL ===" << endl;
		cout << "1. Jugar con flota manual (colocar o cargar y disparar)" << endl;
		cout << "2. Jugar con flota aleatoria" << endl;
		cout << "3. Cargar flota desde \"" << ARCHIVO_FLOTA_ENTRADA << "\"" << endl;
		cout << "4. Ver mi flota" << endl;
		cout << "5. Exportar flota actual a \"" << ARCHIVO_FLOTA_SALIDA << "\"" << endl;
		cout << "6. Generar reporte \"" << ARCHIVO_REPORTE << "\"" << endl;
		cout << "7. Guardar partida binaria (\"" << ARCHIVO_PARTIDA_BINARIA << "\")" << endl;
		cout << "8. Cargar partida binaria (\"" << ARCHIVO_PARTIDA_BINARIA << "\")" << endl;
		cout << "9. Consultar disparo por turno (\"" << ARCHIVO_DISPAROS << "\")" << endl;
		cout << "10. Ver estadisticas con apuntadores (SPRINT 5)" << endl;
		cout << "11. Salir" << endl;
		opcion = leerOpcionValida(1, 11, "Seleccione una opcion: ");
		switch (opcion) {
			case 1:
				// Jugar con flota manual: colocacion propia o carga de texto
				if (cantidadBarcosColocados == 0) {
					prepararFlotaAntesDeJugar();
				} else {
					cout << "\nYa tiene una flota lista con " << cantidadBarcosColocados
					     << " barco(s)." << endl;
				}
				// shots.dat se vacia solo si la partida empieza DE CERO. Si se
				// continua una partida cargada (opcion 8), su historial ya fue
				// reescrito en shots.dat al cargarla y no se debe borrar.
				if (disparosRealizados == 0) {
					reiniciarArchivoDisparos();
				}
				faseDisparos();
				reiniciarPartida();
				break;
			case 2:
				// Jugar con flota aleatoria
				if (cantidadBarcosColocados == 0) {
					generarFlotaAleatoria();
					cout << "\nFlota aleatoria generada con exito (" << cantidadBarcosColocados
					     << "/" << TOTAL_BARCOS_FLOTA << " barcos)." << endl;
					//mostrarTableroJugador();
					int exportar = leerOpcionValida(1, 2,
						"Desea exportar esta flota aleatoria a \""
						+ string(ARCHIVO_FLOTA_SALIDA) + "\"? 1. Si / 2. No: ");
					if (exportar == 1) {
						exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
					}
				} else {
					cout << "\nYa tiene una flota lista con " << cantidadBarcosColocados
					     << " barco(s)." << endl;
				}
				// shots.dat se vacia solo si la partida empieza DE CERO. Si se
				// continua una partida cargada (opcion 8), su historial ya fue
				// reescrito en shots.dat al cargarla y no se debe borrar.
				if (disparosRealizados == 0) {
					reiniciarArchivoDisparos();
				}
				faseDisparos();
				reiniciarPartida();
				break;
			case 3:
				cargarFlotaDesdeArchivo(ARCHIVO_FLOTA_ENTRADA);
				break;
			case 4:
				if (cantidadBarcosColocados == 0) {
					cout << "\nTodavia no hay barcos. Use la opcion 1, 2 o 3." << endl;
				} else {
					mostrarTableroJugador();
				}
				break;
			case 5:
				exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
				break;
			case 6:
				exportarReporteAArchivo(ARCHIVO_REPORTE);
				break;
			case 7:
				guardarPartidaBinaria(ARCHIVO_PARTIDA_BINARIA);
				break;
			case 8:
				cargarPartidaBinaria(ARCHIVO_PARTIDA_BINARIA);
				break;
			case 9:
				consultarDisparoPorTurno();
				break;
			case 10:
				// SPRINT 5: evidencia en pantalla de las funciones con apuntadores
				mostrarEstadisticasConApuntadores();
				break;
			case 11:
				cout << "Gracias por jugar, " << nombreJugador << ". Hasta luego!" << endl;
				return 0;
		}
	} while (opcion != 11);
	return 0;
}