// Lab2_shm_complete.c
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

//esta misma llave la deben tener los clientes y la cocina:
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
// Semáforos: 0 = mutex, 1 = espacio disponible, 2 = pedidos disponibles
//mutex = mutual exclusion lock, candado de exclusión mutua.
int sem_id;

// Función de inicialización de semáforos
void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);         // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS); // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);         // pedidos disponibles
}

//Estructuras de datos para gestión del semáforo, que controla el acceso
//al recurso compartido:
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

// Función para conectarse a los semáforos existentes
void conectar_semaforos() {
    sem_id = semget(SEM_KEY, 3, 0666);
    if (sem_id == -1) {
        perror("Error al conectar con los semáforos");
        exit(1);
    }
}

// Cliente: escribe pedido
void cliente(int id) {
    // Conectarse a la memoria compartida
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    if (shm_id == -1) {
        perror("Cliente: Error al conectar con la memoria compartida");
        exit(1);
    }
    
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    if (buffer == (BufferCompartido *)-1) {
        perror("Cliente: Error al adjuntar la memoria compartida");
        exit(1);
    }
    
    // Conectar con los semáforos existentes
    conectar_semaforos();
    
    char comida[PEDIDO_LEN];
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(comida, PEDIDO_LEN, stdin);
    comida[strcspn(comida, "\n")] = 0; // Eliminar el salto de línea
    
    // Esperar a que haya espacio disponible en la cola
    printf("Cliente %d - Esperando espacio en la cola...\n", id);
    sem_wait(1); // Decrementar semáforo de espacios disponibles
    
    // Solicitar acceso exclusivo al buffer
    sem_wait(0); // Decrementar mutex
    
    // Agregar pedido a la cola
    int pos = buffer->tail;
    buffer->cola[pos].cliente_id = id;
    strncpy(buffer->cola[pos].pedido, comida, PEDIDO_LEN);
    buffer->cola[pos].estado = 1; // Marcar como recibido
    
    // Guardar una copia local del pedido y la posición para verificar después
    char mi_pedido[PEDIDO_LEN];
    strncpy(mi_pedido, comida, PEDIDO_LEN);
    int mi_posicion = pos;
    
    // Actualizar el índice tail
    buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;
    
    printf("Cliente %d - Pedido registrado: %s (posición %d)\n", id, comida, pos);
    
    // Liberar el mutex
    sem_signal(0);
    
    // Notificar que hay un nuevo pedido disponible
    sem_signal(2);
    
    // Esperar confirmación (poll)
    printf("Cliente %d - Esperando confirmación...\n", id);
    int preparado = 0;
    while (!preparado) {
        sem_wait(0); // Adquirir mutex para leer
        
        // Verificar específicamente nuestro pedido por posición
        if (buffer->cola[mi_posicion].cliente_id == id && 
            strcmp(buffer->cola[mi_posicion].pedido, mi_pedido) == 0 && 
            buffer->cola[mi_posicion].estado == 2) {
            printf("Cliente %d - ¡Pedido preparado! %s\n", id, buffer->cola[mi_posicion].pedido);
            preparado = 1;
        }
        
        sem_signal(0); // Liberar mutex
        
        if (!preparado) {
            // Esperar un poco antes de revisar de nuevo
            usleep(500000); // 500ms
        }
    }
    
    // Desvincular memoria compartida
    shmdt(buffer);
}

// Cocina: atiende pedido
void cocina() {
    // Crear o conectarse a la memoria compartida
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Cocina: Error al crear/conectar con la memoria compartida");
        exit(1);
    }
    
    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    if (buffer == (BufferCompartido *)-1) {
        perror("Cocina: Error al adjuntar la memoria compartida");
        exit(1);
    }
    
    // Inicializar la memoria compartida
    buffer->head = 0;
    buffer->tail = 0;
    for (int i = 0; i < MAX_PEDIDOS; i++) {
        buffer->cola[i].estado = 0; // Libre
    }
    
    // Inicializar semáforos
    init_semaforos();
    
    printf("Cocina iniciada. Esperando pedidos...\n");
    
    // Espera activa de los pedidos que llegan del cliente:
    while (1) {
        // Esperar a que haya pedidos disponibles
        printf("Cocina: Esperando nuevos pedidos...\n");
        sem_wait(2); // Decrementar semáforo de pedidos disponibles
        
        // Solicitar acceso exclusivo al buffer
        sem_wait(0); // Decrementar mutex
        
        // Obtener el próximo pedido
        int pos = buffer->head;
        int cliente_id = buffer->cola[pos].cliente_id;
        char pedido[PEDIDO_LEN];
        strncpy(pedido, buffer->cola[pos].pedido, PEDIDO_LEN);
        
        printf("Cocina: Preparando pedido de Cliente %d: %s (posición %d)\n", cliente_id, pedido, pos);
        
        // Actualizar el índice head
        buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        
        // Simular preparación
        sem_signal(0); // Liberar mutex mientras cocinamos
        
        // Simular tiempo de preparación
        sleep(3);
        
        // Volver a adquirir mutex para actualizar estado
        sem_wait(0);
        
        // Marcar el pedido específico como preparado (por posición, no buscando en la cola)
        buffer->cola[pos].estado = 2; // Preparado
        
        printf("Cocina: Pedido listo para Cliente %d: %s (posición %d)\n", cliente_id, pedido, pos);
        
        // Liberar mutex
        sem_signal(0);
        
        // Incrementar semáforo de espacios disponibles
        sem_signal(1);
    }
    
    // Nunca llegamos aquí, pero por completitud:
    shmdt(buffer);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
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
