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

// Estructura del pedido
typedef struct {
    int cliente_id;
    int pedido_id; // id único por pedido
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

// Estructura completa de memoria compartida
typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head;  // índice de próximo pedido a atender
    int tail;  // índice donde el cliente escribe
} BufferCompartido;

// Llaves para memoria y semáforos
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// Semáforos: 0 = mutex, 1 = espacio disponible, 2 = pedidos disponibles
int sem_id;

// Inicializar semáforos
void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);          // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS); // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);           // pedidos disponibles
}

// Operación P (wait)
void sem_wait(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

// Operación V (signal)
void sem_signal(int sem_num) {
    struct sembuf op = {sem_num, +1, 0};
    semop(sem_id, &op, 1);
}

// Cliente: envía pedidos
void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    static int next_pedido_id = 1; // Contador interno para cada pedido

    while (1) {
        char comida[PEDIDO_LEN];
        printf("Cliente %d - Ingrese su pedido (o 'salir' para terminar): ", id);
        fgets(comida, PEDIDO_LEN, stdin);
        comida[strcspn(comida, "\n")] = 0;

        if (strcmp(comida, "salir") == 0) {
            break;
        }

        sem_wait(1); // espera espacio disponible
        sem_wait(0); // lock mutex

        int pos = buffer->tail;
        buffer->cola[pos].cliente_id = id;
        buffer->cola[pos].pedido_id = next_pedido_id;
        strncpy(buffer->cola[pos].pedido, comida, PEDIDO_LEN);
        buffer->cola[pos].estado = 1; // recibido

        int mi_pedido_id = next_pedido_id; // recordar qué pedido esperar
        next_pedido_id++;

        buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

        printf("Cliente %d - Pedido registrado: %s (Pedido ID: %d)\n", id, comida, mi_pedido_id);

        sem_signal(0); // unlock mutex
        sem_signal(2); // avisar que hay un pedido nuevo

        // Ahora esperar que mi pedido esté preparado
        int preparado = 0;
        while (!preparado) {
            sem_wait(0); // lock mutex
            for (int i = 0; i < MAX_PEDIDOS; i++) {
                if (buffer->cola[i].cliente_id == id &&
                    buffer->cola[i].pedido_id == mi_pedido_id &&
                    buffer->cola[i].estado == 2) {

                    preparado = 1;
                    printf("Cliente %d - Pedido preparado: %s (Pedido ID: %d)\n", id, buffer->cola[i].pedido, mi_pedido_id);
                    buffer->cola[i].estado = 0; // liberar espacio
                }
            }
            sem_signal(0); // unlock mutex
            if (!preparado) {
                sleep(1); // esperar antes de volver a revisar
            }
        }
    }

    shmdt(buffer);
}

// Cocina: atiende pedidos
void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    init_semaforos(); // inicializar semáforos al inicio

    printf("Cocina iniciada y lista para recibir pedidos...\n");

    while (1) {
        sem_wait(2); // esperar que haya pedido disponible
        sem_wait(0); // lock mutex

        int pos = buffer->head;
        if (buffer->cola[pos].estado == 1) {
            printf("Cocina - Preparando pedido de cliente %d: %s (Pedido ID: %d)\n",
                buffer->cola[pos].cliente_id, buffer->cola[pos].pedido, buffer->cola[pos].pedido_id);

            sleep(2); // tiempo de preparación simulado

            buffer->cola[pos].estado = 2; // marcar como preparado
            printf("Cocina - Pedido preparado para cliente %d: %s (Pedido ID: %d)\n",
                buffer->cola[pos].cliente_id, buffer->cola[pos].pedido, buffer->cola[pos].pedido_id);

            buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        }

        sem_signal(0); // unlock mutex
        sem_signal(1); // espacio disponible otra vez
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
