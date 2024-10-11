#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>  // Para usar sleep ( SIMULA EL TIEMPO DE ESPERA PARA EJECUCION)

using namespace std;

#define MAX_PROCESOS 100

struct Proceso {
    vector<int> datos;
    string cadena;
};

// Estructura para pasar parámetros a los hilos
struct ThreadData {
    int id_hilo;
};

// Cola de procesos compartida entre los hilos
queue<Proceso> colaProcesos;

// Mutex para proteger el acceso a la cola de procesos
pthread_mutex_t mutexCola = PTHREAD_MUTEX_INITIALIZER;

// Función que ejecuta cada hilo
void *thread_function(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    int id_hilo = data->id_hilo;

    while (true) {
        pthread_mutex_lock(&mutexCola);

        // Si la cola está vacía, el hilo termina
        if (colaProcesos.empty()) {
            pthread_mutex_unlock(&mutexCola);
            break;
        }

        // Tomar un proceso de la cola
        Proceso proceso = colaProcesos.front();
        colaProcesos.pop();

        pthread_mutex_unlock(&mutexCola);

        // Simulación de ejecución del proceso
        printf("Hilo %d ejecutando proceso: ", id_hilo + 1);
        for (int valor : proceso.datos) {
            printf("%d ", valor);
        }
        printf("| %s\n", proceso.cadena.c_str());

        sleep(2);  // Simular que tarda en ejecutarse

        printf("Hilo %d termino de ejecutar el proceso.\n", id_hilo + 1);
    }

    return NULL;
}

// Función para validar que una línea siga el formato "Palabra Entero"
bool validarFormatoLinea(const string &linea, const string &palabra) {
    if (linea.compare(0, palabra.length(), palabra) != 0) {
        return false;
    }

    if (linea[palabra.length()] != ' ') {
        return false;
    }

    string numero = linea.substr(palabra.length() + 1);
    for (char c : numero) {
        if (!isdigit(c) && c != ' ') {
            return false;
        }
    }

    return true;
}

// Función para validar que una línea siga el formato
bool validarLineaMixta(const string &linea) {
    stringstream ss(linea);
    string token;
    int contador = 0;

    // Contamos los tokens y validamos
    while (getline(ss, token, '|')) {
        if (contador == 2) {  // La tercera posición debe ser una cadena
            if (token.empty()) {
                return false; // La cadena no puede estar vacía
            }
        } else {
            // Validar que las demás posiciones sean números
            for (char c : token) {
                if (!isdigit(c)) {
                    return false;  // Si no es un dígito, retorna false
                }
            }
        }
        contador++;
    }
    // Debe haber exactamente 8 elementos: 2 enteros, 1 cadena y 5 enteros
    return contador == 8;
}

// Función para convertir una línea de valores mezclados en un vector de enteros y una cadena
int convertirLineaAMixta(const string &linea, vector<int> &arreglo, string &cadena) {
    stringstream ss(linea);
    string token;
    int contador = 0;

    while (getline(ss, token, '|')) {
        if (contador == 2) {  // La tercera posición debe ser una cadena
            cadena = token;  // Guardamos la cadena
        } else {
            arreglo.push_back(stoi(token));  // Convertimos a entero
        }
        contador++;
    }
    return contador;
}

// INICIO DEL PROGRAMA
int main() {
    ifstream archivo;
    string nombreArchivo;
    string linea;
    vector<int> procesadores(1);
    vector<int> hilos(1);
    Proceso procesos[MAX_PROCESOS];  // Arreglo para almacenar los procesos
    int numProcesos = 0;

    // Pedimos el nombre del archivo
    cout << "Ingrese el nombre del archivo: ";
    cin >> nombreArchivo;

    // Validamos que el archivo sea .dat
    if (nombreArchivo.find(".dat") == string::npos) {
        cout << "Error: El archivo debe tener extensión .dat" << endl;
        return 1;
    }

    // Abrimos el archivo para leer
    archivo.open(nombreArchivo);
    if (!archivo) {
        cout << "Error al abrir el archivo" << endl;
        return 1;
    }

    // Leemos la primera línea (Procesadores)
    if (getline(archivo, linea)) {
        linea.erase(linea.find_last_not_of("\n") + 1); // Quitamos el salto de línea

        if (!validarFormatoLinea(linea, "Procesadores")) {
            cout << "Error: La primera línea debe ser 'Procesadores=N' con un entero N" << endl;
            archivo.close();
            return 1;
        }
        procesadores[0] = stoi(linea.substr(strlen("Procesadores=")));
    }

    // Leemos la segunda línea (Hilos)
    if (getline(archivo, linea)) {
        linea.erase(linea.find_last_not_of("\n") + 1); // Quitamos el salto de línea

        if (!validarFormatoLinea(linea, "Hilos")) {
            cout << "Error: La segunda línea debe ser 'Hilos=N' con un entero N" << endl;
            archivo.close();
            return 1;
        }
        hilos[0] = stoi(linea.substr(strlen("Hilos=")));
    }

    // Ignoramos líneas vacías antes de leer los procesos
    while (getline(archivo, linea)) {
        linea.erase(linea.find_last_not_of("\n") + 1); // Quitamos el salto de línea
        if (linea.empty()) {
            continue;  // Ignorar línea vacía
        }

        if (!validarLineaMixta(linea)) {
            cout << "Error: Línea de valores incorrecta (deben ser 2 enteros, una cadena y 5 enteros)" << endl;
            archivo.close();
            return 1;
        }

        // Convertimos la línea de valores a un arreglo y cadena
        convertirLineaAMixta(linea, procesos[numProcesos].datos, procesos[numProcesos].cadena);

        // Insertamos el proceso en la cola
        colaProcesos.push(procesos[numProcesos]);

        numProcesos++;
    }

    archivo.close();

    // Imprimimos los resultados
    cout << "Procesadores: " << procesadores[0] << endl;
    cout << "Hilos: " << hilos[0] << endl;
    cout << "Procesos:" << endl;
    for (int i = 0; i < numProcesos; i++) {
        cout << "Proceso " << i + 1 << ": ";
        for (int j = 0; j < procesos[i].datos.size(); j++) {
            cout << procesos[i].datos[j] << " ";  // Imprimir los enteros
        }
        cout << "| " << procesos[i].cadena << endl;  // Imprimir la cadena
    }

    cout << "" << endl;

    // EJECUCION DE LOS HILOS
    pthread_t threads[hilos[0]];
    ThreadData threadData[hilos[0]];

    // Crear los hilos
    for (int i = 0; i < hilos[0]; i++) {
        threadData[i].id_hilo = i;
        pthread_create(&threads[i], NULL, thread_function, &threadData[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < hilos[0]; i++) {
        pthread_join(threads[i], NULL);
    }
    cout<<""<< endl;
    cout<< "Se han ejecutado todos los procesos con exito" << endl;
    return 0;
}
