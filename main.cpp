#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>  // Para usar sleep (SIMULA EL TIEMPO DE ESPERA PARA EJECUCION)

using namespace std;

#define MAX_PROCESOS 100

struct Proceso {
    vector<int> datos;
    string cadena;
    int iteraciones_restantes;  // Nuevas iteraciones restantes (según el último valor del vector datos)
};

// Estructura para pasar parámetros a los hilos
struct ThreadData {
    int id_hilo;
};

// Cola de procesos compartida entre los hilos
queue<Proceso> cola_procesos;

// Mutex para proteger el acceso a la cola de procesos
pthread_mutex_t mutex_cola = PTHREAD_MUTEX_INITIALIZER;

// Valor del quantum ya declarado
int quantum = 6;

// Función que ejecuta cada hilo
void *thread_function(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    int id_hilo = data->id_hilo;

    while (true) {
        pthread_mutex_lock(&mutex_cola);

        // Si la cola está vacía, el hilo termina
        if (cola_procesos.empty()) {
            pthread_mutex_unlock(&mutex_cola);
            break;
        }

        // Tomar un proceso de la cola
        Proceso proceso = cola_procesos.front();
        cola_procesos.pop();

        pthread_mutex_unlock(&mutex_cola);

        // El número de iteraciones que tiene que ejecutar el proceso, limitado por el quantum
        int iteraciones_a_ejecutar = min(quantum, proceso.iteraciones_restantes);

        // Simulación de ejecución del proceso 'iteraciones_a_ejecutar' veces
        for (int i = 0; i < iteraciones_a_ejecutar; i++) {
            printf("Hilo %d ejecutando proceso (Iteracion %d/%d): ", id_hilo + 1, i + 1, iteraciones_a_ejecutar);
            for (int valor : proceso.datos) {
                printf("%d ", valor);
            }
            printf("| %s\n", proceso.cadena.c_str());

            sleep(1);  // Simular que tarda en ejecutarse
        }

        // Actualizar las iteraciones restantes
        proceso.iteraciones_restantes -= iteraciones_a_ejecutar;

        // Si el proceso no ha terminado, volver a insertarlo en la cola
        if (proceso.iteraciones_restantes > 0) {
            pthread_mutex_lock(&mutex_cola);
            cola_procesos.push(proceso);
            pthread_mutex_unlock(&mutex_cola);
        } else {
            printf("Hilo %d completo el proceso.\n", id_hilo + 1);
        }
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
bool analizar_linea(const string &linea) {
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
    string nombre_archivo;
    string linea;
    vector<int> procesadores(1);
    vector<int> hilos(1);
    Proceso procesos[MAX_PROCESOS];  // Arreglo para almacenar los procesos
    int num_procesos = 0;

    // Pedimos el nombre del archivo
    cout << "Ingrese el nombre del archivo: ";
    cin >> nombre_archivo;

    // Validamos que el archivo sea .dat
    if (nombre_archivo.find(".dat") == string::npos) {
        cout << "Error: El archivo debe tener extension .dat" << endl;
        return 1;
    }

    // Abrimos el archivo para leer
    archivo.open(nombre_archivo);
    if (!archivo) {
        cout << "Error al abrir el archivo" << endl;
        return 1;
    }

    // Leemos la primera línea (Procesadores)
    if (getline(archivo, linea)) {
        linea.erase(linea.find_last_not_of("\n") + 1); // Quitamos el salto de línea

        if (!validarFormatoLinea(linea, "Procesadores")) {
            cout << "Error: La primera linea debe ser 'Procesadores=N' con un entero N" << endl;
            archivo.close();
            return 1;
        }
        procesadores[0] = stoi(linea.substr(strlen("Procesadores=")));
    }

    // Leemos la segunda línea (Hilos)
    if (getline(archivo, linea)) {
        linea.erase(linea.find_last_not_of("\n") + 1); // Quitamos el salto de línea

        if (!validarFormatoLinea(linea, "Hilos")) {
            cout << "Error: La segunda linea debe ser 'Hilos=N' con un entero N" << endl;
            archivo.close();
            return 1;
        }
        hilos[0] = stoi(linea.substr(strlen("Hilos=")));
    }

    // Ignoramos la lectura de quantum desde el archivo, porque ya está declarado con valor 6

    // Ignoramos líneas vacías antes de leer los procesos
    while (getline(archivo, linea)) {
        linea.erase(linea.find_last_not_of("\n") + 1); // Quitamos el salto de línea
        if (linea.empty()) {
            continue;  // Ignorar línea vacía
        }

        if (!analizar_linea(linea)) {
            cout << "Error: Linea de valores incorrecta (deben ser 2 enteros, una cadena y 5 enteros)" << endl;
            archivo.close();
            return 1;
        }

        // Convertimos la línea de valores a un arreglo y cadena
        convertirLineaAMixta(linea, procesos[num_procesos].datos, procesos[num_procesos].cadena);

        // Asignamos el número de iteraciones restantes al proceso usando el último entero del arreglo
        procesos[num_procesos].iteraciones_restantes = procesos[num_procesos].datos.back();

        // Insertamos el proceso en la cola
        cola_procesos.push(procesos[num_procesos]);

        num_procesos++;
    }

    archivo.close();

    // Calcular el número total de hilos
    int total_hilos = procesadores[0] * hilos[0];
    pthread_t threads[total_hilos];
    ThreadData thread_data[total_hilos];

    // Crear los hilos
    for (int i = 0; i < total_hilos; i++) {
        thread_data[i].id_hilo = i;
        pthread_create(&threads[i], NULL, thread_function, (void *) &thread_data[i]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < total_hilos; i++) {
        pthread_join(threads[i], NULL);
    }

    cout << "Todos los procesos han sido completados." << endl;

    return 0;
}
