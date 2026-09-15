// Codigo elaborado por David Coy Velez, Miguel Angel Martinez y Andres Santiago Sabogal Meza
// Version mejorada: Sistema completo de barcos, colocacion, disparos, estadisticas y archivos de texto

/*
* ============================================================
*  BATALLA NAVAL (FunFleet) - SPRINT 3
* ============================================================
*  Mantiene TODO lo del Sprint 2 (colocacion, disparos,
*  hundimientos, estadisticas y validaciones) y agrega el tema
*  central del Sprint 3: ARCHIVOS DE TEXTO CON ACCESO SECUENCIAL.
*
*    1) Carga manual de la flota desde "fleet-grid.txt".
*    2) Generacion de flota ALEATORIA, exportada automaticamente
*       a "ai-fleet-grid.txt".
*    3) Exportacion de la flota actual a "ai-fleet-grid.txt".
*    4) Reporte basico de la partida en "funfleet-report.txt",
*       incluyendo jugador, turnos usados y el tablero.
*    5) Validacion de CANTIDAD y TAMANO de cada barco al leer el
*       archivo (numero de casillas, linea recta, sin huecos,
*       sin superposicion y sin salirse del tablero 10x10).
*
*  Correcciones aplicadas sobre la version anterior:
*    - El tablero de combate YA NO revela donde estan los barcos
*      sin disparar (se ocultan como agua hasta que se disparan).
*    - Se agrego el modo de flota ALEATORIA.
*    - El reporte de texto ahora incluye el tablero con formato
*      de filas y columnas (igual que en pantalla).
*
*  Ademas, la flota se alineo a la del enunciado oficial:
*    P (4), S (3), A1 (3), A2 (3), D1 (2), D2 (2), D3 (2),
*    F1 (1), F2 (1)  ->  9 barcos en total.
*
*  Formato del archivo de texto (acceso secuencial):
*    10 lineas, cada una con 10 tokens separados por espacios.
*    Tokens validos: ~  P  S  A1  A2  D1  D2  D3  F1  F2
*
*  Compilacion:  g++ sprint3.cpp -o sprint3
*  Ejecucion:    ./sprint3      (los .txt se leen/escriben en la
*                                misma carpeta del ejecutable)
* ============================================================
*/

#include <iostream>
#include <cstring>
#include <cctype>
#include <iomanip>
#include <sstream>  // Para validar entradas completas del usuario
#include <cstdlib>  // Para srand() y rand()
#include <ctime>    // Para time()
#include <fstream>  // SPRINT 3: lectura/escritura de archivos de texto
#include <string>   // SPRINT 3: manejo de tokens leidos del archivo
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

// -- SPRINT 3: nombres de los archivos de texto del proyecto --
const char ARCHIVO_FLOTA_ENTRADA[] = "fleet-grid.txt";      // Se carga (si existe)
const char ARCHIVO_FLOTA_SALIDA[]  = "ai-fleet-grid.txt";   // Se exporta
const char ARCHIVO_REPORTE[]       = "funfleet-report.txt"; // Reporte basico

// ============================================================
//                    ESTRUCTURAS DE DATOS
// ============================================================

// Estructura que representa un barco individual en el tablero
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
*   - Saber el TAMANO obligatorio de cada barco.
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
//                    PROTOTIPOS
// ============================================================

// -- Inicializacion --
void inicializarTablero();
void inicializarBarcosColocados();

// -- Registro y menu principal --
void registrarJugador();
void menuPrincipal();

// -- Fase de colocacion de barcos --
void faseColocacionBarcosCompleta();
void mostrarOpcionesBarcos();
bool intentarColocarBarco(int indiceTipo);
bool validarColocacionBarco(int posX, int posY, int tamanio, bool esHorizontal);
void colocarBarcoEnTablero(const char codigoBarco[],
                            int posX, int posY, int tamanio, bool esHorizontal);
void crearBarcoEnArray(const char tipoBarco[], const char codigoBarco[],
                       int posX, int posY, int tamanio, bool esHorizontal);

// -- SPRINT 3 (correccion): generacion aleatoria de la flota --
void colocarBarcosAleatoriamente();

// -- Tablero --
void mostrarTablero();
void mostrarTableroJugador();  // Version que muestra barcos al jugador (solo en colocacion)
string formatoCeldaCombate(const string& celda);
void escribirTableroEnArchivo(ofstream& archivo, string grid[][TAM_TABLERO], bool ocultarBarcos);

// -- Disparos --
void faseDisparos();
bool procesarDisparo(int posX, int posY);
int encontrarBarcoEnPosicion(int posX, int posY);
void registrarDisparo(int indiceBarco, int posX, int posY);
void marcarBarcoHundido(int indiceBarco);

// -- Comandos de texto --
void leerComando(char comando[], int tam);
void aMayusculas(char cadena[]);
void menuComandos();
void mostrarReporte();

// -- SPRINT 3: archivos de texto con acceso secuencial --
bool existeArchivo(const char nombreArchivo[]);
int buscarTipoPorCodigo(const string& codigo);
string codigoAnchoDos(const string& codigo);
string quitarEspacios(const string& texto);
bool esLineaVacia(const string& linea);
bool leerGridDesdeArchivo(const char nombreArchivo[], string gridLeido[][TAM_TABLERO]);
bool validarGridDeFlota(string gridLeido[][TAM_TABLERO]);
void aplicarGridAlJuego(string gridLeido[][TAM_TABLERO]);
bool cargarFlotaDesdeArchivo(const char nombreArchivo[]);
bool exportarFlotaAArchivo(const char nombreArchivo[]);
bool exportarReporteAArchivo(const char nombreArchivo[]);
void prepararFlotaAntesDeJugar();

// -- Utilidades --
void limpiarBufferEntrada();
int leerOpcionValida(int min, int max, const string& mensaje);
int leerCoordenada(const string& mensaje);
bool leerDirectionHorizontal();
void verificarBarcosHundidos();
bool todosBarcosHundidos();
void reiniciarPartida();

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
}

// ============================================================
//            FUNCIONES DE FASE DE COLOCACION
// ============================================================

/*
* mostrarOpcionesBarcos()
* Muestra la flota completa indicando cuales barcos ya fueron colocados
* y cuales siguen pendientes. Asi el jugador nunca puede repetir un barco
* (validacion de CANTIDAD) ni inventarse un tamano (validacion de TAMANO).
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
* validarColocacionBarco()
* Valida que:
*   - El barco no salga del tablero (1-10).
*   - No se solape con otros barcos ya colocados.
*   - La posicion sea valida (dentro de rango).
*
* Retorna true si la colocacion es valida, false en caso contrario.
*/
bool validarColocacionBarco(int posX, int posY, int tamanio, bool esHorizontal) {
	// Validar que las coordenadas esten en rango 1-10
	if (posX < COORD_MIN || posX > COORD_MAX || posY < COORD_MIN || posY > COORD_MAX) {
		cout << "ERROR: Las coordenadas deben estar entre 1 y 10." << endl;
		return false;
	}

	// Convertir a indices de array (0-9)
	int fila = posY - 1;
	int col = posX - 1;

	// Validar que el barco no salga del tablero en su direccion
	if (esHorizontal) {
		if (col + tamanio > TAM_TABLERO) {
			cout << "ERROR: El barco se saldria del tablero hacia la derecha." << endl;
			return false;
		}
	} else {
		if (fila + tamanio > TAM_TABLERO) {
			cout << "ERROR: El barco se saldria del tablero hacia abajo." << endl;
			return false;
		}
	}

	// Verificar que no haya sobreposicion con otros barcos
	if (esHorizontal) {
		for (int i = 0; i < tamanio; i++) {
			if (tablerojugador[fila][col + i] != "~ ") {
				cout << "ERROR: Ya hay un barco en esa posicion (casilla "
				     << (col + i + 1) << ", " << (fila + 1) << ")." << endl;
				return false;
			}
		}
	} else {
		for (int i = 0; i < tamanio; i++) {
			if (tablerojugador[fila + i][col] != "~ ") {
				cout << "ERROR: Ya hay un barco en esa posicion (casilla "
				     << col + 1 << ", " << (fila + i + 1) << ")." << endl;
				return false;
			}
		}
	}

	return true;
}

/*
* colocarBarcoEnTablero()
* Marca en el tablero del jugador todas las casillas que ocupa el barco
* con su codigo de tipo (P, S, A1, A2, D1, D2, D3, F1, F2).
* Los codigos se guardan con ancho 2 ("P " o "A1") para que el tablero
* siga alineado en pantalla.
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
* crearBarcoEnArray()
* Crea un objeto Barco con la informacion suministrada y lo agrega
* al array de barcosColocados. Esto permite rastrear la salud de cada
* barco y detectar cuando se hunde.
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
* intentarColocarBarco()
* Flujo completo para colocar un barco del catalogo:
*   1. Pedir coordenadas x, y.
*   2. Pedir direccion (horizontal/vertical).
*   3. Validar colocacion.
*   4. Si es valida, marcar en el tablero y agregar al array.
*
* Retorna true si el barco se coloco exitosamente.
*/
bool intentarColocarBarco(int indiceTipo) {
	const TipoBarco& tipo = CATALOGO_FLOTA[indiceTipo];

	cout << "\n-- Colocando " << tipo.nombre << " [" << tipo.codigo
	     << "] de " << tipo.tamanio << " casilla(s) --" << endl;

	int posX = leerCoordenada("Ingrese la posicion X (1-10): ");
	int posY = leerCoordenada("Ingrese la posicion Y (1-10): ");

	// Un barco de 1 casilla no necesita direccion, se asume horizontal
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
* Menu interactivo donde el jugador coloca sus barcos antes
* de empezar la fase de disparos. El jugador puede:
*   - Elegir que barco de la flota colocar (si aun esta pendiente).
*   - Ver el tablero con sus barcos (util mientras los ubica).
*   - Terminar cuando haya colocado al menos un barco.
*
* Mostrar el tablero aqui es correcto: el jugador esta ubicando SUS
* propios barcos y necesita ver donde los va poniendo. Distinto es
* mostrar el tablero ya en la fase de disparos, donde si se ocultan
* los barcos (ver mostrarTablero()).
*/
void faseColocacionBarcosCompleta() {
	cout << "\n=== FASE DE COLOCACION DE BARCOS ===" << endl;
	cout << "Coloque sus barcos en el tablero. (Minimo 1 barco para empezar)." << endl;

	bool terminoColocacion = false;
	while (!terminoColocacion) {
		mostrarOpcionesBarcos();
		int opcion = leerOpcionValida(1, TOTAL_BARCOS_FLOTA + 1, "Seleccione una opcion: ");

		if (opcion == TOTAL_BARCOS_FLOTA + 1) {
			// Opcion "Terminar colocacion"
			if (cantidadBarcosColocados == 0) {
				cout << "ERROR: Debe colocar al menos 1 barco antes de comenzar." << endl;
				continue;
			}

			cout << "\nEsta seguro de terminar? Tiene " << cantidadBarcosColocados
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
			// Opciones 1..9 -> barcos del catalogo
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
* colocarBarcosAleatoriamente()
* CORRECCION: genera una flota completa y valida colocando los 9 barcos
* del catalogo en posiciones y direcciones aleatorias.
*
* Para cada barco, sortea X, Y y direccion con rand() y solo la acepta
* si cabe dentro del tablero (1-10) y no se superpone con otro barco ya
* colocado; si no sirve, se vuelve a sortear hasta encontrar una posicion
* valida. No imprime nada en pantalla (el tablero resultante NO se
* muestra al jugador, ver correccion de mostrarTablero()).
*/
void colocarBarcosAleatoriamente() {
	inicializarTablero();
	inicializarBarcosColocados();

	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		const TipoBarco& tipo = CATALOGO_FLOTA[i];
		bool colocado = false;

		while (!colocado) {
			int posX = (rand() % TAM_TABLERO) + 1;
			int posY = (rand() % TAM_TABLERO) + 1;
			bool esHorizontal = (rand() % 2 == 0);
			if (tipo.tamanio == 1) {
				esHorizontal = true;
			}

			int fila = posY - 1;
			int col = posX - 1;

			bool cabeEnTablero = esHorizontal
				? (col + tipo.tamanio <= TAM_TABLERO)
				: (fila + tipo.tamanio <= TAM_TABLERO);
			if (!cabeEnTablero) {
				continue;
			}

			bool haySuperposicion = false;
			if (esHorizontal) {
				for (int k = 0; k < tipo.tamanio; k++) {
					if (tablerojugador[fila][col + k] != "~ ") {
						haySuperposicion = true;
						break;
					}
				}
			} else {
				for (int k = 0; k < tipo.tamanio; k++) {
					if (tablerojugador[fila + k][col] != "~ ") {
						haySuperposicion = true;
						break;
					}
				}
			}
			if (haySuperposicion) {
				continue;
			}

			colocarBarcoEnTablero(tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
			crearBarcoEnArray(tipo.nombre, tipo.codigo, posX, posY, tipo.tamanio, esHorizontal);
			barcoYaColocado[i] = true;
			colocado = true;
		}
	}
}

// ============================================================
//   SPRINT 3: ARCHIVOS DE TEXTO CON ACCESO SECUENCIAL
// ============================================================

/*
* existeArchivo()
* Retorna true si el archivo se puede abrir para lectura.
* Se usa para preguntar al jugador si desea cargar la flota ya guardada.
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
* Sirve para tolerar lineas vacias al final del archivo.
*/
bool esLineaVacia(const string& linea) {
	for (int i = 0; i < (int)linea.length(); i++) {
		if (!isspace(static_cast<unsigned char>(linea[i]))) {
			return false;
		}
	}
	return true;
}

/*
* leerGridDesdeArchivo()
* PASO 1 de la carga: lee el archivo de texto de forma SECUENCIAL
* (linea por linea con getline) y guarda los 100 tokens en gridLeido.
*
* Valida en esta etapa:
*   - Que el archivo se pueda abrir.
*   - Que existan exactamente 10 filas y 10 columnas.
*   - Que cada token sea "~" o un codigo valido del catalogo.
*
* NO modifica el juego: primero se lee todo y luego se valida, para no
* danar la partida actual si el archivo esta mal.
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
		// Saltar lineas en blanco intermedias
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

		// Si sobra algo despues de la columna 10, la fila es invalida
		string sobrante;
		if (separador >> sobrante) {
			cout << "ERROR: La fila " << (fila + 1) << " tiene mas de "
			     << TAM_TABLERO << " columnas." << endl;
			archivo.close();
			return false;
		}
	}

	// Revisar que no haya filas extra con contenido
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
* PASO 2 de la carga: valida la CANTIDAD y el TAMANO de cada barco.
*
* Para cada barco del catalogo revisa que:
*   - Aparezca en el tablero (la flota debe estar completa: 9 barcos).
*   - Ocupe exactamente el numero de casillas de su tamano
*     (ej: P debe aparecer 4 veces, D1 exactamente 2 veces).
*   - Sus casillas formen una linea recta horizontal o vertical.
*   - No queden huecos en el medio (casillas contiguas).
*
* La superposicion es imposible en este formato, porque cada casilla del
* archivo guarda un solo codigo.
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

		// Validacion de CANTIDAD: el barco debe existir
		if (casillasEncontradas == 0) {
			cout << "ERROR: Falta el barco " << tipo.nombre
			     << " [" << tipo.codigo << "] en el archivo." << endl;
			todoCorrecto = false;
			continue;
		}

		// Validacion de TAMANO: numero exacto de casillas
		if (casillasEncontradas != tipo.tamanio) {
			cout << "ERROR: El barco " << tipo.nombre << " [" << tipo.codigo
			     << "] debe ocupar " << tipo.tamanio << " casilla(s) y ocupa "
			     << casillasEncontradas << "." << endl;
			todoCorrecto = false;
			continue;
		}

		// Validacion de FORMA: linea recta horizontal o vertical
		bool esHorizontal = (filaMin == filaMax);
		bool esVertical = (colMin == colMax);

		if (!esHorizontal && !esVertical) {
			cout << "ERROR: El barco " << tipo.nombre << " [" << tipo.codigo
			     << "] no esta en linea recta (debe ser horizontal o vertical)." << endl;
			todoCorrecto = false;
			continue;
		}

		// Validacion de CONTIGUIDAD: no puede tener huecos
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

		// Un barco de 1 casilla se considera horizontal por convencion
		bool esHorizontal = (filaMin == filaMax);

		// Coordenadas del usuario (1-10)
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
* Si algo falla, el juego queda como estaba antes de intentar cargar.
*
* CORRECCION: ya NO se imprime el tablero al terminar de cargar. Antes
* se llamaba mostrarTableroJugador(), lo cual revelaba la posicion de
* todos los barcos justo antes de empezar a jugar.
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

	// Al cargar una flota nueva, las estadisticas de disparos se reinician
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
* entrada (10 filas x 10 columnas separadas por espacios). Escritura
* secuencial con ofstream.
*/
bool exportarFlotaAArchivo(const char nombreArchivo[]) {
	if (cantidadBarcosColocados == 0) {
		cout << "ERROR: No hay barcos colocados. Primero coloque, cargue o genere una flota." << endl;
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
* formatoCeldaCombate()
* CORRECCION CLAVE: dado el contenido real de una celda del tablero de
* combate, decide que se le muestra al jugador. Si la celda tiene un
* barco que todavia no ha sido disparado ("B "), se disfraza como agua
* ("~ ") para que el jugador nunca vea donde estan los barcos antes de
* golpearlos. Los estados ya conocidos (O, X, H) se muestran tal cual.
*/
string formatoCeldaCombate(const string& celda) {
	if (celda == "B ") {
		return "~ ";
	}
	return celda;
}

/*
* escribirTableroEnArchivo()
* Escribe un tablero 10x10 en un archivo de texto ya abierto, con
* encabezados de columnas (C1..C10) y filas (F1..F10), igual que se ve
* en pantalla. Si ocultarBarcos es true, usa formatoCeldaCombate() para
* no revelar los barcos sin disparar (se usa para el tablero de combate
* del reporte).
*/
void escribirTableroEnArchivo(ofstream& archivo, string grid[][TAM_TABLERO], bool ocultarBarcos) {
	archivo << "     ";
	for (int columna = 0; columna < TAM_TABLERO; columna++) {
		archivo << "C" << (columna + 1) << "  ";
	}
	archivo << endl;

	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		if (fila + 1 < 10) archivo << " F" << (fila + 1) << "  ";
		else archivo << "F" << (fila + 1) << "  ";

		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			string celda = grid[fila][columna];
			if (ocultarBarcos) {
				celda = formatoCeldaCombate(celda);
			}
			archivo << celda << " ";
		}
		archivo << endl;
	}
}

/*
* exportarReporteAArchivo()
* Genera el reporte basico de la partida en un archivo de texto:
* jugador, turnos usados, estadisticas, estado de cada barco y los dos
* tableros con formato de filas/columnas (CORRECCION: antes se escribia
* el tablero en crudo, sin encabezados, y era dificil de leer).
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
	archivo << "Barcos en la flota   : " << cantidadBarcosColocados
	        << "/" << TOTAL_BARCOS_FLOTA << endl;
	archivo << "Turnos usados        : " << disparosRealizados << endl;
	archivo << "Aciertos             : " << aciertosTotales << endl;
	archivo << "Fallos               : " << fallosTotales << endl;
	archivo << "Barcos hundidos      : " << barcosHundidos << endl;

	if (cantidadBarcosColocados > 0 && todosBarcosHundidos()) {
		archivo << "Resultado            : VICTORIA (flota hundida)" << endl;
	} else {
		archivo << "Resultado            : PARTIDA EN CURSO" << endl;
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
	archivo << "--- TABLERO DE LA FLOTA (posiciones reales) ---" << endl;
	escribirTableroEnArchivo(archivo, tablerojugador, false);

	archivo << endl;
	archivo << "--- TABLERO DE COMBATE (lo que ve el jugador) ---" << endl;
	archivo << "Leyenda: ~ sin disparar | O fallo (agua) | X tocado | H hundido" << endl;
	escribirTableroEnArchivo(archivo, tablero, true);

	archivo.close();

	cout << "Reporte generado correctamente en \"" << nombreArchivo << "\"." << endl;
	return true;
}

/*
* prepararFlotaAntesDeJugar()
* Antes de disparar, decide de donde sale la flota:
*   - Si "fleet-grid.txt" existe: cargarlo, colocar manualmente o
*     generar una flota aleatoria.
*   - Si no existe: colocar manualmente o generar una flota aleatoria.
*
* CORRECCION: cuando la flota se genera aleatoriamente, se exporta
* automaticamente a "ai-fleet-grid.txt" (sin preguntar) y en ningun
* caso se muestra el tablero con las posiciones al jugador.
*/
void prepararFlotaAntesDeJugar() {
	bool flotaLista = false;
	bool flotaYaExportada = false;

	if (existeArchivo(ARCHIVO_FLOTA_ENTRADA)) {
		cout << "\nSe encontro el archivo \"" << ARCHIVO_FLOTA_ENTRADA << "\"." << endl;
		cout << "1. Cargar la flota del archivo" << endl;
		cout << "2. Colocar los barcos manualmente" << endl;
		cout << "3. Generar una flota aleatoria" << endl;
		int opcion = leerOpcionValida(1, 3, "Seleccione una opcion: ");

		if (opcion == 1) {
			flotaLista = cargarFlotaDesdeArchivo(ARCHIVO_FLOTA_ENTRADA);
			if (!flotaLista) {
				cout << "Se continuara con la colocacion manual." << endl;
			}
		} else if (opcion == 3) {
			colocarBarcosAleatoriamente();
			cout << "\nFlota aleatoria generada: " << cantidadBarcosColocados
			     << " barco(s) listos para la batalla." << endl;
			exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
			flotaLista = true;
			flotaYaExportada = true;
		}
	} else {
		cout << "\nNo se encontro \"" << ARCHIVO_FLOTA_ENTRADA << "\"." << endl;
		cout << "1. Colocar los barcos manualmente" << endl;
		cout << "2. Generar una flota aleatoria" << endl;
		int opcion = leerOpcionValida(1, 2, "Seleccione una opcion: ");

		if (opcion == 2) {
			colocarBarcosAleatoriamente();
			cout << "\nFlota aleatoria generada: " << cantidadBarcosColocados
			     << " barco(s) listos para la batalla." << endl;
			exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
			flotaLista = true;
			flotaYaExportada = true;
		}
	}

	if (!flotaLista) {
		faseColocacionBarcosCompleta();
	}

	if (!flotaYaExportada) {
		int exportar = leerOpcionValida(1, 2,
			"\nDesea exportar esta flota a un archivo de texto? 1. Si / 2. No: ");
		if (exportar == 1) {
			exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
		}
	}
}

// ============================================================
//                   FUNCIONES DE DISPAROS
// ============================================================

/*
* encontrarBarcoEnPosicion()
* Busca en el array de barcosColocados si hay algun barco en la
* posicion (posX, posY). Retorna el indice del barco si lo encuentra,
* o -1 si no hay barco en esa posicion.
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
* registrarDisparo()
* Reduce la vida del barco en 1. Si la vida llega a 0, marca el barco
* como hundido. Se invoca cuando un disparo acierta a un barco.
*/
void registrarDisparo(int indiceBarco, int posX, int posY) {
	Barco& barco = barcosColocados[indiceBarco];

	// Evitar que la vida del barco pueda quedar en un valor negativo.
	if (barco.vidaActual > 0) {
		barco.vidaActual--;
	}

	int fila = posY - 1;
	int col = posX - 1;

	// Marcar en el tablero de juego (no el del jugador) que hubo un acierto
	tablero[fila][col] = "X ";
	aciertosTotales++;

	cout << "TOCADO! Le diste al " << barco.tipo << " [" << barco.codigo << "]." << endl;

	if (barco.vidaActual == 0) {
		barco.estaHundido = true;
		barcosHundidos++;
		marcarBarcoHundido(indiceBarco);
		cout << barco.tipo << " HUNDIDO!!" << endl;
	} else {
		cout << "Vida restante del " << barco.tipo << ": " << barco.vidaActual << endl;
	}
}

/*
* procesarDisparo()
* Procesa un disparo en las coordenadas (posX, posY):
*   - Si hay agua y no fue disparada: marca como agua disparada (O), +1 fallo
*   - Si hay agua y ya fue disparada: avisa que ya se disparo alli
*   - Si hay barco: llama a registrarDisparo() para restar vida
*   - Si hay barco ya disparado: avisa que ya se disparo alli
*
* Retorna true si el disparo fue valido (sin importar si acerto o fallo).
*/
/*
 * marcarBarcoHundido()
 * Cambia a "H " todas las casillas del barco cuando ya no tiene vida.
 * Esto permite identificar claramente el barco que ya fue hundido.
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

bool procesarDisparo(int posX, int posY) {
	// Validar rangos
	if (posX < COORD_MIN || posX > COORD_MAX || posY < COORD_MIN || posY > COORD_MAX) {
		cout << "ERROR: Coordenadas fuera de rango (1-10)." << endl;
		return false;
	}

	int fila = posY - 1;
	int col = posX - 1;
	string estadoCasilla = tablero[fila][col];

	// Caso 1: Agua sin disparar
	if (estadoCasilla == "~ ") {
		tablero[fila][col] = "O ";
		fallosTotales++;
		cout << "AGUA! El disparo no acerto nada." << endl;
		return true;
	}

	// Caso 2: Agua ya disparada
	if (estadoCasilla == "O ") {
		cout << "ERROR: Ya habias disparado a esa casilla (agua)." << endl;
		return false;
	}

	// Caso 3: Barco sin disparar
	if (estadoCasilla == "B ") {
		int indiceBarco = encontrarBarcoEnPosicion(posX, posY);
		if (indiceBarco != -1) {
			registrarDisparo(indiceBarco, posX, posY);
			return true;
		}
	}

	// Caso 4: Acierto repetido
	if (estadoCasilla == "X ") {
		cout << "ERROR: Ya habias disparado a esa casilla (acierto previo)." << endl;
		return false;
	}

	// Caso 5: Barco hundido
	if (estadoCasilla == "H ") {
		cout << "ERROR: Ya habias hundido ese barco en esa casilla." << endl;
		return false;
	}

	return false;
}

// ============================================================
//                   FUNCIONES DE TABLERO
// ============================================================

/*
* mostrarTablero()
* Tablero de combate que ve el jugador durante la fase de disparos.
*
* CORRECCION IMPORTANTE: antes esta funcion imprimia el contenido real
* del tablero, que incluia "B " en las casillas donde habia un barco
* sin disparar todavia. Eso revelaba la posicion de toda la flota con
* solo escribir el comando TABLERO. Ahora, con formatoCeldaCombate(),
* cualquier barco que no ha sido tocado se disfraza como agua ("~ ")
* y solo se revela cuando el jugador realmente le dispara.
*/
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
				cout << " " << formatoCeldaCombate(tablero[fila][columna]) << " ";
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
* D1, D2, D3, F1, F2). Solo se usa DURANTE la colocacion manual, para
* que el jugador vea donde va ubicando sus propios barcos. No se llama
* en ningun punto de la fase de disparos ni despues de cargar/generar
* una flota, para no revelar posiciones antes de jugar.
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

void leerComando(char comando[], int tam) {
	string entrada;

	cout << "\nIngrese un comando (DISPARAR, TABLERO, REPORTE, GUARDAR, SALIR): ";
	getline(cin, entrada);

	// Evitar desbordamiento del arreglo de caracteres.
	if (entrada.length() >= static_cast<size_t>(tam)) {
		entrada = entrada.substr(0, tam - 1);
	}

	strcpy(comando, entrada.c_str());
	cout << endl;
}

void aMayusculas(char cadena[]) {
	for (int i = 0; cadena[i] != '\0'; i++) {
		cadena[i] = static_cast<char>(toupper(static_cast<unsigned char>(cadena[i])));
	}
}

/*
* mostrarReporte()
* Muestra un resumen en pantalla de la partida: nombre del jugador,
* turnos usados, aciertos, fallos, barcos hundidos. No muestra el
* tablero (para eso esta el archivo funfleet-report.txt, que si lo
* incluye con el formato completo).
*/
void mostrarReporte() {
	cout << "\n=== REPORTE DE PARTIDA ===" << endl;
	cout << "Jugador: " << nombreJugador << endl;
	cout << "Turnos usados: " << disparosRealizados << endl;
	cout << "Aciertos: " << aciertosTotales << endl;
	cout << "Fallos: " << fallosTotales << endl;
	cout << "Barcos hundidos: " << barcosHundidos << "/" << cantidadBarcosColocados << endl;

	if (cantidadBarcosColocados > 0 && todosBarcosHundidos()) {
		cout << "\n*** VICTORIA! Hundiste todos los barcos! ***" << endl;
	}
}

/*
* verificarBarcosHundidos()
* Itera el array de barcosColocados y marca como hundido a aquellos
* que tengan vidaActual == 0. Se invoca cada vez que se hace un disparo.
*/
void verificarBarcosHundidos() {
	for (int i = 0; i < cantidadBarcosColocados; i++) {
		if (barcosColocados[i].vidaActual == 0 && !barcosColocados[i].estaHundido) {
			barcosColocados[i].estaHundido = true;
			barcosHundidos++;
		}
	}
}

/*
* todosBarcosHundidos()
* Retorna true si todos los barcos colocados han sido hundidos, false en caso contrario.
* Se usa para detectar condicion de victoria.
*/
bool todosBarcosHundidos() {
	if (cantidadBarcosColocados == 0) {
		return false;
	}
	return barcosHundidos == cantidadBarcosColocados;
}

/*
* faseDisparos()
* Menu interactivo donde el jugador dispara contra los barcos de su
* propio tablero (batalla naval en solitario: el reto es hundir la
* flota que se cargo, coloco o genero, sin ver donde esta).
*
* Comandos disponibles:
*   DISPARAR -> dispara a una coordenada.
*   TABLERO  -> muestra el tablero de combate (barcos ocultos).
*   REPORTE  -> muestra estadisticas en pantalla.
*   GUARDAR  -> escribe funfleet-report.txt y ai-fleet-grid.txt.
*   SALIR    -> termina la partida.
*
* CORRECCION: se elimino el comando FLOTA que existia antes, ya que
* mostraba el tablero completo con los barcos y permitia "hacer trampa"
* durante la partida.
*/
void faseDisparos() {
	cout << "\n=== FASE DE DISPAROS ===" << endl;
	cout << "Comienza la batalla! Dispara coordenadas para hundir barcos." << endl;

	char comando[TAM_COMANDO];
	char comandoMayus[TAM_COMANDO];
	bool seguirJugando = true;

	while (seguirJugando && !todosBarcosHundidos()) {
		leerComando(comando, TAM_COMANDO);
		// El uso de getline() evita dejar caracteres pendientes en el buffer.

		strcpy(comandoMayus, comando);
		aMayusculas(comandoMayus);

		if (strcmp(comandoMayus, "DISPARAR") == 0) {
			int posX = leerCoordenada("Ingrese la posicion X (1-10): ");
			int posY = leerCoordenada("Ingrese la posicion Y (1-10): ");

			if (procesarDisparo(posX, posY)) {
				disparosRealizados++;
				verificarBarcosHundidos();

				if (todosBarcosHundidos()) {
					cout << "\n" << nombreJugador << ", GANASTE!! Hundiste todos los barcos!!" << endl;
					// Se genera el reporte final automaticamente (Sprint 3)
					exportarReporteAArchivo(ARCHIVO_REPORTE);
					seguirJugando = false;
				}
			}

		} else if (strcmp(comandoMayus, "TABLERO") == 0) {
			mostrarTablero();

		} else if (strcmp(comandoMayus, "REPORTE") == 0) {
			mostrarReporte();

		} else if (strcmp(comandoMayus, "GUARDAR") == 0) {
			exportarReporteAArchivo(ARCHIVO_REPORTE);
			exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);

		} else if (strcmp(comandoMayus, "SALIR") == 0) {
			cout << "Juego pausado. Gracias por jugar, " << nombreJugador << "." << endl;
			// Se guarda el reporte antes de salir (Sprint 3)
			exportarReporteAArchivo(ARCHIVO_REPORTE);
			seguirJugando = false;

		} else {
			cout << "Comando invalido: \"" << comando << "\"." << endl;
			cout << "Los comandos validos son: DISPARAR, TABLERO, REPORTE, GUARDAR, SALIR." << endl;
		}
	}
}

// ============================================================
//                   FUNCIONES AUXILIARES
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
		getline(cin, entrada);

		// Verificar que la entrada no este vacia
		if (entrada.empty()) {
			cout << "Entrada invalida. Debe ingresar un numero entero." << endl;
			continue;
		}

		// Intentar convertir la entrada a un numero entero
		stringstream conversor(entrada);

		if (!(conversor >> opcion)) {
			cout << "Entrada invalida. Debe ingresar un numero entero." << endl;
			continue;
		}

		// Verificar que despues del numero no haya ningun caracter adicional
		if (conversor >> caracterExtra) {
			cout << "Entrada invalida. No se permiten letras, puntos ni caracteres adicionales." << endl;
			continue;
		}

		// Verificar que el numero este dentro del rango permitido
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
* Lee una coordenada individual (X o Y) del usuario, validando que sea
* numerica y este en el rango 1-10. Si hay error, vuelve a pedir.
*/
int leerCoordenada(const string& mensaje) {
	string entrada;
	int coordenada;
	char caracterExtra;
	bool coordenadaValida = false;

	while (!coordenadaValida) {
		cout << mensaje;
		getline(cin, entrada);

		// Verificar que la entrada no este vacia
		if (entrada.empty()) {
			cout << "ERROR: Debes ingresar un numero entero." << endl;
			continue;
		}

		// Intentar convertir la entrada a un numero entero
		stringstream conversor(entrada);

		if (!(conversor >> coordenada)) {
			cout << "ERROR: Debes ingresar un numero entero." << endl;
			continue;
		}

		// Verificar que despues del numero no haya ningun caracter adicional
		if (conversor >> caracterExtra) {
			cout << "ERROR: La coordenada debe contener solamente un numero entero." << endl;
			continue;
		}

		// Verificar que la coordenada este dentro del tablero
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

// ============================================================
//                         MAIN
// ============================================================

int main() {
	srand(time(0));  // Inicializar semilla de numeros aleatorios (para el modo aleatorio)

	// Fase 1: Registro de jugador
	registrarJugador();

	// Fase 2: Inicializar tablero
	inicializarTablero();
	inicializarBarcosColocados();

	// Fase 3: Menu principal
	int opcion;
	do {
		cout << "\n=== BATALLA NAVAL - MENU PRINCIPAL ===" << endl;
		cout << "1. Jugar (colocar, cargar o generar flota y disparar)" << endl;
		cout << "2. Cargar flota desde \"" << ARCHIVO_FLOTA_ENTRADA << "\"" << endl;
		cout << "3. Generar flota aleatoria (exporta a \"" << ARCHIVO_FLOTA_SALIDA << "\")" << endl;
		cout << "4. Exportar flota actual a \"" << ARCHIVO_FLOTA_SALIDA << "\"" << endl;
		cout << "5. Generar reporte \"" << ARCHIVO_REPORTE << "\"" << endl;
		cout << "6. Salir" << endl;

		opcion = leerOpcionValida(1, 6, "Seleccione una opcion: ");

		switch (opcion) {
			case 1:
				// Fase 4: Obtener la flota (archivo, manual o aleatoria)
				if (cantidadBarcosColocados == 0) {
					prepararFlotaAntesDeJugar();
				} else {
					cout << "\nYa tiene una flota lista con " << cantidadBarcosColocados
					     << " barco(s)." << endl;
				}

				// Fase 5: Disparar y hundir barcos
				faseDisparos();

				// Reiniciar para otra partida
				reiniciarPartida();
				break;

			case 2:
				cargarFlotaDesdeArchivo(ARCHIVO_FLOTA_ENTRADA);
				break;

			case 3:
				colocarBarcosAleatoriamente();
				cout << "\nFlota aleatoria generada: " << cantidadBarcosColocados
				     << " barco(s)." << endl;
				exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
				break;

			case 4:
				exportarFlotaAArchivo(ARCHIVO_FLOTA_SALIDA);
				break;

			case 5:
				exportarReporteAArchivo(ARCHIVO_REPORTE);
				break;

			case 6:
				cout << "Gracias por jugar, " << nombreJugador << ". Hasta luego!" << endl;
				return 0;
		}

	} while (opcion != 6);

	return 0;
}
