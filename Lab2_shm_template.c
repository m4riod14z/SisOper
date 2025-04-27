// Lab2_shm.c
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
    int pedido_id;  // Nuevo: identificador único del pedido
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

// Estructura de la memoria compartida
typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head;  // índice del próximo pedido a atender
    int tail;  // índice donde el cliente escribe
    int next_pedido_id; // siguiente ID de pedido disponible
} BufferCompartido;

// Llaves de memoria y semáforo
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// Semáforos: 0 = mutex, 1 = espacio disponible, 2 = pedidos disponibles
int sem_id;

// Inicializar semáforos
void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);         // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS); // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);         // pedidos disponibles
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

// Cliente: escribe un pedido
void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    if (shm_id == -1) {
        perror("Cliente - Error al acceder a la memoria compartida");
        exit(1);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    if (buffer == (void *) -1) {
        perror("Cliente - Error al enlazar memoria");
        exit(1);
    }

    char comida[PEDIDO_LEN];
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(comida, PEDIDO_LEN, stdin);
    comida[strcspn(comida, "\n")] = 0; // quitar salto de línea

    // Esperar a que haya espacio disponible
    sem_wait(1);
    // Esperar el mutex para modificar la cola
    sem_wait(0);

    int pos = buffer->tail;
    buffer->cola[pos].cliente_id = id;
    buffer->cola[pos].pedido_id = buffer->next_pedido_id++;
    strncpy(buffer->cola[pos].pedido, comida, PEDIDO_LEN);
    buffer->cola[pos].estado = 1; // recibido

    printf("Cliente %d - Pedido enviado (ID: %d)\n", id, buffer->cola[pos].pedido_id);

    buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

    // Liberar mutex
    sem_signal(0);
    // Señalar que hay un pedido disponible
    sem_signal(2);

    // Esperar a que el pedido sea preparado
    int pedido_id = buffer->cola[pos].pedido_id;
    while (1) {
        sem_wait(0); // entrar en sección crítica

        if (buffer->cola[pos].pedido_id == pedido_id && buffer->cola[pos].estado == 2) {
            printf("Cliente %d - Su pedido '%s' está listo\n", id, buffer->cola[pos].pedido);
            sem_signal(0);
            break;
        }

        sem_signal(0);
        usleep(100000); // dormir 100 ms para no saturar CPU
    }

    shmdt(buffer);
}

// Cocina: procesa pedidos
void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | IPC_EXCL | 0666);
    BufferCompartido *buffer;

    if (shm_id != -1) {
        // Memoria recién creada
        printf("Cocina - Creando nueva memoria compartida...\n");
        buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

        memset(buffer, 0, sizeof(BufferCompartido));
        buffer->head = 0;
        buffer->tail = 0;
        buffer->next_pedido_id = 1;

        init_semaforos();
    } else {
        // Ya existe, solo conectarse
        shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
        buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

        sem_id = semget(SEM_KEY, 3, 0666); // Recuperar semáforos
    }

    if (buffer == (void *) -1) {
        perror("Cocina - Error al enlazar memoria");
        exit(1);
    }

    printf("Cocina - Iniciada\n");

    while (1) {
        // Esperar a que haya un pedido disponible
        sem_wait(2);
        // Entrar en sección crítica
        sem_wait(0);

        int pos = buffer->head;
        if (buffer->cola[pos].estado == 1) {
            printf("Cocina - Preparando pedido de cliente %d: %s (ID: %d)\n",
                   buffer->cola[pos].cliente_id,
                   buffer->cola[pos].pedido,
                   buffer->cola[pos].pedido_id);

            sleep(2); // Simular preparación

            buffer->cola[pos].estado = 2; // marcar como preparado
            printf("Cocina - Pedido listo para cliente %d: %s\n",
                   buffer->cola[pos].cliente_id,
                   buffer->cola[pos].pedido);

            buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        }

        // Salir de sección crítica
        sem_signal(0);
        // Señalar que hay espacio disponible
        sem_signal(1);
    }

    shmdt(buffer);
}

// Programa principal
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
