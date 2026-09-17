#include <iostream>
#include <cstring>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <map>
using namespace std;
const int TAM_TABLERO = 10;
const int COORD_MIN = 1;
const int COORD_MAX = 10;
const int TAM_NOMBRE = 50;
const int TAM_COMANDO = 20;
const int TAM_TIPO_BARCO = 20;
const int MAX_TURNOS = 60;
const char ARCHIVO_FLOTA_ENTRADA[] = "fleet-grid.txt";
const char ARCHIVO_FLOTA_SALIDA[]  = "ai-fleet-grid.txt";
const char ARCHIVO_REPORTE[]       = "funfleet-report.txt";
const char ARCHIVO_DISPAROS[]         = "shots.dat";
const char ARCHIVO_PARTIDA_BINARIA[]  = "funfleet-save.dat";

struct Barco {
	char tipo[TAM_TIPO_BARCO];
	char codigo[3];
	int tamanio;
	int vidaActual;
	bool estaHundido;
	int posXInicio, posYInicio;
	bool esHorizontal;
};
struct TipoBarco {
	char codigo[3];
	char nombre[TAM_TIPO_BARCO];
	int tamanio;
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
struct RegistroDisparo {
	int turno;
	int x;
	int y;
	char resultado;
	char codigoBarco[3];
};
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
	char firma[4];
	char nombreJugador[TAM_NOMBRE];
	int tipoColocacion;
	int turnosRealizados;
	int aciertosGuardados;
	int fallosGuardados;
	int barcosHundidosGuardados;
	int cantidadBarcosGuardados;
	BarcoBinario barcos[TOTAL_BARCOS_FLOTA];
	char gridBarcos[TAM_TABLERO][TAM_TABLERO][3];
	char gridDisparos[TAM_TABLERO][TAM_TABLERO];
	int cantidadHistorialGuardado;
};

struct Disparo {
	int turno;
	int x;
	int y;
	char resultado;
	char codigoBarco[3];
};

struct Partida {
	char nombreJugador[TAM_NOMBRE];
	int turnosRealizados;
	int totalBarcos;
	int barcosActivos;
	int barcosHundidos;
	int vidaTotalRestante;
};

bool tableroInicializado = false;
string** gridDisparos = NULL;
string** gridBarcos = NULL;
char nombreJugador[TAM_NOMBRE];
bool jugadorRegistrado = false;
Barco barcosColocados[TOTAL_BARCOS_FLOTA];
int cantidadBarcosColocados = 0;
bool barcoYaColocado[TOTAL_BARCOS_FLOTA];
int disparosRealizados = 0;
int aciertosTotales = 0;
int fallosTotales = 0;
int barcosHundidos = 0;
bool faseColocacionCompleta = false;

struct NodoDisparo {
	Disparo dato;
	NodoDisparo* siguiente;
};

NodoDisparo* primerDisparo = NULL;
NodoDisparo* ultimoDisparo = NULL;
int cantidadHistorial = 0;

Barco* barcosSeleccionadosPtr[TOTAL_BARCOS_FLOTA];
int cantidadBarcosSeleccionadosPtr = 0;

const int COLOCACION_NINGUNA = 0;
const int COLOCACION_MANUAL = 1;
const int COLOCACION_ARCHIVO = 2;
const int COLOCACION_ALEATORIA = 3;
int tipoColocacion = COLOCACION_NINGUNA;

const char FIRMA_PARTIDA[] = "FF7";

int reservasMemoria = 0;
int liberacionesMemoria = 0;

string** crearGrid(int filas, int columnas) {
	string** grid = new string*[filas];
	reservasMemoria++;
	grid[0] = new string[filas * columnas];
	reservasMemoria++;
	for (int fila = 1; fila < filas; fila++) {
		grid[fila] = grid[0] + fila * columnas;
	}
	return grid;
}

void liberarGrid(string**& grid) {
	if (grid == NULL) {
		return;
	}
	delete[] grid[0];
	liberacionesMemoria++;
	delete[] grid;
	liberacionesMemoria++;
	grid = NULL;
}

void registrarDisparoEnHistorial(int turno, int x, int y, char resultado,
                                  const char codigoBarco[]) {
	NodoDisparo* nuevo = new NodoDisparo;
	reservasMemoria++;
	nuevo->dato.turno = turno;
	nuevo->dato.x = x;
	nuevo->dato.y = y;
	nuevo->dato.resultado = resultado;
	strncpy(nuevo->dato.codigoBarco, codigoBarco, 2);
	nuevo->dato.codigoBarco[2] = '\0';
	nuevo->siguiente = NULL;
	if (primerDisparo == NULL) {
		primerDisparo = nuevo;
	} else {
		ultimoDisparo->siguiente = nuevo;
	}
	ultimoDisparo = nuevo;
	cantidadHistorial++;
}

void liberarHistorial() {
	NodoDisparo* actual = primerDisparo;
	while (actual != NULL) {
		NodoDisparo* siguiente = actual->siguiente;
		delete actual;
		liberacionesMemoria++;
		actual = siguiente;
	}
	primerDisparo = NULL;
	ultimoDisparo = NULL;
	cantidadHistorial = 0;
}

void liberarMemoria() {
	liberarHistorial();
	liberarGrid(gridBarcos);
	liberarGrid(gridDisparos);
}

string nombreTipoColocacion(int tipo) {
	if (tipo == COLOCACION_MANUAL) return "Manual";
	if (tipo == COLOCACION_ARCHIVO) return "Desde archivo";
	if (tipo == COLOCACION_ALEATORIA) return "Aleatoria";
	return "Sin flota";
}

string textoResultado(char resultado) {
	if (resultado == 'A') return "Agua";
	if (resultado == 'T') return "Tocado";
	if (resultado == 'H') return "Hundido";
	return "Desconocido";
}

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
	     << " | FLOTA_PTR, SELECCIONAR, MEMORIA | FLOTA_DEV): ";
	if (!getline(cin, entrada)) {
		cout << "\nERROR: No hay mas entrada disponible. Cerrando el programa." << endl;
		exit(0);
	}
	if (entrada.length() >= static_cast<size_t>(tam)) {
		entrada = entrada.substr(0, tam - 1);
	}
	strcpy(comando, entrada.c_str());
	cout << endl;
}
bool existeArchivo(const char nombreArchivo[]) {
	ifstream archivo(nombreArchivo);
	bool existe = archivo.is_open();
	archivo.close();
	return existe;
}
int buscarTipoPorCodigo(const string& codigo) {
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		if (codigo == CATALOGO_FLOTA[i].codigo) {
			return i;
		}
	}
	return -1;
}
string codigoAnchoDos(const string& codigo) {
	string resultado = codigo;
	while (resultado.length() < 2) {
		resultado += " ";
	}
	return resultado;
}
string quitarEspacios(const string& texto) {
	string limpio = "";
	for (int i = 0; i < (int)texto.length(); i++) {
		if (texto[i] != ' ') {
			limpio += texto[i];
		}
	}
	return limpio;
}
bool esLineaVacia(const string& linea) {
	for (int i = 0; i < (int)linea.length(); i++) {
		if (!isspace(static_cast<unsigned char>(linea[i]))) {
			return false;
		}
	}
	return true;
}
void inicializarTablero() {
	tableroInicializado = true;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			gridDisparos[fila][columna] = "~ ";
			gridBarcos[fila][columna] = "~ ";
		}
	}
}
void inicializarBarcosColocados() {
	cantidadBarcosColocados = 0;
	for (int i = 0; i < TOTAL_BARCOS_FLOTA; i++) {
		barcoYaColocado[i] = false;
	}
}
bool todosBarcosHundidos() {
	if (cantidadBarcosColocados == 0) {
		return false;
	}
	return barcosHundidos == cantidadBarcosColocados;
}
void reiniciarArchivoDisparos() {
	ofstream archivo(ARCHIVO_DISPAROS, ios::binary | ios::trunc);
	if (!archivo.is_open()) {
		cout << "ADVERTENCIA: No se pudo reiniciar el archivo \"" << ARCHIVO_DISPAROS << "\"." << endl;
		return;
	}
	archivo.close();
}
void reiniciarPartida() {
	inicializarTablero();
	inicializarBarcosColocados();
	disparosRealizados = 0;
	aciertosTotales = 0;
	fallosTotales = 0;
	barcosHundidos = 0;
	faseColocacionCompleta = false;

	liberarHistorial();
	cantidadBarcosSeleccionadosPtr = 0;
	tipoColocacion = COLOCACION_NINGUNA;
}
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
				cout << " " << celdaVisibleParaJugador(gridDisparos[fila][columna]) << " ";
			}
			cout << endl;
		}
	} else {
		cout << "El tablero no se ha inicializado." << endl;
	}
}
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
				cout << " " << gridBarcos[fila][columna] << " ";
			}
			cout << endl;
		}
	}
}

void contarCasillasFlotaPtr(string* gridBarcos, int totalCasillas,
                             int* totalConBarco, int* totalAgua) {
	*totalConBarco = 0;
	*totalAgua = 0;
	string* actual = gridBarcos;
	for (int i = 0; i < totalCasillas; i++) {
		if (*actual != "~ ") {
			(*totalConBarco)++;
		} else {
			(*totalAgua)++;
		}
		actual++;
	}
}
void contarResultadosDisparosPtr(string* gridDisparos, int totalCasillas,
                                  int* totalFallos, int* totalTocados,
                                  int* totalHundidos) {
	*totalFallos = 0;
	*totalTocados = 0;
	*totalHundidos = 0;
	string* fin = gridDisparos + totalCasillas;
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
void contarSinDispararPorFilaPtr(string** gridDisparos, int totalFilas,
                                  int resultadoPorFila[]) {
	for (int fila = 0; fila < totalFilas; fila++) {
		resultadoPorFila[fila] = 0;
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			if ((*gridDisparos)[columna] == "~ " || (*gridDisparos)[columna] == "B ") {
				resultadoPorFila[fila]++;
			}
		}
		gridDisparos++;
	}
}
void mostrarEstadisticasConApuntadores() {
	if (!tableroInicializado) {
		cout << "\nEl tablero no se ha inicializado todavia." << endl;
		return;
	}
	int totalCasillas = TAM_TABLERO * TAM_TABLERO;
	int casillasConBarco = 0, casillasAgua = 0;
	contarCasillasFlotaPtr(&gridBarcos[0][0], totalCasillas,
	                       &casillasConBarco, &casillasAgua);
	int fallos = 0, tocados = 0, hundidosCasillas = 0;
	contarResultadosDisparosPtr(&gridDisparos[0][0], totalCasillas,
	                            &fallos, &tocados, &hundidosCasillas);
	int sinDisparar = 0;
	contarCasillasSinDispararPtr(&gridDisparos[0][0], totalCasillas, &sinDisparar);
	cout << "\n=== ESTADISTICAS CON APUNTADORES (SPRINT 5) ===" << endl;
	cout << "gridBarcos -> casillas con barco: " << casillasConBarco
	     << " | casillas de agua: " << casillasAgua << endl;
	cout << "gridDisparos -> fallos (O): " << fallos
	     << " | tocados (X): " << tocados
	     << " | hundidos (H): " << hundidosCasillas
	     << " | sin disparar aun: " << sinDisparar << endl;
	int hundidos = 0, aFlote = 0, vidaRestante = 0;
	resumenFlotaPtr(barcosColocados, cantidadBarcosColocados,
	                &hundidos, &aFlote, &vidaRestante);
	cout << "flota (arreglo de Barco) -> hundidos: " << hundidos
	     << " | a flote: " << aFlote
	     << " | casillas de vida restantes: " << vidaRestante << endl;
	int filasSinDisparar[TAM_TABLERO];
	contarSinDispararPorFilaPtr(gridDisparos, TAM_TABLERO, filasSinDisparar);
	cout << "gridDisparos por fila (apuntador a apuntador) -> casillas sin disparar: ";
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		cout << "F" << (fila + 1) << "=" << filasSinDisparar[fila] << " ";
	}
	cout << endl;
}
void actualizarTurnoPtr(int* turnosUsados, int* turnosRestantes) {
	(*turnosUsados)++;
	*turnosRestantes = MAX_TURNOS - *turnosUsados;
}

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

void mostrarHistorialPtr(NodoDisparo* inicio) {
	cout << "\n--- HISTORIAL DE DISPAROS (lista enlazada, del mas antiguo al mas reciente) ---" << endl;
	if (inicio == NULL) {
		cout << "Aun no se ha registrado ningun disparo en esta partida." << endl;
		return;
	}
	for (NodoDisparo* actual = inicio; actual != NULL; actual = actual->siguiente) {
		cout << "Turno " << actual->dato.turno << ": (" << actual->dato.x << "," << actual->dato.y
		     << ") -> " << textoResultado(actual->dato.resultado);
		if (actual->dato.resultado != 'A') {
			cout << " [" << actual->dato.codigoBarco << "]";
		}
		cout << endl;
	}
}

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

map<string, Barco*> construirMapaBarcosPorCodigoPtr(Barco* flota, int cantidad) {
	map<string, Barco*> mapaCodigos;
	Barco* fin = flota + cantidad;
	for (Barco* actual = flota; actual < fin; actual++) {
		mapaCodigos[string(actual->codigo)] = actual;
	}
	return mapaCodigos;
}

void mostrarEstadoCompletoPtr() {
	if (cantidadBarcosColocados == 0) {
		cout << "\nTodavia no hay una flota colocada." << endl;
		return;
	}

	mostrarDetalleFlotaPtr(barcosColocados, cantidadBarcosColocados);
	mostrarHistorialPtr(primerDisparo);

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
	Barco* ultimoBarco = barcosColocados + (cantidadBarcosColocados - 1);
	string codigoBuscado = ultimoBarco->codigo;
	int comparaciones = 0;
	Barco* porLineal = NULL;
	for (Barco* actual = barcosColocados; actual <= ultimoBarco; actual++) {
		comparaciones++;
		if (codigoBuscado == actual->codigo) {
			porLineal = actual;
			break;
		}
	}
	map<string, Barco*>::iterator encontrado = mapaCodigos.find(codigoBuscado);
	Barco* porMap = (encontrado != mapaCodigos.end()) ? encontrado->second : NULL;
	cout << "\n--- COMPARACION: busqueda lineal vs std::map<codigo, Barco*> ---" << endl;
	cout << "Codigo buscado: " << codigoBuscado << endl;
	cout << "Busqueda lineal en el arreglo: " << comparaciones << " comparaciones -> "
	     << (porLineal != NULL ? porLineal->tipo : "no encontrado") << endl;
	cout << "Busqueda en std::map (arbol de " << mapaCodigos.size() << " claves) -> "
	     << (porMap != NULL ? porMap->tipo : "no encontrado") << endl;
	cout << "Ambas busquedas apuntan al mismo barco: " << (porLineal == porMap ? "si" : "no") << endl;
}

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
void colocarBarcoEnTablero(const char codigoBarco[],
                            int posX, int posY, int tamanio, bool esHorizontal) {
	int fila = posY - 1;
	int col = posX - 1;
	string marcaBarco = codigoAnchoDos(codigoBarco);

	if (esHorizontal) {
		for (int i = 0; i < tamanio; i++) {
			gridBarcos[fila][col + i] = marcaBarco;
			gridDisparos[fila][col + i] = "B ";
		}
	} else {
		for (int i = 0; i < tamanio; i++) {
			gridBarcos[fila + i][col] = marcaBarco;
			gridDisparos[fila + i][col] = "B ";
		}
	}
}
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
			if (gridBarcos[fila][col + i] != "~ ") {
				if (mostrarErrores) {
					cout << "ERROR: Ya hay un barco en esa posicion (casilla "
					     << (col + i + 1) << ", " << (fila + 1) << ")." << endl;
				}
				return false;
			}
		}
	} else {
		for (int i = 0; i < tamanio; i++) {
			if (gridBarcos[fila + i][col] != "~ ") {
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
				tipoColocacion = COLOCACION_MANUAL;
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
				exito = false;
			}
		}
	}
	faseColocacionCompleta = true;
	tipoColocacion = COLOCACION_ALEATORIA;
}
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
	tipoColocacion = COLOCACION_ARCHIVO;
	liberarHistorial();
	cantidadBarcosSeleccionadosPtr = 0;
	cout << "Flota cargada correctamente: " << cantidadBarcosColocados
	     << " barcos validados." << endl;
	return true;
}
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
			archivo << quitarEspacios(gridBarcos[fila][columna]);
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
double calcularPrecision() {
	if (disparosRealizados == 0) {
		return 0.0;
	}
	return (aciertosTotales * 100.0) / disparosRealizados;
}
void escribirGridEnArchivo(ofstream& archivo, string** grid) {
	archivo << "     ";
	for (int columna = 0; columna < TAM_TABLERO; columna++) {
		archivo << "C" << (columna + 1) << (columna + 1 < 10 ? "  " : " ");
	}
	archivo << endl;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		archivo << (fila + 1 < 10 ? " F" : "F") << (fila + 1) << "  ";
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			archivo << grid[fila][columna] << "  ";
		}
		archivo << endl;
	}
}
bool exportarReporteAArchivo(const char nombreArchivo[]) {
	ofstream archivo(nombreArchivo);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo crear el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	archivo << "============================================" << endl;
	archivo << "          FUNFLEET - REPORTE FINAL          " << endl;
	archivo << "============================================" << endl;
	archivo << "Jugador              : " << nombreJugador << endl;
	archivo << "Tipo de colocacion   : " << nombreTipoColocacion(tipoColocacion) << endl;
	archivo << "Barcos en la flota   : " << cantidadBarcosColocados
	        << "/" << TOTAL_BARCOS_FLOTA << endl;
	archivo << "Turnos maximos       : " << MAX_TURNOS << endl;
	if (cantidadBarcosColocados > 0 && todosBarcosHundidos()) {
		archivo << "Resultado            : VICTORIA (flota hundida)" << endl;
	} else if (disparosRealizados >= MAX_TURNOS) {
		archivo << "Resultado            : DERROTA (turnos agotados)" << endl;
	} else {
		archivo << "Resultado            : PARTIDA EN CURSO" << endl;
	}
	archivo << endl;
	archivo << "--- ESTADISTICAS ---" << endl;
	archivo << "Disparos realizados  : " << disparosRealizados << endl;
	archivo << "Aciertos             : " << aciertosTotales << endl;
	archivo << "Agua                 : " << fallosTotales << endl;
	archivo << "Barcos hundidos      : " << barcosHundidos << "/" << cantidadBarcosColocados << endl;
	archivo << "Precision            : " << fixed << setprecision(1) << calcularPrecision() << "%" << endl;
	archivo << endl;
	archivo << "--- FLOTA ---" << endl;
	archivo << "CODIGO  BARCO           TAM  VIDA  ESTADO   INICIO(X,Y)  DIRECCION" << endl;
	Barco* fin = barcosColocados + cantidadBarcosColocados;
	for (Barco* barco = barcosColocados; barco < fin; barco++) {
		archivo << left << setw(8) << barco->codigo
		        << setw(16) << barco->tipo
		        << setw(5) << barco->tamanio
		        << setw(6) << barco->vidaActual
		        << setw(9) << (barco->estaHundido ? "HUNDIDO" : "A FLOTE")
		        << "(" << barco->posXInicio << "," << barco->posYInicio << ")"
		        << "        " << (barco->esHorizontal ? "Horizontal" : "Vertical")
		        << endl;
	}
	archivo << right;
	archivo << endl;
	archivo << "--- GRID DE BARCOS (gridBarcos) ---" << endl;
	escribirGridEnArchivo(archivo, gridBarcos);
	archivo << endl;
	archivo << "--- GRID DE DISPAROS (gridDisparos) ---" << endl;
	archivo << "Leyenda: ~ agua | B barco | O fallo | X tocado | H hundido" << endl;
	escribirGridEnArchivo(archivo, gridDisparos);
	archivo << endl;
	archivo << "--- HISTORIAL DE MOVIMIENTOS (del mas antiguo al mas reciente) ---" << endl;
	if (primerDisparo == NULL) {
		archivo << "Sin disparos registrados." << endl;
	} else {
		archivo << left << setw(7) << "TURNO" << setw(10) << "(X,Y)" << setw(11) << "RESULTADO" << "BARCO" << endl;
		for (NodoDisparo* actual = primerDisparo; actual != NULL; actual = actual->siguiente) {
			string coordenada = "(" + to_string(actual->dato.x) + "," + to_string(actual->dato.y) + ")";
			archivo << left << setw(7) << actual->dato.turno
			        << setw(10) << coordenada
			        << setw(11) << textoResultado(actual->dato.resultado)
			        << actual->dato.codigoBarco << endl;
		}
		archivo << right;
	}
	archivo.close();
	cout << "Reporte generado correctamente en \"" << nombreArchivo << "\"." << endl;
	return true;
}
bool guardarPartidaBinaria(const char nombreArchivo[]) {
	if (cantidadBarcosColocados == 0) {
		cout << "ERROR: No hay una partida activa para guardar (no hay barcos colocados)." << endl;
		return false;
	}
	PartidaBinaria datos;
	memset(&datos, 0, sizeof(PartidaBinaria));
	strcpy(datos.firma, FIRMA_PARTIDA);
	strncpy(datos.nombreJugador, nombreJugador, TAM_NOMBRE - 1);
	datos.tipoColocacion = tipoColocacion;
	datos.turnosRealizados = disparosRealizados;
	datos.aciertosGuardados = aciertosTotales;
	datos.fallosGuardados = fallosTotales;
	datos.barcosHundidosGuardados = barcosHundidos;
	datos.cantidadBarcosGuardados = cantidadBarcosColocados;
	Barco* origen = barcosColocados;
	BarcoBinario* destino = datos.barcos;
	for (int i = 0; i < cantidadBarcosColocados; i++, origen++, destino++) {
		strcpy(destino->codigo, origen->codigo);
		strncpy(destino->tipo, origen->tipo, TAM_TIPO_BARCO - 1);
		destino->tamanio = origen->tamanio;
		destino->vidaActual = origen->vidaActual;
		destino->estaHundido = origen->estaHundido;
		destino->posXInicio = origen->posXInicio;
		destino->posYInicio = origen->posYInicio;
		destino->esHorizontal = origen->esHorizontal;
	}
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			string codigo = quitarEspacios(gridBarcos[fila][columna]);
			strncpy(datos.gridBarcos[fila][columna], codigo.c_str(), 2);
			datos.gridDisparos[fila][columna] = gridDisparos[fila][columna][0];
		}
	}
	datos.cantidadHistorialGuardado = cantidadHistorial;
	ofstream archivo(nombreArchivo, ios::binary | ios::trunc);
	if (!archivo.is_open()) {
		cout << "ERROR: No se pudo crear el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	archivo.write(reinterpret_cast<char*>(&datos), sizeof(PartidaBinaria));
	for (NodoDisparo* actual = primerDisparo; actual != NULL; actual = actual->siguiente) {
		RegistroDisparo registro;
		memset(&registro, 0, sizeof(RegistroDisparo));
		registro.turno = actual->dato.turno;
		registro.x = actual->dato.x;
		registro.y = actual->dato.y;
		registro.resultado = actual->dato.resultado;
		strcpy(registro.codigoBarco, actual->dato.codigoBarco);
		archivo.write(reinterpret_cast<char*>(&registro), sizeof(RegistroDisparo));
	}
	archivo.close();
	cout << "Partida guardada correctamente en \"" << nombreArchivo << "\" ("
	     << cantidadHistorial << " disparos en el historial)." << endl;
	return true;
}
bool cargarPartidaBinaria(const char nombreArchivo[]) {
	ifstream archivo(nombreArchivo, ios::binary);
	if (!archivo.is_open()) {
		cout << "ERROR: No existe el archivo \"" << nombreArchivo << "\"." << endl;
		return false;
	}
	PartidaBinaria datos;
	archivo.read(reinterpret_cast<char*>(&datos), sizeof(PartidaBinaria));
	if (archivo.gcount() != static_cast<streamsize>(sizeof(PartidaBinaria)) ||
	    strncmp(datos.firma, FIRMA_PARTIDA, 4) != 0) {
		cout << "ERROR: El archivo \"" << nombreArchivo
		     << "\" esta danado, incompleto o es de una version anterior." << endl;
		archivo.close();
		return false;
	}
	if (datos.cantidadBarcosGuardados <= 0 || datos.cantidadBarcosGuardados > TOTAL_BARCOS_FLOTA ||
	    datos.cantidadHistorialGuardado < 0 || datos.cantidadHistorialGuardado > MAX_TURNOS ||
	    datos.turnosRealizados < 0 || datos.turnosRealizados > MAX_TURNOS ||
	    datos.tipoColocacion < COLOCACION_NINGUNA || datos.tipoColocacion > COLOCACION_ALEATORIA) {
		cout << "ERROR: El archivo \"" << nombreArchivo << "\" contiene datos invalidos." << endl;
		archivo.close();
		return false;
	}
	int cantidadRegistros = datos.cantidadHistorialGuardado;
	RegistroDisparo* registros = new RegistroDisparo[cantidadRegistros];
	reservasMemoria++;
	archivo.read(reinterpret_cast<char*>(registros), cantidadRegistros * sizeof(RegistroDisparo));
	bool historialCompleto = archivo.gcount() ==
	                         static_cast<streamsize>(cantidadRegistros * sizeof(RegistroDisparo));
	archivo.close();
	if (!historialCompleto) {
		delete[] registros;
		liberacionesMemoria++;
		cout << "ERROR: El historial guardado en \"" << nombreArchivo << "\" esta incompleto." << endl;
		return false;
	}
	datos.nombreJugador[TAM_NOMBRE - 1] = '\0';
	strcpy(nombreJugador, datos.nombreJugador);
	inicializarTablero();
	inicializarBarcosColocados();
	cantidadBarcosColocados = datos.cantidadBarcosGuardados;
	BarcoBinario* leido = datos.barcos;
	Barco* destino = barcosColocados;
	for (int i = 0; i < cantidadBarcosColocados; i++, leido++, destino++) {
		strcpy(destino->codigo, leido->codigo);
		strcpy(destino->tipo, leido->tipo);
		destino->tamanio = leido->tamanio;
		destino->vidaActual = leido->vidaActual;
		destino->estaHundido = leido->estaHundido;
		destino->posXInicio = leido->posXInicio;
		destino->posYInicio = leido->posYInicio;
		destino->esHorizontal = leido->esHorizontal;
		int indiceTipo = buscarTipoPorCodigo(string(destino->codigo));
		if (indiceTipo != -1) {
			barcoYaColocado[indiceTipo] = true;
		}
	}
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			datos.gridBarcos[fila][columna][2] = '\0';
			gridBarcos[fila][columna] = codigoAnchoDos(datos.gridBarcos[fila][columna]);
			string valor = "";
			valor += datos.gridDisparos[fila][columna];
			valor += " ";
			gridDisparos[fila][columna] = valor;
		}
	}
	disparosRealizados = datos.turnosRealizados;
	aciertosTotales = datos.aciertosGuardados;
	fallosTotales = datos.fallosGuardados;
	barcosHundidos = datos.barcosHundidosGuardados;
	tipoColocacion = datos.tipoColocacion;
	faseColocacionCompleta = true;
	cantidadBarcosSeleccionadosPtr = 0;
	liberarHistorial();
	RegistroDisparo* registro = registros;
	for (int i = 0; i < cantidadRegistros; i++, registro++) {
		registro->codigoBarco[2] = '\0';
		registrarDisparoEnHistorial(registro->turno, registro->x, registro->y,
		                            registro->resultado, registro->codigoBarco);
	}
	ofstream disparos(ARCHIVO_DISPAROS, ios::binary | ios::trunc);
	if (disparos.is_open()) {
		disparos.write(reinterpret_cast<char*>(registros), cantidadRegistros * sizeof(RegistroDisparo));
		disparos.close();
	} else {
		cout << "ADVERTENCIA: No se pudo reconstruir \"" << ARCHIVO_DISPAROS << "\"." << endl;
	}
	delete[] registros;
	liberacionesMemoria++;
	cout << "Partida cargada correctamente desde \"" << nombreArchivo << "\"." << endl;
	cout << "Colocacion: " << nombreTipoColocacion(tipoColocacion)
	     << " | Turnos: " << disparosRealizados
	     << " | Disparos en el historial: " << cantidadHistorial << endl;
	return true;
}

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
void consultarDisparoPorTurno() {
	cout << "\n=== CONSULTAR_DISPARO: consulta de un turno en \"" << ARCHIVO_DISPAROS << "\" ===" << endl;
	ifstream archivo(ARCHIVO_DISPAROS, ios::binary);
	if (!archivo.is_open()) {
		cout << "ERROR: No existe el archivo \"" << ARCHIVO_DISPAROS
		     << "\" todavia. Juegue al menos un turno antes de consultar." << endl;
		return;
	}
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
	string resultadoTexto = textoResultado(registro.resultado);
	cout << "\n--- Turno " << registro.turno << " ---" << endl;
	cout << "Coordenadas    : (" << registro.x << ", " << registro.y << ")" << endl;
	cout << "Resultado      : " << resultadoTexto << endl;
	if (registro.resultado != 'A') {
		cout << "Barco impactado: " << registro.codigoBarco << endl;
	}
}
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
void marcarBarcoHundido(int indiceBarco) {
	Barco& barco = barcosColocados[indiceBarco];
	int filaInicio = barco.posYInicio - 1;
	int colInicio = barco.posXInicio - 1;

	for (int i = 0; i < barco.tamanio; i++) {
		if (barco.esHorizontal) {
			gridDisparos[filaInicio][colInicio + i] = "H ";
		} else {
			gridDisparos[filaInicio + i][colInicio] = "H ";
		}
	}
}
void registrarDisparo(int indiceBarco, int posX, int posY) {
	Barco* barco = &barcosColocados[indiceBarco];
	if (barco->vidaActual > 0) {
		barco->vidaActual--;
	}
	int fila = posY - 1;
	int col = posX - 1;
	gridDisparos[fila][col] = "X ";
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
bool procesarDisparo(int posX, int posY, char &resultadoDisparo, char codigoBarcoImpactado[3]) {
	if (posX < COORD_MIN || posX > COORD_MAX || posY < COORD_MIN || posY > COORD_MAX) {
		cout << "ERROR: Coordenadas fuera de rango (1-10)." << endl;
		return false;
	}
	int fila = posY - 1;
	int col = posX - 1;
	string estadoCasilla = gridDisparos[fila][col];
	if (estadoCasilla == "~ ") {
		gridDisparos[fila][col] = "O ";
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
void mostrarReporte() {
	cout << "\n=== REPORTE DE PARTIDA ===" << endl;
	cout << "Jugador: " << nombreJugador << endl;
	cout << "Tipo de colocacion: " << nombreTipoColocacion(tipoColocacion) << endl;
	cout << "Turnos maximos: " << MAX_TURNOS << endl;
	cout << "Disparos realizados: " << disparosRealizados << endl;
	cout << "Aciertos: " << aciertosTotales << endl;
	cout << "Agua: " << fallosTotales << endl;
	cout << "Barcos hundidos: " << barcosHundidos << "/" << cantidadBarcosColocados << endl;
	cout << "Precision: " << fixed << setprecision(1) << calcularPrecision() << "%" << endl;
	cout.unsetf(ios::fixed);
	cout << setprecision(6);
	if (cantidadBarcosColocados > 0 && todosBarcosHundidos()) {
		cout << "\n*** VICTORIA! Hundiste todos los barcos! ***" << endl;
	} else if (disparosRealizados >= MAX_TURNOS) {
		cout << "\n*** Se agotaron los turnos disponibles. ***" << endl;
	}
}
void mostrarComparacionMemoria() {
	int celdas = TAM_TABLERO * TAM_TABLERO;
	int bytesGridEstatico = static_cast<int>(celdas * sizeof(string));
	int bytesGridDinamico = static_cast<int>(sizeof(string**) + TAM_TABLERO * sizeof(string*)
	                                         + celdas * sizeof(string));
	int bytesHistorialEstatico = static_cast<int>(MAX_TURNOS * sizeof(Disparo));
	int bytesHistorialDinamico = static_cast<int>(cantidadHistorial * sizeof(NodoDisparo));
	cout << "\n=== MEMORIA ESTATICA vs DINAMICA APLICADA AL TABLERO ===" << endl;
	cout << "Estatica (string grid[10][10]): su tamano se fija al compilar, existe durante" << endl;
	cout << "  todo el programa y no se puede liberar ni cambiar de tamano." << endl;
	cout << "Dinamica (string** grid = new ...): se crea en ejecucion con new, se usa a" << endl;
	cout << "  traves de apuntadores y se devuelve al sistema con delete[]." << endl;
	cout << endl;
	cout << left << setw(26) << "Estructura" << setw(16) << "Estatica" << "Dinamica" << endl;
	cout << setw(26) << "Un grid 10x10"
	     << setw(16) << (to_string(bytesGridEstatico) + " bytes")
	     << bytesGridDinamico << " bytes" << endl;
	cout << setw(26) << "Historial de disparos"
	     << setw(16) << (to_string(bytesHistorialEstatico) + " bytes")
	     << bytesHistorialDinamico << " bytes" << endl;
	cout << right;
	cout << endl;
	cout << "El grid dinamico usa " << (bytesGridDinamico - bytesGridEstatico)
	     << " bytes extra en apuntadores a cambio de crearse y liberarse cuando se necesita." << endl;
	cout << "El historial estatico reserva " << MAX_TURNOS << " casillas siempre; la lista enlazada usa "
	     << cantidadHistorial << " nodos." << endl;
	cout << "Bloques reservados con new: " << reservasMemoria
	     << " | liberados con delete: " << liberacionesMemoria
	     << " | en uso: " << (reservasMemoria - liberacionesMemoria) << endl;
}
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
				int turnosRestantes = 0;
				actualizarTurnoPtr(&disparosRealizados, &turnosRestantes);
				guardarDisparoBinario(disparosRealizados, posX, posY, resultado, codigoBarcoImpactado);
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
			cout << "(OPCION DE DESARROLLADOR: esto revela las posiciones)" << endl;
			mostrarTableroJugador();
		} else if (strcmp(comandoMayus, "REPORTE") == 0) {
			mostrarReporte();
		} else if (strcmp(comandoMayus, "CONSULTAR_DISPARO") == 0) {
			consultarDisparoPorTurno();
		} else if (strcmp(comandoMayus, "FLOTA_PTR") == 0) {
			mostrarEstadoCompletoPtr();
		} else if (strcmp(comandoMayus, "SELECCIONAR") == 0) {
			if (cantidadBarcosColocados == 0) {
				cout << "ERROR: Todavia no hay barcos colocados." << endl;
			} else {
				cout << "Ingrese el codigo del barco a seleccionar (ej. P, S, A1, D2, F1): ";
				string codigoIngresado;
				getline(cin, codigoIngresado);
				codigoIngresado = quitarEspacios(codigoIngresado);
				if (codigoIngresado.empty()) {
					cout << "ERROR: Debe ingresar un codigo." << endl;
				} else {
					aMayusculas(&codigoIngresado[0]);
					seleccionarBarcoPtr(codigoIngresado.c_str());
				}
			}
		} else if (strcmp(comandoMayus, "MEMORIA") == 0) {
			mostrarComparacionMemoria();
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
			     << "FLOTA_PTR, SELECCIONAR, MEMORIA, SALIR." << endl;
			cout << "(FLOTA_DEV existe, pero revela la flota: es solo para depurar.)" << endl;
		}
	}
}
int main() {
	srand(time(0));
	gridBarcos = crearGrid(TAM_TABLERO, TAM_TABLERO);
	gridDisparos = crearGrid(TAM_TABLERO, TAM_TABLERO);
	atexit(liberarMemoria);
	registrarJugador();
	inicializarTablero();
	inicializarBarcosColocados();
	int opcion;
	do {
		cout << "\n=== BATALLA NAVAL - MENU PRINCIPAL ===" << endl;
		cout << "1. Jugar con flota manual (colocar o cargar y disparar)" << endl;
		cout << "2. Jugar con flota aleatoria" << endl;
		cout << "3. Cargar flota desde \"" << ARCHIVO_FLOTA_ENTRADA << "\"" << endl;
		cout << "4. Ver mi flota" << endl;
		cout << "5. Exportar flota actual a \"" << ARCHIVO_FLOTA_SALIDA << "\"" << endl;
		cout << "6. Generar reporte \"" << ARCHIVO_REPORTE << "\"" << endl;
		cout << "7. Guardar partida (\"" << ARCHIVO_PARTIDA_BINARIA << "\")" << endl;
		cout << "8. Cargar partida (\"" << ARCHIVO_PARTIDA_BINARIA << "\")" << endl;
		cout << "9. Consultar disparo por turno (\"" << ARCHIVO_DISPAROS << "\")" << endl;
		cout << "10. Ver estadisticas con apuntadores" << endl;
		cout << "11. Comparar memoria estatica vs dinamica" << endl;
		cout << "12. Salir" << endl;
		opcion = leerOpcionValida(1, 12, "Seleccione una opcion: ");
		switch (opcion) {
			case 1:
				if (cantidadBarcosColocados == 0) {
					prepararFlotaAntesDeJugar();
				} else {
					cout << "\nYa tiene una flota lista con " << cantidadBarcosColocados
					     << " barco(s)." << endl;
				}
				if (disparosRealizados == 0) {
					reiniciarArchivoDisparos();
				}
				faseDisparos();
				reiniciarPartida();
				break;
			case 2:
				if (cantidadBarcosColocados == 0) {
					generarFlotaAleatoria();
					cout << "\nFlota aleatoria generada con exito (" << cantidadBarcosColocados
					     << "/" << TOTAL_BARCOS_FLOTA << " barcos)." << endl;
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
				mostrarEstadisticasConApuntadores();
				break;
			case 11:
				mostrarComparacionMemoria();
				break;
			case 12:
				liberarMemoria();
				cout << "Memoria liberada: " << liberacionesMemoria << " de "
				     << reservasMemoria << " bloques reservados." << endl;
				cout << "Gracias por jugar, " << nombreJugador << ". Hasta luego!" << endl;
				return 0;
		}
	} while (opcion != 12);
	return 0;
}
