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
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

// Estructura completa de memoria compartida
typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head;  // índice de próximo pedido a atender
    int tail;  // índice donde el cliente escribe
} BufferCompartido;

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

int sem_id;

// Función de inicialización de semáforos
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

// Cliente: escribe pedido
void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    char comida[PEDIDO_LEN];
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(comida, PEDIDO_LEN, stdin);
    comida[strcspn(comida, "\n")] = 0;

    sem_wait(1); // Esperar espacio disponible
    sem_wait(0); // Entrar en sección crítica

    // Registrar el pedido
    buffer->cola[buffer->tail].cliente_id = id;
    strncpy(buffer->cola[buffer->tail].pedido, comida, PEDIDO_LEN);
    buffer->cola[buffer->tail].estado = 1; // Pedido recibido
    buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

    sem_signal(0); // Salir de sección crítica
    sem_signal(2); // Notificar que hay un pedido disponible

    printf("Cliente %d - Pedido recibido: %s\n", id, comida);

    // Esperar confirmación de que el pedido fue preparado
    sem_wait(0); // Entrar en sección crítica
    while (buffer->cola[(buffer->tail - 1 + MAX_PEDIDOS) % MAX_PEDIDOS].estado != 2) {
        sem_signal(0); // Salir de sección crítica
        usleep(100000); // Esperar un poco antes de volver a comprobar
        sem_wait(0); // Entrar en sección crítica
    }
    sem_signal(0); // Salir de sección crítica

    printf("Cliente %d - Pedido preparado: %s\n", id, comida);

    shmdt(buffer);
}

// Cocina: atiende pedido
void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    init_semaforos(); // Solo una vez al iniciar

    while (1) {
        sem_wait(2); // Esperar a que haya un pedido disponible
        sem_wait(0); // Entrar en sección crítica

        // Atender el pedido
        Pedido *pedido = &buffer->cola[buffer->head];
        if (pedido->estado ==  1) {
            printf("Cocina - Atendiendo pedido de cliente %d: %s\n", pedido->cliente_id, pedido->pedido);
            sleep(2); // Simular tiempo de preparación
            pedido->estado = 2; // Pedido preparado
            buffer->head = (buffer->head + 1) % MAX_PEDIDOS; // Mover el índice del pedido atendido
        }

        sem_signal(0); // Salir de sección crítica
        sem_signal(1); // Notificar que hay espacio disponible
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
