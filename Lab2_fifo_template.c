// Lab2_fifo_template.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO_PEDIDOS "fifo_pedidos"
#define FIFO_RESPUESTAS "fifo_respuestas"
#define MAX_PEDIDO 128

typedef struct {
    int cliente_id;
    char mensaje[MAX_PEDIDO];
} Pedido;

// Función que inicializa los FIFOs si no existen
void inicializar_fifos() {
    // Crear el FIFO para pedidos
    mkfifo(FIFO_PEDIDOS, 0666);
    // Crear el FIFO para respuestas
    mkfifo(FIFO_RESPUESTAS, 0666);
}

// Función para el cliente
void cliente(int id) {
    Pedido p;
    p.cliente_id = id;

    // 1. Leer pedido desde teclado
    printf("Cliente %d, ingrese su pedido: ", id);
    fgets(p.mensaje, MAX_PEDIDO, stdin);
    p.mensaje[strcspn(p.mensaje, "\n")] = 0; // Eliminar el salto de línea

    // 2. Escribir en FIFO_PEDIDOS
    int fd_pedidos = open(FIFO_PEDIDOS, O_WRONLY);
    write(fd_pedidos, &p, sizeof(Pedido));
    close(fd_pedidos);

    // 3. Leer confirmación desde FIFO_RESPUESTAS
    int fd_respuestas = open(FIFO_RESPUESTAS, O_RDONLY);
    read(fd_respuestas, p.mensaje, MAX_PEDIDO);
    close(fd_respuestas);

    // 4. Mostrar mensaje recibido
    printf("Cliente %d recibió confirmación: %s\n", id, p.mensaje);
}

// Función para la cocina
void cocina() {
    Pedido p;

    // 1. Crear los FIFOs si es necesario
    inicializar_fifos();

    // 2. Bucle infinito para recibir pedidos
    while (1) {
        // 3. Leer pedido desde FIFO_PEDIDOS
        int fd_pedidos = open(FIFO_PEDIDOS, O_RDONLY);
        read(fd_pedidos, &p, sizeof(Pedido));
        close(fd_pedidos);

        // Simular preparación (puedes usar sleep para simular tiempo de cocción)
        printf("Cocina está preparando el pedido del cliente %d: %s\n", p.cliente_id, p.mensaje);
        sleep(2); // Simula el tiempo de preparación

        // 5. Enviar mensaje de confirmación a FIFO_RESPUESTAS
        int fd_respuestas = open(FIFO_RESPUESTAS, O_WRONLY);
        sprintf(p.mensaje, "Pedido del cliente %d preparado", p.cliente_id);
        write(fd_respuestas, p.mensaje, MAX_PEDIDO);
        close(fd_respuestas);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s [cliente ID | cocina]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "cocina") == 0) {
        cocina();
    } else {
        cliente(atoi(argv[1]));
    }

    return 0;
}