#include <stdio.h>
#include <stdlib.h>

void mostrar_menu(int memoria_liberada) {
    printf("\n--- MENÚ ---\n");
    printf("1. Redimensionar el array\n");
    printf("2. Liberar espacio de la memoria\n");
    printf("3. Validar el comportamiento de memoria\n");
    printf("4. Salir\n");

    printf("Seleccione una opción: ");
}

int main() {
    int *array = NULL;
    int tamanio_inicial = 5, nuevo_tamanio, i;
    int memoria_liberada = 0;  // Para verificar si la memoria ha sido liberada
    int opcion;

    array = (int *)malloc(tamanio_inicial * sizeof(int));
    if (array == NULL) {
        printf("Error al asignar memoria\n");
        return 1;
    }

    for (i = 0; i < tamanio_inicial; i++) {
        array[i] = i;
    }

    printf("Contenido inicial del array:\n");
    for (i = 0; i < tamanio_inicial; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    while (1) {
        mostrar_menu(memoria_liberada);
        scanf("%d", &opcion);

        switch (opcion) {
            case 1:
                if (memoria_liberada) {
                    printf("No puedes redimensionar, la memoria ya fue liberada.\n");
                    break;
                }

                printf("Ingresa el nuevo tamaño del array: ");
                scanf("%d", &nuevo_tamanio);

                if (nuevo_tamanio == 0) { 
                    // Si el usuario ingresa 0, liberar la memoria y marcar como liberada
                    free(array);
                    array = NULL;
                    memoria_liberada = 1;
                    printf("Memoria liberada debido a tamaño 0.\n");
                    break;
                }

                int *temp = (int *)realloc(array, nuevo_tamanio * sizeof(int));
                if (temp == NULL) {
                    printf("Error al redimensionar memoria\n");
                    break;
                }
                array = temp;

                if (nuevo_tamanio > tamanio_inicial) {
                    for (i = tamanio_inicial; i < nuevo_tamanio; i++) {
                        array[i] = -1;
                    }
                }

                tamanio_inicial = nuevo_tamanio;

                printf("Contenido del array redimensionado:\n");
                for (i = 0; i < nuevo_tamanio; i++) {
                    printf("%d ", array[i]);
                }
                printf("\n");
                break;

            case 2:
                if (memoria_liberada) {
                    printf("La memoria ya ha sido liberada.\n");
                    break;
                }

                free(array);
                array = NULL;
                memoria_liberada = 1;
                printf("Memoria liberada correctamente.\n");
                break;

            case 3:
                if (memoria_liberada) {
                    printf("Validación: La memoria ha sido liberada correctamente. Saliendo del programa.\n");
                    return 0;
                } else {
                    printf("Validación: La memoria aún está en uso.\n");
                }
                break;

            case 4:
                printf("Saliendo del programa.\n");
                return 0;

            default:
                printf("Opción no válida. Intente de nuevo.\n");
        }
    }

    return 0;
}
