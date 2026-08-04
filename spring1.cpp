// Código elaborado por David Coy Vélez, Miguel Ángel Martinez y Andrés Santiago Sabogal Meza

/*
* ============================================================
*  BATALLA NAVAL - SPRINT 1
* ============================================================
*/

#include <iostream>
using namespace std;

// ------------------------------------------------------------
// CONSTANTES GLOBALES
// ------------------------------------------------------------
const int TAM_TABLERO = 10;
const int COORD_MIN = 1;
const int COORD_MAX = 10;
bool tableroInicializado = false;

// Tablero de strings: cada celda puede tener "~" (agua)
// o un codigo de barco de 1 o 2 caracteres ("P", "A1", "D2", etc.)
string tablero[TAM_TABLERO][TAM_TABLERO];

// ------------------------------------------------------------
// PROTOTIPOS
// Necesarios porque main() y algunas funciones llaman a otras
// que estan definidas mas abajo en el archivo.
// ------------------------------------------------------------
void inicializarTablero();
void mostrarTablero();
void mostrarInfoTipos();
bool validarCoordenada(int x, int y);
void leerCoordenada();

/*
* inicializarTablero()
*/
void inicializarTablero() {
	tableroInicializado = true;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			tablero[fila][columna] = "~";
		}
	}
}

/*
* mostrarTablero()
*/
void mostrarTablero() {
	if (tableroInicializado) {
		cout << "\n    ";
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			cout << (columna + 1) << "  ";
		}
		cout << endl;
		
		for (int fila = 0; fila < TAM_TABLERO; fila++) {
			if (fila + 1 < 10) cout << " " << (fila + 1) << " ";
			else cout << (fila + 1) << " ";
			
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
* mostrarInfoTipos()
*/
void mostrarInfoTipos() {
	int numeroEntero = 10;
	char letra = 'N';
	bool valorBooleano = true;
	float numeroFlotante = 3.14f;
	double numeroDoble = 3.14159265;
	signed int numeroConSigno = -25;
	
	cout << "\n--- Tamanos en memoria (sizeof) ---" << endl;
	cout << "int: " << sizeof(numeroEntero) << " bytes" << endl;
	cout << "char: " << sizeof(letra) << " byte" << endl;
	cout << "bool: " << sizeof(valorBooleano) << " byte" << endl;
	cout << "float: " << sizeof(numeroFlotante) << " bytes" << endl;
	cout << "double: " << sizeof(numeroDoble) << " bytes" << endl;
	cout << "signed int: " << sizeof(numeroConSigno) << " bytes"
		<< " (valor de ejemplo: " << numeroConSigno << ")" << endl;
	
	cout << "\n--- Conversiones entre variables ---" << endl;
	double resultadoImplicito = numeroEntero;
	int resultadoExplicito = (int)numeroFlotante;
	
	cout << "Conversion implicita (int a double): " << resultadoImplicito << endl;
	cout << "Conversion explicita (float a int, se quita): " << resultadoExplicito << endl;
}

/*
* validarCoordenada()
* Retorna true si x,y estan en el rango visible 1..10
*/
bool validarCoordenada(int x, int y) {
	if (y < COORD_MIN || y > COORD_MAX || x < COORD_MIN || x > COORD_MAX) {
		cout << "Valores fuera del rango (deben estar entre 1 y 10)." << endl;
		return false;
	}
	return true;
}

/*
* leerCoordenada()
* Pide x,y por teclado, valida, convierte a indices 0..9
* y coloca el codigo de barco si la coordenada es valida
* y la casilla esta libre. Si es invalida, NO toca el tablero.
*/
void leerCoordenada() {
	int x = 0, y = 0;
	string codigo;
	
	cout << "Ingrese la posicion x (1-10): ";
	cin >> x;
	cout << "Ingrese la posicion y (1-10): ";
	cin >> y;
	
	if (validarCoordenada(x, y)) {
		int fila = y - 1;
		int col  = x - 1;
		
		cout << "Ingrese el codigo del barco: ";
		cin >> codigo;
		
		if (tablero[fila][col] == "~") {
			tablero[fila][col] = codigo;
			cout << "Barco colocado correctamente." << endl;
		} else {
			cout << "Esa casilla ya esta ocupada." << endl;
		}
	}
	// si la coordenada es invalida, validarCoordenada ya imprimio
	// el mensaje de error y el tablero no se modifica
}

/*
* ============================================================
* MAIN
* ============================================================
*/
int main() {
	int opcion;
	
	do {
		cout << "\n=== BATALLA NAVAL - MENU ===" << endl;
		cout << "1. Iniciar juego" << endl;
		cout << "2. Mostrar tablero" << endl;
		cout << "3. Ingresar coordenada" << endl;
		cout << "4. Ver info de tipos de datos" << endl;
		cout << "5. Salir" << endl;
		cout << "Seleccione una opcion: ";
		cin >> opcion;
		
		switch (opcion) {
		case 1:
			inicializarTablero();
			cout << "Tablero inicializado." << endl;
			break;
		case 2:
			mostrarTablero();
			break;
		case 3:
			if (tableroInicializado) {
				leerCoordenada();
			} else {
				cout << "Debe iniciar el juego primero (opcion 1)." << endl;
			}
			break;
		case 4:
			mostrarInfoTipos();
			break;
		case 5:
			cout << "Saliendo del programa..." << endl;
			break;
		default:
			cout << "Opcion invalida, intente de nuevo." << endl;
		}
	} while (opcion != 5);
	
	return 0;
}
