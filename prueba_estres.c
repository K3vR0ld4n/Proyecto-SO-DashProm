#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MEM_STRESS_SIZE_MB 2000 // Tamaño de memoria a utilizar en MB
#define NUM_THREADS 8           // Número de hilos para estresar el CPU
#define STRESS_DURATION 30      // Duración del estrés en segundos
#define DISK_FILE_SIZE_MB 1000  // Tamaño del archivo en MB para estrés en disco

// Función para estresar el CPU
void *estresar_cpu(void *arg) {
    printf("Iniciando estrés en CPU...\n");
    while (1) {
        double x = 0.1;
        for (int i = 0; i < 1000000; i++) {
            x += x * x;
        }
    }
    return NULL;
}

// Función para estresar la memoria
void estresar_memoria() {
    printf("Iniciando estrés en memoria...\n");
    size_t size = MEM_STRESS_SIZE_MB * 1024 * 1024; 
    char *mem = (char *)malloc(size);

    if (mem == NULL) {
        perror("Error al asignar memoria");
        return;
    }

    memset(mem, 'A', size);
    printf("Memoria de %d MB ocupada.\n", MEM_STRESS_SIZE_MB);

    sleep(STRESS_DURATION);
    free(mem);
    printf("Memoria liberada.\n");
}

// Función para estresar el disco
void estresar_disco() {
    printf("Iniciando estrés en disco...\n");
    FILE *file = fopen("disk_stress_test.tmp", "w");
    if (file == NULL) {
        perror("Error al crear archivo temporal");
        return;
    }

    size_t size = DISK_FILE_SIZE_MB * 1024 * 1024; 
    char *data = (char *)malloc(1024); 
    if (data == NULL) {
        perror("Error al asignar memoria para disco");
        fclose(file);
        return;
    }

    memset(data, 'A', 1024); 
    size_t written = 0;

    while (written < size) {
        fwrite(data, 1, 1024, file);
        written += 1024;
    }

    printf("Archivo de %d MB escrito en disco.\n", DISK_FILE_SIZE_MB);
    fflush(file);

    sleep(STRESS_DURATION);

    fclose(file);
    free(data);

    // Eliminar el archivo temporal
    if (remove("disk_stress_test.tmp") == 0) {
        printf("Archivo temporal eliminado.\n");
    } else {
        perror("Error al eliminar archivo temporal");
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int i;

    printf("Duración del estrés: %d segundos\n", STRESS_DURATION);

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, estresar_cpu, NULL) != 0) {
            perror("Error al crear hilo");
            exit(EXIT_FAILURE);
        }
    }

    // Estresar la memoria
    estresar_memoria();

    // Estresar el disco
    estresar_disco();

    // Esperar el tiempo de duración del estrés
    sleep(STRESS_DURATION);

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_cancel(threads[i]);
    }

    printf("Estrés finalizado.\n");
    return 0;
}
