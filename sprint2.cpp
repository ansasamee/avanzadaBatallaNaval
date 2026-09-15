// Código elaborado por David Coy Vélez, Miguel Ángel Martinez y Andrés Santiago Sabogal Meza
// Versión mejorada: Sistema completo de barcos, colocación, disparos y estadísticas

/*
* ============================================================
*  BATALLA NAVAL (FunFleet) - SPRINT 2 MEJORADO
* ============================================================
*  Extensión del Sprint 2 que agrega:
*
*    1) Sistema de tipos de barcos (Crucero, Destructor,
*       Submarino, Lancha) con códigos char únicos.
*    2) Fase de colocación de barcos ANTES de empezar a disparar,
*       con validación de no sobreposición y límites por tipo.
*    3) Sistema mejorado de estados en el tablero:
*       - "~ " = agua (sin disparar)
*       - "B " = barco (sin disparar)
*       - "X " = acierto (barco disparado)
*       - "O " = fallo (agua disparada)
*       - "H " = barco hundido
*    4) Estadísticas completas: aciertos, fallos, barcos hundidos.
*    5) Validaciones robustas en cada paso (límites "anti-bobos").
*
*  Mantiene toda la funcionalidad del Sprint 2 y agrega estas
*  nuevas capas de lógica de forma modular.
* ============================================================
*/

#include <iostream>
#include <cstring>
#include <cctype>
#include <iomanip>
#include <sstream>  // Para validar entradas completas del usuario
#include <cstdlib>  // Para srand() y rand()
#include <ctime>    // Para time()
using namespace std;

// ============================================================
//                    CONSTANTES GLOBALES
// ============================================================
const int TAM_TABLERO = 10;
const int COORD_MIN = 1;
const int COORD_MAX = 10;
const int TAM_NOMBRE = 50;
const int TAM_COMANDO = 20;
const int TAM_TIPO_BARCO = 15;

// Límites de barcos por tipo (validaciones "anti-bobos")
const int MAX_CRUCEROS = 1;       // 1 barco de 4 casillas
const int MAX_DESTRUCTORES = 2;   // 2 barcos de 3 casillas
const int MAX_SUBMARINOS = 3;     // 3 barcos de 2 casillas
const int MAX_LANCHAS = 4;        // 4 barcos de 1 casilla

// ============================================================
//                    ESTRUCTURAS DE DATOS
// ============================================================

// Estructura que representa un barco individual en el tablero
struct Barco {
	char tipo[TAM_TIPO_BARCO];      // "Crucero", "Destructor", "Submarino", "Lancha"
	char codigo;                     // 'C', 'D', 'S', 'L' (para identificar en tablero)
	int tamanio;                     // Cuántas casillas ocupa (4, 3, 2, 1)
	int vidaActual;                  // Cuántas casillas aún no han sido disparadas
	bool estaHundido;                // true si vidaActual == 0
	int posXInicio, posYInicio;      // Coordenada inicial (usuario: 1-10)
	bool esHorizontal;               // true = horizontal, false = vertical
};

// ============================================================
//                    VARIABLES GLOBALES
// ============================================================
bool tableroInicializado = false;
string tablero[TAM_TABLERO][TAM_TABLERO];
string tablerojugador[TAM_TABLERO][TAM_TABLERO];  // Tablero oculto del jugador (ve barcos)

char nombreJugador[TAM_NOMBRE];
bool jugadorRegistrado = false;

// Array dinámico de barcos colocados en el tablero
Barco barcosColocados[MAX_CRUCEROS + MAX_DESTRUCTORES + MAX_SUBMARINOS + MAX_LANCHAS];
int cantidadBarcosColocados = 0;

// Contadores de barcos por tipo (para validar límites)
int crucerosPuestos = 0;
int destructoresPuestos = 0;
int submarinosPuestos = 0;
int lanchasPuestas = 0;

// Estadísticas del juego
int disparosRealizados = 0;
int aciertosTotales = 0;
int fallosTotales = 0;
int barcosHundidos = 0;

// Fase del juego
bool faseColocacionCompleta = false;

// ============================================================
//                    PROTOTIPOS
// ============================================================

// -- Inicialización --
void inicializarTablero();
void inicializarBarcosColocados();

// -- Registro y menú principal --
void registrarJugador();
void menuPrincipal();

// -- Fase de colocación de barcos --
void faseColocacionBarcosCompleta();
void mostrarOpcionesBarcos();
bool intentarColocarBarco(const char tipoBarco[], char codigoBarco, int tamanio);
bool validarColocacionBarco(int posX, int posY, int tamanio, bool esHorizontal);
void colocarBarcoEnTablero(const char tipoBarco[], char codigoBarco, 
                            int posX, int posY, int tamanio, bool esHorizontal);
void crearBarcoEnArray(const char tipoBarco[], char codigoBarco, 
                       int posX, int posY, int tamanio, bool esHorizontal);

// -- Tablero --
void mostrarTablero();
void mostrarTableroJugador();  // Versión que muestra barcos al jugador

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

// -- Utilidades --
void limpiarBufferEntrada();
int leerOpcionValida(int min, int max, const string& mensaje);
int leerCoordenada(const string& mensaje);
bool leerDirectionHorizontal();
void verificarBarcosHundidos();
bool todosBarcosHundidos();

// ============================================================
//                 FUNCIONES DE INICIALIZACIÓN
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
	crucerosPuestos = 0;
	destructoresPuestos = 0;
	submarinosPuestos = 0;
	lanchasPuestas = 0;
}

// ============================================================
//            FUNCIONES DE FASE DE COLOCACIÓN
// ============================================================

/*
* mostrarOpcionesBarcos()
* Muestra al jugador qué barcos puede colocar aún, según los límites.
* Esto previene que intente colocar más de los permitidos.
*/
void mostrarOpcionesBarcos() {
	cout << "\n=== OPCIONES DE BARCOS DISPONIBLES ===" << endl;
	cout << "1. Crucero (4 casillas)        - Disponibles: " 
	     << (MAX_CRUCEROS - crucerosPuestos) << endl;
	cout << "2. Destructor (3 casillas)    - Disponibles: " 
	     << (MAX_DESTRUCTORES - destructoresPuestos) << endl;
	cout << "3. Submarino (2 casillas)     - Disponibles: " 
	     << (MAX_SUBMARINOS - submarinosPuestos) << endl;
	cout << "4. Lancha (1 casilla)         - Disponibles: " 
	     << (MAX_LANCHAS - lanchasPuestas) << endl;
	cout << "5. Terminar colocacion" << endl;
}

/*
* validarColocacionBarco()
* Valida que:
*   - El barco no salga del tablero (1-10).
*   - No se solape con otros barcos ya colocados.
*   - La posición sea válida (dentro de rango).
*
* Retorna true si la colocación es válida, false en caso contrario.
*/
bool validarColocacionBarco(int posX, int posY, int tamanio, bool esHorizontal) {
	// Validar que las coordenadas estén en rango 1-10
	if (posX < COORD_MIN || posX > COORD_MAX || posY < COORD_MIN || posY > COORD_MAX) {
		cout << "ERROR: Las coordenadas deben estar entre 1 y 10." << endl;
		return false;
	}

	// Convertir a índices de array (0-9)
	int fila = posY - 1;
	int col = posX - 1;

	// Validar que el barco no salga del tablero en su dirección
	if (esHorizontal) {
		if (col + tamanio > TAM_TABLERO) {
			cout << "ERROR: El barco se saldría del tablero hacia la derecha." << endl;
			return false;
		}
	} else {
		if (fila + tamanio > TAM_TABLERO) {
			cout << "ERROR: El barco se saldría del tablero hacia abajo." << endl;
			return false;
		}
	}

	// Verificar que no haya sobreposición con otros barcos
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
* con su código de tipo (C, D, S, L).
*/
void colocarBarcoEnTablero(const char tipoBarco[], char codigoBarco,
                            int posX, int posY, int tamanio, bool esHorizontal) {
	int fila = posY - 1;
	int col = posX - 1;

	if (esHorizontal) {
		for (int i = 0; i < tamanio; i++) {
			string marcaBarco = "";
			marcaBarco += codigoBarco;
			marcaBarco += " ";
			tablerojugador[fila][col + i] = marcaBarco;
			tablero[fila][col + i] = "B ";
		}
	} else {
		for (int i = 0; i < tamanio; i++) {
			string marcaBarco = "";
			marcaBarco += codigoBarco;
			marcaBarco += " ";
			tablerojugador[fila + i][col] = marcaBarco;
			tablero[fila + i][col] = "B ";
		}
	}
}

/*
* crearBarcoEnArray()
* Crea un objeto Barco con la información suministrada y lo agrega
* al array de barcosColocados. Esto permite rastrear la salud de cada
* barco y detectar cuándo se hunde.
*/
void crearBarcoEnArray(const char tipoBarco[], char codigoBarco,
                       int posX, int posY, int tamanio, bool esHorizontal) {
	Barco nuevoBarco;
	strcpy(nuevoBarco.tipo, tipoBarco);
	nuevoBarco.codigo = codigoBarco;
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
* Flujo completo para colocar un barco:
*   1. Pedir coordenadas x, y.
*   2. Pedir dirección (horizontal/vertical).
*   3. Validar colocación.
*   4. Si es válida, marcar en el tablero y agregar al array.
*   5. Incrementar contador del tipo de barco.
*
* Retorna true si el barco se colocó exitosamente.
*/
bool intentarColocarBarco(const char tipoBarco[], char codigoBarco, int tamanio) {
	int posX = leerCoordenada("Ingrese la posicion X (1-10): ");
	int posY = leerCoordenada("Ingrese la posicion Y (1-10): ");
	bool esHorizontal = leerDirectionHorizontal();

	if (!validarColocacionBarco(posX, posY, tamanio, esHorizontal)) {
		cout << "No se pudo colocar el barco. Intente nuevamente." << endl;
		return false;
	}

	colocarBarcoEnTablero(tipoBarco, codigoBarco, posX, posY, tamanio, esHorizontal);
	crearBarcoEnArray(tipoBarco, codigoBarco, posX, posY, tamanio, esHorizontal);

	cout << "¡" << tipoBarco << " colocado exitosamente!" << endl;
	return true;
}

/*
* faseColocacionBarcosCompleta()
* Menú interactivo donde el jugador coloca todos sus barcos antes
* de empezar la fase de disparos. El jugador puede:
*   - Elegir qué tipo de barco colocar (si aún hay disponibles).
*   - Ver el tablero con sus barcos.
*   - Terminar cuando haya colocado al menos un barco.
*/
void faseColocacionBarcosCompleta() {
	cout << "\n=== FASE DE COLOCACION DE BARCOS ===" << endl;
	cout << "Coloque sus barcos en el tablero. (Mínimo 1 barco para empezar)." << endl;

	bool terminoColocacion = false;
	while (!terminoColocacion) {
		mostrarOpcionesBarcos();
		int opcion = leerOpcionValida(1, 5, "Seleccione una opcion: ");

		switch (opcion) {
			case 1:  // Crucero
				if (crucerosPuestos < MAX_CRUCEROS) {
					if (intentarColocarBarco("Crucero", 'C', 4)) {
						crucerosPuestos++;
						mostrarTableroJugador();
					}
				} else {
					cout << "ERROR: Ya ha colocado el máximo de Cruceros (" << MAX_CRUCEROS << ")." << endl;
				}
				break;

			case 2:  // Destructor
				if (destructoresPuestos < MAX_DESTRUCTORES) {
					if (intentarColocarBarco("Destructor", 'D', 3)) {
						destructoresPuestos++;
						mostrarTableroJugador();
					}
				} else {
					cout << "ERROR: Ya ha colocado el máximo de Destructores (" << MAX_DESTRUCTORES << ")." << endl;
				}
				break;

			case 3:  // Submarino
				if (submarinosPuestos < MAX_SUBMARINOS) {
					if (intentarColocarBarco("Submarino", 'S', 2)) {
						submarinosPuestos++;
						mostrarTableroJugador();
					}
				} else {
					cout << "ERROR: Ya ha colocado el máximo de Submarinos (" << MAX_SUBMARINOS << ")." << endl;
				}
				break;

			case 4:  // Lancha
				if (lanchasPuestas < MAX_LANCHAS) {
					if (intentarColocarBarco("Lancha", 'L', 1)) {
						lanchasPuestas++;
						mostrarTableroJugador();
					}
				} else {
					cout << "ERROR: Ya ha colocado el máximo de Lanchas (" << MAX_LANCHAS << ")." << endl;
				}
				break;

			case 5:  // Terminar
				if (cantidadBarcosColocados > 0) {
					cout << "\n¿Está seguro de terminar? Tiene " << cantidadBarcosColocados 
					     << " barco(s) colocado(s)." << endl;
					int confirmacion = leerOpcionValida(1, 2, "1. Si, empezar / 2. No, seguir colocando: ");
					if (confirmacion == 1) {
						terminoColocacion = true;
						faseColocacionCompleta = true;
						cout << "\n¡Colocacion completada! Prepárese para disparar." << endl;
					}
				} else {
					cout << "ERROR: Debe colocar al menos 1 barco antes de comenzar." << endl;
				}
				break;
		}
	}
}

// ============================================================
//                   FUNCIONES DE DISPAROS
// ============================================================

/*
* encontrarBarcoEnPosicion()
* Busca en el array de barcosColocados si hay algún barco en la
* posición (posX, posY). Retorna el índice del barco si lo encuentra,
* o -1 si no hay barco en esa posición.
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

	cout << "¡ACIERTO! Le diste al " << barco.tipo << "." << endl;

	if (barco.vidaActual == 0) {
		barco.estaHundido = true;
		barcosHundidos++;
		marcarBarcoHundido(indiceBarco);
		cout << "¡¡" << barco.tipo << " HUNDIDO!!" << endl;
	} else {
		cout << "Vida restante del " << barco.tipo << ": " << barco.vidaActual << endl;
	}
}

/*
* procesarDisparo()
* Procesa un disparo en las coordenadas (posX, posY):
*   - Si hay agua y no fue disparada: marca como agua disparada (O), +1 fallo
*   - Si hay agua y ya fue disparada: avisa que ya se disparó allí
*   - Si hay barco: llama a registrarDisparo() para restar vida
*   - Si hay barco ya disparado: avisa que ya se disparó allí
*
* Retorna true si el disparo fue válido (sin importar si acertó o falló).
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
		cout << "¡AGUA! El disparo no acertó nada." << endl;
		return true;
	}

	// Caso 2: Agua ya disparada
	if (estadoCasilla == "O ") {
		cout << "ERROR: Ya habías disparado a esa casilla (agua)." << endl;
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
		cout << "ERROR: Ya habías disparado a esa casilla (acierto previo)." << endl;
		return false;
	}

	// Caso 5: Barco hundido
	if (estadoCasilla == "H ") {
		cout << "ERROR: Ya habías hundido ese barco en esa casilla." << endl;
		return false;
	}

	return false;
}

// ============================================================
//                   FUNCIONES DE TABLERO
// ============================================================

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
				cout << " " << tablero[fila][columna] << " ";
			}
			cout << endl;
		}
	} else {
		cout << "El tablero no se ha inicializado." << endl;
	}
}

/*
* mostrarTableroJugador()
* Versión del tablero que muestra los barcos del jugador (B, C, D, S, L)
* así como las posiciones que ha disparado. Se usa durante colocación y
* para que el jugador pueda ver su posición actual.
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

	cout << "\nIngrese un comando (DISPARAR, TABLERO, REPORTE, SALIR): ";
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
* Muestra un resumen completo de la partida: nombre del jugador,
* cantidad de disparos realizados, aciertos, fallos, barcos hundidos,
* y si todos los barcos enemigos (teóricos) han sido hundidos.
*/
void mostrarReporte() {
	cout << "\n=== REPORTE DE PARTIDA ===" << endl;
	cout << "Jugador: " << nombreJugador << endl;
	cout << "Disparos realizados: " << disparosRealizados << endl;
	cout << "Aciertos: " << aciertosTotales << endl;
	cout << "Fallos: " << fallosTotales << endl;
	cout << "Barcos hundidos: " << barcosHundidos << "/" << cantidadBarcosColocados << endl;

	if (todosBarcosHundidos()) {
		cout << "\n*** ¡¡VICTORIA!! ¡¡Hundiste todos los barcos!! ***" << endl;
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
* Se usa para detectar condición de victoria.
*/
bool todosBarcosHundidos() {
	return barcosHundidos == cantidadBarcosColocados;
}

/*
* faseDisparos()
* Menú interactivo donde el jugador dispara contra los barcos del oponente.
* El menú de comandos es similar al del Sprint 2, pero ahora DISPARAR se refiere
* a disparar contra barcos colocados en el tablero actual.
*/
void faseDisparos() {
	cout << "\n=== FASE DE DISPAROS ===" << endl;
	cout << "¡Comienza la batalla! Dispara coordenadas para hundir barcos." << endl;

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
					cout << "\n¡¡" << nombreJugador << ", GANASTE!! ¡¡Hundiste todos los barcos!!" << endl;
					seguirJugando = false;
				}
			}

		} else if (strcmp(comandoMayus, "TABLERO") == 0) {
			mostrarTablero();

		} else if (strcmp(comandoMayus, "REPORTE") == 0) {
			mostrarReporte();

		} else if (strcmp(comandoMayus, "SALIR") == 0) {
			cout << "Juego pausado. Gracias por jugar, " << nombreJugador << "." << endl;
			seguirJugando = false;

		} else {
			cout << "Comando invalido: \"" << comando << "\"." << endl;
			cout << "Los comandos validos son: DISPARAR, TABLERO, REPORTE, SALIR." << endl;
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
* Lee una coordenada individual (X o Y) del usuario, validando que sea numérica
* y esté en el rango 1-10. Si hay error, vuelve a pedir.
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
* o vertical (2). Valida que sea una opción válida.
*/
bool leerDirectionHorizontal() {
	int opcion = leerOpcionValida(1, 2, "Direccion: 1. Horizontal / 2. Vertical: ");
	return opcion == 1;
}

// ============================================================
//                         MAIN
// ============================================================

int main() {
	srand(time(0));  // Inicializar semilla de números aleatorios (si se usa después)

	// Fase 1: Registro de jugador
	registrarJugador();

	// Fase 2: Inicializar tablero
	inicializarTablero();
	inicializarBarcosColocados();

	// Fase 3: Menú principal
	int opcion;
	do {
		cout << "\n=== BATALLA NAVAL - MENU PRINCIPAL ===" << endl;
		cout << "1. Jugar" << endl;
		cout << "2. Salir" << endl;

		opcion = leerOpcionValida(1, 2, "Seleccione una opcion: ");

		switch (opcion) {
			case 1:
				// Fase 4: Colocar barcos
				faseColocacionBarcosCompleta();

				// Fase 5: Disparar y hundir barcos
				faseDisparos();

				// Reiniciar para otra partida
				inicializarTablero();
				inicializarBarcosColocados();
				disparosRealizados = 0;
				aciertosTotales = 0;
				fallosTotales = 0;
				barcosHundidos = 0;
				faseColocacionCompleta = false;
				break;

			case 2:
				cout << "Gracias por jugar, " << nombreJugador << ". ¡Hasta luego!" << endl;
				return 0;
		}

	} while (opcion != 2);

	return 0;
}
