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

typedef struct {
    int cliente_id;
    int pedido_id;
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head;
    int tail;
    int next_pedido_id; // nuevo: ID único para todos
} BufferCompartido;

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

int sem_id;

void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS);
    semctl(sem_id, 2, SETVAL, 0);
}

void sem_wait(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

void sem_signal(int sem_num) {
    struct sembuf op = {sem_num, +1, 0};
    semop(sem_id, &op, 1);
}

void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    if (shm_id == -1) {
        perror("Error accediendo a la memoria compartida");
        exit(1);
    }
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    while (1) {
        char comida[PEDIDO_LEN];
        printf("Cliente %d - Ingrese su pedido (o 'salir'): ", id);
        fgets(comida, PEDIDO_LEN, stdin);
        comida[strcspn(comida, "\n")] = 0;

        if (strcmp(comida, "salir") == 0) {
            break;
        }

        sem_wait(1);
        sem_wait(0);

        int pos = buffer->tail;

        buffer->cola[pos].cliente_id = id;
        buffer->cola[pos].pedido_id = buffer->next_pedido_id++;
        strncpy(buffer->cola[pos].pedido, comida, PEDIDO_LEN);
        buffer->cola[pos].estado = 1; // recibido

        int mi_pedido_id = buffer->cola[pos].pedido_id;

        buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

        printf("Cliente %d - Pedido enviado: %s (ID: %d)\n", id, comida, mi_pedido_id);

        sem_signal(0);
        sem_signal(2);

        // Esperar que la cocina prepare mi pedido
        int preparado = 0;
        while (!preparado) {
            sem_wait(0);
            for (int i = 0; i < MAX_PEDIDOS; i++) {
                if (buffer->cola[i].cliente_id == id &&
                    buffer->cola[i].pedido_id == mi_pedido_id &&
                    buffer->cola[i].estado == 2) {

                    preparado = 1;
                    printf("Cliente %d - Pedido preparado: %s\n", id, buffer->cola[i].pedido);
                    buffer->cola[i].estado = 0; // liberar
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

void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    // Inicializar la memoria compartida la primera vez
    if (buffer->next_pedido_id == 0) {
        for (int i = 0; i < MAX_PEDIDOS; i++) {
            buffer->cola[i].estado = 0;
        }
        buffer->head = 0;
        buffer->tail = 0;
        buffer->next_pedido_id = 1;
        init_semaforos();
    }

    printf("Cocina iniciada\n");

    while (1) {
        sem_wait(2);
        sem_wait(0);

        int pos = buffer->head;
        if (buffer->cola[pos].estado == 1) {
            printf("Cocina - Preparando pedido de cliente %d: %s (ID: %d)\n",
                buffer->cola[pos].cliente_id, buffer->cola[pos].pedido, buffer->cola[pos].pedido_id);

            sleep(2);

            buffer->cola[pos].estado = 2; // preparado
            printf("Cocina - Pedido listo para cliente %d: %s\n",
                buffer->cola[pos].cliente_id, buffer->cola[pos].pedido);

            buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        }

        sem_signal(0);
        sem_signal(1);
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
