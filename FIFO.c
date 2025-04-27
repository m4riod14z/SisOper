// Lab2_fifo_complete.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define FIFO_PEDIDOS "fifo_pedidos"
#define FIFO_RESPUESTAS "fifo_respuestas"
#define MAX_PEDIDO 128
#define MAX_RESPUESTA 256

typedef struct {
    int cliente_id;
    char mensaje[MAX_PEDIDO];
} Pedido;

typedef struct {
    int cliente_id;
    int estado;  // 1: recibido, 2: preparado
    char mensaje[MAX_RESPUESTA];
} Respuesta;

// Función para crear los FIFOs si no existen
void inicializar_fifos() {
    struct stat st;
    
    // Verificar si el FIFO_PEDIDOS existe, sino crearlo
    if (stat(FIFO_PEDIDOS, &st) == -1) {
        if (mkfifo(FIFO_PEDIDOS, 0666) == -1) {
            perror("Error al crear el FIFO de pedidos");
            exit(EXIT_FAILURE);
        }
        printf("FIFO de pedidos creado.\n");
    }
    
    // Verificar si el FIFO_RESPUESTAS existe, sino crearlo
    if (stat(FIFO_RESPUESTAS, &st) == -1) {
        if (mkfifo(FIFO_RESPUESTAS, 0666) == -1) {
            perror("Error al crear el FIFO de respuestas");
            exit(EXIT_FAILURE);
        }
        printf("FIFO de respuestas creado.\n");
    }
}

// Función para el cliente
void cliente(int id) {
    Pedido p;
    Respuesta r;
    int fd_pedidos, fd_respuestas;
    
    p.cliente_id = id;
    
    // 1. Leer pedido desde teclado
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(p.mensaje, MAX_PEDIDO, stdin);
    p.mensaje[strcspn(p.mensaje, "\n")] = 0; // Eliminar el salto de línea
    
    // 2. Abrir y escribir en FIFO_PEDIDOS
    printf("Cliente %d - Enviando pedido: %s\n", id, p.mensaje);
    fd_pedidos = open(FIFO_PEDIDOS, O_WRONLY);
    if (fd_pedidos == -1) {
        perror("Error al abrir el FIFO de pedidos");
        exit(EXIT_FAILURE);
    }
    
    // Para evitar escrituras parciales o entrelazadas
    if (write(fd_pedidos, &p, sizeof(Pedido)) != sizeof(Pedido)) {
        perror("Error al escribir en el FIFO de pedidos");
        close(fd_pedidos);
        exit(EXIT_FAILURE);
    }
    close(fd_pedidos);
    
    // 3. Abrir y leer confirmación de recepción desde FIFO_RESPUESTAS
    printf("Cliente %d - Esperando confirmación de recepción...\n", id);
    fd_respuestas = open(FIFO_RESPUESTAS, O_RDONLY);
    if (fd_respuestas == -1) {
        perror("Error al abrir el FIFO de respuestas");
        exit(EXIT_FAILURE);
    }
    
    int encontrado = 0;
    while (!encontrado) {
        if (read(fd_respuestas, &r, sizeof(Respuesta)) == sizeof(Respuesta)) {
            if (r.cliente_id == id && r.estado == 1) {
                // 4. Mostrar mensaje de confirmación de recepción
                printf("Cliente %d - Confirmación recibida: %s\n", id, r.mensaje);
                encontrado = 1;
            }
        }
    }
    close(fd_respuestas);
    
    // 5. Esperar y leer confirmación de preparación
    printf("Cliente %d - Esperando confirmación de preparación...\n", id);
    fd_respuestas = open(FIFO_RESPUESTAS, O_RDONLY);
    if (fd_respuestas == -1) {
        perror("Error al abrir el FIFO de respuestas");
        exit(EXIT_FAILURE);
    }
    
    encontrado = 0;
    while (!encontrado) {
        if (read(fd_respuestas, &r, sizeof(Respuesta)) == sizeof(Respuesta)) {
            if (r.cliente_id == id && r.estado == 2) {
                // 6. Mostrar mensaje de confirmación de preparación
                printf("Cliente %d - Pedido listo: %s\n", id, r.mensaje);
                encontrado = 1;
            }
        }
    }
    close(fd_respuestas);
}

// Manejador de señales para limpieza al terminar
void limpiar(int sig) {
    unlink(FIFO_PEDIDOS);
    unlink(FIFO_RESPUESTAS);
    printf("\nFIFOs eliminados. Saliendo...\n");
    exit(0);
}

// Función para la cocina
void cocina() {
    Pedido p;
    Respuesta r;
    int fd_pedidos, fd_respuestas;
    
    // 1. Crear los FIFOs
    inicializar_fifos();
    
    // Configurar manejador de señales para limpieza al terminar
    signal(SIGINT, limpiar);  // Ctrl+C
    
    printf("Cocina iniciada. Esperando pedidos...\n");
    
    // 2. Bucle infinito para recibir pedidos
    while (1) {
        // Abrir FIFO para lectura de pedidos
        fd_pedidos = open(FIFO_PEDIDOS, O_RDONLY);
        if (fd_pedidos == -1) {
            perror("Error al abrir el FIFO de pedidos");
            continue;
        }
        
        // 3. Leer pedido desde FIFO_PEDIDOS
        if (read(fd_pedidos, &p, sizeof(Pedido)) == sizeof(Pedido)) {
            printf("Cocina: Pedido recibido del Cliente %d: %s\n", p.cliente_id, p.mensaje);
            close(fd_pedidos);
            
            // Enviar confirmación de recepción
            r.cliente_id = p.cliente_id;
            r.estado = 1; // Recibido
            sprintf(r.mensaje, "Pedido '%s' recibido correctamente", p.mensaje);
            
            fd_respuestas = open(FIFO_RESPUESTAS, O_WRONLY);
            if (fd_respuestas == -1) {
                perror("Error al abrir el FIFO de respuestas para confirmación");
                continue;
            }
            
            write(fd_respuestas, &r, sizeof(Respuesta));
            close(fd_respuestas);
            
            // 4. Simular preparación
            printf("Cocina: Preparando pedido '%s' del Cliente %d...\n", p.mensaje, p.cliente_id);
            sleep(3); // Simulación de tiempo de preparación
            
            // 5. Enviar mensaje de confirmación a FIFO_RESPUESTAS
            r.estado = 2; // Preparado
            sprintf(r.mensaje, "Su pedido '%s' está listo para recoger", p.mensaje);
            
            fd_respuestas = open(FIFO_RESPUESTAS, O_WRONLY);
            if (fd_respuestas == -1) {
                perror("Error al abrir el FIFO de respuestas para finalización");
                continue;
            }
            
            write(fd_respuestas, &r, sizeof(Respuesta));
            close(fd_respuestas);
            
            printf("Cocina: Pedido '%s' para Cliente %d listo y notificado.\n", p.mensaje, p.cliente_id);
        } else {
            close(fd_pedidos);
        }
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
