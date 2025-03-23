#include <stdio.h>
#include <stdlib.h>

int main() {
    int *array;
    int tamanio_inicial = 5;
    int nuevo_tamanio, i, opcion;
    int memoria_liberada = 0; // Para validar si la memoria fue liberada

    // 1. Asignar memoria inicial con malloc
    array = (int *)malloc(tamanio_inicial * sizeof(int));

    if (array == NULL) {
        printf("Error al asignar memoria\n");
        return 1;
    }

    // Inicializar el array con valores
    for (i = 0; i < tamanio_inicial; i++) {
        array[i] = i;
    }

    // Mostrar el contenido inicial del array
    printf("Contenido inicial del array:\n");
    for (i = 0; i < tamanio_inicial; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    // Menú de opciones
    do {
        printf("\n--- MENÚ ---\n");
        printf("1. Continuar con el tamaño inicial\n");
        printf("2. Redimensionar el array\n");
        printf("3. Liberar espacio de la memoria\n");
        printf("4. Validar el comportamiento de memoria\n");
        printf("5. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1:
                printf("El array mantiene su tamaño inicial de %d elementos.\n", tamanio_inicial);
                break;

            case 2:
                printf("Ingresa el nuevo tamaño del array: ");
                scanf("%d", &nuevo_tamanio);

                array = (int *)realloc(array, nuevo_tamanio * sizeof(int));

                if (array == NULL) {
                    printf("Error al redimensionar memoria\n");
                    return 1;
                }

                // Inicializar los nuevos elementos con -1
                for (i = tamanio_inicial; i < nuevo_tamanio; i++) {
                    array[i] = -1;
                }

                printf("Contenido del array redimensionado:\n");
                for (i = 0; i < nuevo_tamanio; i++) {
                    printf("%d ", array[i]);
                }
                printf("\n");
                break;

            case 3:
                free(array);
                memoria_liberada = 1;
                printf("Memoria liberada correctamente.\n");
                break;

            case 4:
                if (memoria_liberada) {
                    printf("Memoria ya ha sido liberada, acceso al array no es seguro.\n");
                } else {
                    printf("Memoria aún en uso, el array sigue disponible.\n");
                }
                break;

            case 5:
                printf("Saliendo del programa.\n");
                break;

            default:
                printf("Opción no válida, intenta de nuevo.\n");
        }
    } while (opcion != 5);

    return 0;
}
