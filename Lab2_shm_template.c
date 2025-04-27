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
    int head;  // índice del próximo pedido a atender
    int tail;  // índice donde el cliente escribe
} BufferCompartido;

// Llaves de memoria compartida y semáforo
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// Índices de los semáforos
// 0 = mutex
// 1 = espacio disponible
// 2 = pedidos disponibles
int sem_id;

// Inicializar los semáforos
void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);          // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS); // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);           // pedidos disponibles
}

// Operación wait (P) - bajar semáforo
void sem_wait(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

// Operación signal (V) - subir semáforo
void sem_signal(int sem_num) {
    struct sembuf op = {sem_num, +1, 0};
    semop(sem_id, &op, 1);
}

// Función del cliente
void cliente(int id) {
    // Conectarse a la memoria compartida
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    char comida[PEDIDO_LEN];
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(comida, PEDIDO_LEN, stdin);
    comida[strcspn(comida, "\n")] = 0; // Eliminar salto de línea

    // Esperar espacio disponible
    sem_wait(1);

    // Entrar a la sección crítica
    sem_wait(0);

    // Registrar el pedido en la cola
    int pos = buffer->tail;
    buffer->cola[pos].cliente_id = id;
    strncpy(buffer->cola[pos].pedido, comida, PEDIDO_LEN);
    buffer->cola[pos].estado = 1; // Pedido recibido

    // Avanzar el tail
    buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

    printf("Cliente %d - Pedido registrado: %s\n", id, comida);

    // Salir de la sección crítica
    sem_signal(0);

    // Avisar que hay un nuevo pedido
    sem_signal(2);

    // Ahora esperar a que el pedido sea preparado
    int preparado = 0;
    while (!preparado) {
        sem_wait(0); // Entrar para leer de forma segura
        for (int i = 0; i < MAX_PEDIDOS; i++) {
            if (buffer->cola[i].cliente_id == id && buffer->cola[i].estado == 2) {
                preparado = 1;
                printf("Cliente %d - Pedido preparado: %s\n", id, buffer->cola[i].pedido);
                break;
            }
        }
        sem_signal(0); // Salir
        if (!preparado) {
            sleep(1); // Esperar un poco antes de volver a checar
        }
    }

    // Desvincular memoria compartida
    shmdt(buffer);
}

// Función de la cocina
void cocina() {
    // Crear o conectar memoria compartida
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    // Inicializar índices si es nuevo
    buffer->head = 0;
    buffer->tail = 0;

    init_semaforos(); // Solo una vez al iniciar

    printf("Cocina lista para recibir pedidos...\n");

    while (1) {
        // Esperar que haya pedidos disponibles
        sem_wait(2);

        // Entrar a la sección crítica
        sem_wait(0);

        // Atender el pedido en la posición head
        int pos = buffer->head;
        Pedido *p = &buffer->cola[pos];

        if (p->estado == 1) { // Pedido recibido
            printf("Cocina - Preparando pedido de cliente %d: %s\n", p->cliente_id, p->pedido);
            sleep(3); // Simular tiempo de preparación

            p->estado = 2; // Pedido preparado
            printf("Cocina - Pedido de cliente %d preparado!\n", p->cliente_id);

            // Avanzar el head
            buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        }

        // Salir de la sección crítica
        sem_signal(0);

        // Liberar espacio en la cola
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
