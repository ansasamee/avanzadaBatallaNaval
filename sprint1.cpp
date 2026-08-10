// Código elaborado por David Coy Vélez, Miguel Ángel Martinez y Andrés Santiago Sabogal Meza

/*
* ============================================================
*  BATALLA NAVAL - SPRINT 1
* ============================================================
*  Juego de consola donde el usuario coloca "barcos" (por ahora
*  representados con una "X") en un tablero de 10x10 indicando
*  coordenadas (x, y) entre 1 y 10.
* ============================================================
*/

#include <iostream>
using namespace std;

// ------------------------------------------------------------
// CONSTANTES GLOBALES
// ------------------------------------------------------------
const int TAM_TABLERO = 10;   // Tamaño del tablero (10x10)
const int COORD_MIN = 1;      // Coordenada mínima visible para el usuario
const int COORD_MAX = 10;     // Coordenada máxima visible para el usuario

// Bandera que indica si el tablero ya fue inicializado con "~" (agua).
// Se usa para evitar mostrar/usar un tablero "vacío" (con basura de memoria).
bool tableroInicializado = false;

// Tablero de strings: cada celda puede tener "~" (agua)
// o un código de barco de 1 o 2 caracteres ("P", "A1", "D2", etc.)
// Nota: se declara de forma global para que todas las funciones
// puedan acceder y modificar el mismo tablero sin pasarlo por parámetro.
string tablero[TAM_TABLERO][TAM_TABLERO];

// ------------------------------------------------------------
// PROTOTIPOS
// Necesarios porque main() y algunas funciones llaman a otras
// que están definidas más abajo en el archivo.
// ------------------------------------------------------------
void inicializarTablero();
void mostrarTablero();
void mostrarInfoTipos();
bool validarCoordenada(int x, int y);
void leerCoordenada();
void menutablero();
void limpiarBufferEntrada();   // NUEVA: función auxiliar para limpiar cin
int leerOpcionValida(int min, int max, const string& mensaje); // NUEVA: lee un entero validado

// ------------------------------------------------------------
// limpiarBufferEntrada()
// Función auxiliar que centraliza la limpieza del buffer de cin.
// Antes esto se repetía varias veces en el código (cin.clear() +
// cin.ignore()); ahora se llama una sola función para evitar
// duplicación y errores de copiar/pegar.
// ------------------------------------------------------------
void limpiarBufferEntrada() {
	cin.clear();              // Quita las banderas de error de cin
	cin.ignore(1000, '\n');   // Descarta hasta 1000 caracteres del buffer o hasta el salto de línea,
	                          // lo que ocurra primero (mismo enfoque que ya usaba el código original).
}

// ------------------------------------------------------------
// leerOpcionValida()
// MEJORA DE RESTRICCIÓN:
// Antes, si el usuario ingresaba una letra en vez de un número,
// cin quedaba en estado de error y el programa podía entrar en
// un ciclo infinito imprimiendo "opcion invalida" sin dejar
// escribir nada nuevo. Esta función:
//   1. Verifica que la entrada realmente sea un número (cin.fail()).
//   2. Verifica que el número esté dentro del rango permitido.
//   3. Repite hasta obtener una entrada válida.
// ------------------------------------------------------------
int leerOpcionValida(int min, int max, const string& mensaje) {
	int opcion;
	bool entradaValida = false;

	do {
		cout << mensaje;
		cin >> opcion;

		if (cin.fail()) {
			// El usuario ingresó algo que no es un número (ej: una letra)
			cout << "Entrada invalida. Debe ingresar un numero." << endl;
			limpiarBufferEntrada();
		} else if (opcion < min || opcion > max) {
			// El número es válido pero está fuera de rango
			cout << "Opcion fuera de rango (" << min << "-" << max << ")." << endl;
			limpiarBufferEntrada();
		} else {
			// Entrada correcta: número y dentro del rango
			limpiarBufferEntrada();
			entradaValida = true;
		}
	} while (!entradaValida);

	return opcion;
}

// ------------------------------------------------------------
// menutablero()
// Muestra el submenú del juego una vez que el tablero ya fue
// inicializado. Permite ingresar coordenadas, ver el tablero
// o finalizar el juego.
// ------------------------------------------------------------
void menutablero() {
	bool seguirJugando = true;

	while (seguirJugando) {
		cout << "\n=== BATALLA NAVAL - JUEGO ===" << endl;
		cout << "1. Ingresar coordenada" << endl;
		cout << "2. Mostrar tablero" << endl;
		cout << "3. Finalizar juego" << endl;

		// Se usa la función validada en vez de leer "opc" directamente,
		// así se evita el problema de entradas no numéricas.
		int opc = leerOpcionValida(1, 3, "Seleccione una opcion: ");

		switch (opc) {
			case 1:
				leerCoordenada();
				break;
			case 2:
				mostrarTablero();
				break;
			case 3:
				seguirJugando = false;
				cout << "Juego finalizado" << endl;
				break;
			// No hace falta un "default" porque leerOpcionValida ya
			// garantiza que opc está entre 1 y 3.
		}
	}
}

/*
* inicializarTablero()
* Llena todas las celdas del tablero con "~", que representa agua.
* Se marca tableroInicializado en true para que el resto del
* programa sepa que ya es seguro mostrar/usar el tablero.
*/
void inicializarTablero() {
	tableroInicializado = true;
	for (int fila = 0; fila < TAM_TABLERO; fila++) {
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			tablero[fila][columna] = "~ ";
		}
	}
}

/*
* mostrarTablero()
* Imprime el tablero en consola con números de columna (1-10)
* en la parte superior y números de fila (1-10) a la izquierda,
* para que el usuario pueda ubicar coordenadas fácilmente.
*/
void mostrarTablero() {
	if (tableroInicializado) {
		// Encabezado con los números de columna
		cout << "\n    ";
		for (int columna = 0; columna < TAM_TABLERO; columna++) {
			cout << "C" << (columna + 1) << "  ";
		}
		cout << endl;

		// Filas del tablero, cada una precedida por su número
		for (int fila = 0; fila < TAM_TABLERO; fila++) {
			// Alineación: los números de una sola cifra (1-9) llevan
			// un espacio extra para que la tabla quede alineada con
			// los de doble cifra (10).
			if (fila + 1 < 10) cout << " " << "F" << (fila + 1) << " ";
			else cout << "F" << (fila + 1) << " ";

			for (int columna = 0; columna < TAM_TABLERO; columna++) {
				cout << " " << tablero[fila][columna] << " ";
			}
			cout << endl;
		}
	} else {
		// Protección: evita mostrar un tablero que aún no tiene datos válidos
		cout << "El tablero no se ha inicializado." << endl;
	}
}
/*
* validarCoordenada()
* Retorna true si x,y están en el rango visible 1..10.
* Si están fuera de rango, imprime un mensaje de error y
* retorna false SIN modificar el tablero.
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
* Pide x,y por teclado, valida que sean números y que estén en
* rango, convierte a índices 0..9 y coloca una marca ("X") en el
* tablero si la coordenada es válida y la casilla está libre.
* Si la coordenada es inválida, NO se toca el tablero.
*
* MEJORA DE RESTRICCIÓN:
* Antes no se validaba si el usuario ingresaba letras en x o y,
* lo cual dejaba a cin en estado de error. Ahora se verifica con
* cin.fail() antes de intentar usar los valores.
*/
void leerCoordenada() {
	int x = 0, y = 0;

	cout << "Ingrese la posicion x (1-10): ";
	cin >> x;
	if (cin.fail()) {
		cout << "Entrada invalida: x debe ser un numero." << endl;
		limpiarBufferEntrada();
		return; // Se sale de la función sin tocar el tablero
	}

	cout << "Ingrese la posicion y (1-10): ";
	cin >> y;
	if (cin.fail()) {
		cout << "Entrada invalida: y debe ser un numero." << endl;
		limpiarBufferEntrada();
		return;
	}

	if (validarCoordenada(x, y)) {
		// Se convierten las coordenadas "humanas" (1-10) a índices
		// de arreglo en C++ (0-9).
		int fila = y - 1;
		int col  = x - 1;

		if (tablero[fila][col] == "~ ") {
			tablero[fila][col] = "X ";
			cout << "Barco colocado correctamente." << endl;
		} else {
			cout << "Esa casilla ya esta ocupada." << endl;
		}
	}
	// Si la coordenada es inválida, validarCoordenada() ya imprimió
	// el mensaje de error y el tablero no se modifica.
}

/*
* ============================================================
* MAIN
* ============================================================
* Muestra el menú principal del programa y repite hasta que el
* usuario elija la opción de salir (3).
*/
int main() {
	int opcion;

	do {
		cout << "\n=== BATALLA NAVAL - MENU ===" << endl;
		cout << "1. Iniciar juego" << endl;
		cout << "3. Finalizar programa" << endl;

		// MEJORA DE RESTRICCIÓN:
		// Se reemplazó la lectura directa de "opcion" (cin >> opcion)
		// por leerOpcionValida(), que ya maneja tanto el caso de
		// letras/textos ingresados por error como el de números
		// fuera de rango, evitando el ciclo infinito que se podía
		// producir si cin quedaba en estado de fallo.
		opcion = leerOpcionValida(1, 2, "Seleccione una opcion: ");

		switch (opcion) {
			case 1:
				inicializarTablero();
				menutablero();
				break;
			case 3:
				cout << "Saliendo del programa..." << endl;
				return 0;
		}

	} while (opcion < 1 || opcion > 3);
	// Nota: en la práctica esta condición del while ya nunca se
	// cumple, porque leerOpcionValida() garantiza que "opcion"
	// siempre está entre 1 y 3. Se deja como salvaguarda adicional.

	return 0;
}