#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// Tamaño del buffer para leer y escribir en bloques de 1024 bytes
#define BUFFER_SIZE 1024

/*
 * Función que copia el contenido de un archivo a otro.
 * Preserva los permisos del archivo original.
 * Parámetros:
 *   - origen: ruta del archivo a copiar.
 *   - destino: ruta donde se guardará la copia.
 */
void copiar_archivo(const char *origen, const char *destino) {
    int fd_origen, fd_destino;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_leidos, bytes_escritos;
    struct stat info_archivo;

    // Abrir archivo de origen
    fd_origen = open(origen, O_RDONLY);
    if (fd_origen == -1) {
        perror("Error al intentar abrir el archivo origen");
        exit(EXIT_FAILURE);
    }

    // Obtener permisos del archivo origen
    if (fstat(fd_origen, &info_archivo) == -1) {
        perror("Error al obtener la información del archivo origen");
        close(fd_origen);
        exit(EXIT_FAILURE);
    }

    // Crear archivo destino con los mismos permisos del archivo origen
    fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, info_archivo.st_mode);
    if (fd_destino == -1) {
        perror("Error al crear el archivo destino");
        close(fd_origen);
        exit(EXIT_FAILURE);
    }

    // Leer y escribir en bloques en el archivo destino
    while ((bytes_leidos = read(fd_origen, buffer, BUFFER_SIZE)) > 0) {
        bytes_escritos = write(fd_destino, buffer, bytes_leidos);
        if (bytes_escritos != bytes_leidos) {
            perror("Error al escribir en el archivo destino");
            close(fd_origen);
            close(fd_destino);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_leidos == -1) {
        perror("Error al leer el archivo origen");
    }

    close(fd_origen);
    close(fd_destino);
}

/*
 * Función main: valida los argumentos y llama a copiar_archivo().
 */  

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_origen> <archivo_destino>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    copiar_archivo(argv[1], argv[2]);
    printf("El archivo fue copiado exitosamente.\n");

    return 0;
}
