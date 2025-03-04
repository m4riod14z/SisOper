#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Declaración de variables globales
int variable_global = 42;
static int variable_global_estatica = 100;

// Función para mostrar las direcciones de memoria de diferentes segmentos.
void mostrar_direcciones() {
    // Declaración variables locales
    int variable_local = 10; 
    static int variable_local_estatica = 20; 
    int *heap_var = (int*) malloc(sizeof(int));// Variable asignada dinámicamente en el heap

    printf("\n--- Direcciones de Memoria ---\n");
    printf("Variable global: %p\n", (void*)&variable_global);
    printf("Variable estática global: %p\n", (void*)&variable_global_estatica);
    printf("Variable local: %p\n", (void*)&variable_local);
    printf("Variable estática local: %p\n", (void*)&variable_local_estatica);
    printf("Variable asignada en heap: %p\n", (void*)heap_var);

    free(heap_var); // Liberar memoria asignada en heap
}

// Función para medir el consumo de memoria usando /proc/self/status
void mostrar_consumo_memoria() {
    char comando[50];
    snprintf(comando, sizeof(comando), "cat /proc/%d/status | grep -E 'VmSize|VmRSS'", getpid());
    printf("\n--- Estadisticas del consumo de memoria ---\n");
    system(comando);
}

// Función para asignar y liberar memoria dinámica, y medir el consumo antes, durante y después
void asignar_memoria_dinamica(size_t cant) {
    printf("\n Consumo de memoria ANTES de asignar memoria:\n");
    mostrar_consumo_memoria();

    // Reservar memoria en heap
    int *arr = (int*) malloc(cant * sizeof(int));
    if (arr == NULL) {
        printf("Error al asignar memoria.\n");
        return;
    }

    // Llenar arreglo con datos
    for (size_t i = 0; i < cant; i++) {
        arr[i] = i;
    }

    printf("\n Consumo de memoria DESPUÉS de asignar memoria:\n");
    mostrar_consumo_memoria();

    // Liberar memoria
    printf("\nPresiona ENTER para liberar la memoria...");
    getchar();
    
    free(arr);
    printf("\n Memoria liberada.\n");

    printf("\n Consumo de memoria DESPUÉS de liberar la memoria:\n");
    mostrar_consumo_memoria();
}

// Menú interactivo
void menu() {
    int opcion;
    size_t cant;

    do {
        printf("\n--- Menú de gestión de memoria ---\n");
        printf("1. Imprimir direcciones de memoria\n");
        printf("2. Mostrar consumo de memoria\n");
        printf("3. Asignar y liberar memoria dinámica\n");
        printf("4. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);
        getchar(); // Limpiar buffer de entrada

        switch (opcion) {
            case 1:
                mostrar_direcciones();
                break;
            case 2:
                mostrar_consumo_memoria();
                break;
            case 3:
                printf("Ingrese la cantidad de datos a asignar en memoria: ");
                scanf("%zu", &cant);
                getchar(); // Limpiar buffer de entrada
                asignar_memoria_dinamica(cant);
                break;
            case 4:
                printf("Saliendo...\n");
                break;
            default:
                printf("La opción ingresada no es válida.\n");
        }
    } while (opcion != 4);
}

// Función principal
int main() {
    menu();
    return 0;
}