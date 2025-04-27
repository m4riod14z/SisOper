// Lab2_shm_template.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAX_PEDIDOS 5
#define PEDIDO_LEN 64

// Estructura de un pedido
typedef struct {
    int cliente_id;
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

// Memoria compartida completa
typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head;  // siguiente pedido que atiende la cocina
    int tail;  // posición donde escribe el cliente
} BufferCompartido;

// Llaves compartidas
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// Semáforos: 0 = mutex, 1 = espacios disponibles, 2 = pedidos disponibles
int sem_id;

// Inicializa los semáforos
void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);         // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS); // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);         // pedidos disponibles
}

// Operaciones de semáforo
void sem_wait(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

void sem_signal(int sem_num) {
    struct sembuf op = {sem_num, +1, 0};
    semop(sem_id, &op, 1);
}

// Código del cliente
void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    if (shm_id == -1) {
        perror("Error al acceder a memoria compartida");
        exit(1);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    if (buffer == (void *)-1) {
        perror("Error al enlazar memoria compartida");
        exit(1);
    }

    while (1) {
        char comida[PEDIDO_LEN];
        printf("Cliente %d - Ingrese su pedido (o 'salir' para terminar): ", id);
        fgets(comida, PEDIDO_LEN, stdin);
        comida[strcspn(comida, "\n")] = 0;

        if (strcmp(comida, "salir") == 0) {
            break;
        }

        // Esperar espacio disponible
        sem_wait(1);

        // Entrar a la sección crítica
        sem_wait(0);

        int pos = buffer->tail;
        buffer->cola[pos].cliente_id = id;
        strncpy(buffer->cola[pos].pedido, comida, PEDIDO_LEN);
        buffer->cola[pos].estado = 1; // recibido

        buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

        printf("Cliente %d - Pedido registrado: %s\n", id, comida);

        sem_signal(0); // salir sección crítica

        // Avisar que hay un nuevo pedido disponible
        sem_signal(2);

        // Esperar a que el pedido esté preparado
        int preparado = 0;
        while (!preparado) {
            sem_wait(0);
            for (int i = 0; i < MAX_PEDIDOS; i++) {
                if (buffer->cola[i].cliente_id == id && buffer->cola[i].estado == 2) {
                    preparado = 1;
                    printf("Cliente %d - Pedido preparado: %s\n", id, buffer->cola[i].pedido);
                    // Marcar casilla libre otra vez (opcional)
                    buffer->cola[i].estado = 0;
                }
            }
            sem_signal(0);
            if (!preparado) {
                sleep(1);
            }
        }
    }

    shmdt(buffer);
}

// Código de la cocina
void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    if (buffer == (void *)-1) {
        perror("Error al enlazar memoria compartida");
        exit(1);
    }

    init_semaforos();

    buffer->head = 0;
    buffer->tail = 0;
    for (int i = 0; i < MAX_PEDIDOS; i++) {
        buffer->cola[i].estado = 0;
    }

    printf("Cocina lista para recibir pedidos.\n");

    while (1) {
        sem_wait(2); // esperar pedidos disponibles
        sem_wait(0); // entrar sección crítica

        int pos = buffer->head;
        if (buffer->cola[pos].estado == 1) {
            printf("Cocina preparando pedido de Cliente %d: %s\n",
                   buffer->cola[pos].cliente_id, buffer->cola[pos].pedido);

            sleep(3); // Simular tiempo de preparación

            buffer->cola[pos].estado = 2; // marcar preparado
            printf("Cocina terminó pedido de Cliente %d: %s\n",
                   buffer->cola[pos].cliente_id, buffer->cola[pos].pedido);

            buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        }

        sem_signal(0); // salir sección crítica
        sem_signal(1); // liberar espacio
    }

    shmdt(buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s [cliente ID | cocina]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "cocina") == 0) {
        cocina();
    } else {
        int id = atoi(argv[1]);
        cliente(id);
    }

    return 0;
}
