#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

// Valores predeterminados para los parámetros
int mem_stress_size_mb = 500; // Tamaño de memoria a utilizar en MB
int num_threads = 2;           // Número de hilos para estresar el CPU
int stress_duration = 30;      // Duración del estrés en segundos
int disk_file_size_mb = 2000;  // Tamaño del archivo en MB para estrés en disco

// Función para mostrar el mensaje de ayuda
void mostrar_ayuda(const char *nombre_programa) {
    printf("Uso: %s [opciones]\n", nombre_programa);
    printf("Opciones:\n");
    printf("  -m <MB>   Tamaño de memoria a estresar (en MB). Valor predeterminado: 2000.\n");
    printf("  -t <hilos> Número de hilos para estresar la CPU. Valor predeterminado: 8.\n");
    printf("  -d <seg>  Duración de la prueba de estrés (en segundos). Valor predeterminado: 30.\n");
    printf("  -f <MB>   Tamaño del archivo para estresar el disco (en MB). Valor predeterminado: 1000.\n");
    printf("  -h        Muestra este mensaje de ayuda y termina.\n");
    printf("\nEjemplo:\n");
    printf("  %s -m 3000 -t 16 -d 60 -f 2000\n", nombre_programa);
    printf("    Realiza una prueba de estrés con:\n");
    printf("    - 3000 MB de memoria.\n");
    printf("    - 16 hilos de CPU.\n");
    printf("    - 60 segundos de duración.\n");
    printf("    - 2000 MB de archivo en disco.\n");
    exit(EXIT_SUCCESS);
}

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
    size_t size = mem_stress_size_mb * 1024 * 1024; 
    char *mem = (char *)malloc(size);

    if (mem == NULL) {
        perror("Error al asignar memoria");
        return;
    }

    memset(mem, 'A', size);
    printf("Memoria de %d MB ocupada.\n", mem_stress_size_mb);

    sleep(stress_duration);
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

    size_t size = disk_file_size_mb * 1024 * 1024; 
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

    printf("Archivo de %d MB escrito en disco.\n", disk_file_size_mb);
    fflush(file);

    sleep(stress_duration);

    fclose(file);
    free(data);

    if (remove("disk_stress_test.tmp") == 0) {
        printf("Archivo temporal eliminado.\n");
    } else {
        perror("Error al eliminar archivo temporal");
    }
}

int main(int argc, char *argv[]) {
    pthread_t *threads;
    int opt;

    // Manejo de opciones
    while ((opt = getopt(argc, argv, "m:t:d:f:h")) != -1) {
        switch (opt) {
            case 'm': // Tamaño de memoria
                mem_stress_size_mb = atoi(optarg);
                break;
            case 't': // Número de hilos
                num_threads = atoi(optarg);
                break;
            case 'd': // Duración del estrés
                stress_duration = atoi(optarg);
                break;
            case 'f': // Tamaño del archivo de disco
                disk_file_size_mb = atoi(optarg);
                break;
            case 'h': // Ayuda
                mostrar_ayuda(argv[0]);
                break;
            default:
                mostrar_ayuda(argv[0]);
        }
    }

    printf("Configuración de la prueba de estrés:\n");
    printf(" - Memoria: %d MB\n", mem_stress_size_mb);
    printf(" - Hilos de CPU: %d\n", num_threads);
    printf(" - Duración: %d segundos\n", stress_duration);
    printf(" - Archivo de disco: %d MB\n", disk_file_size_mb);

    threads = malloc(num_threads * sizeof(pthread_t));
    if (threads == NULL) {
        perror("Error al asignar memoria para los hilos");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, estresar_cpu, NULL) != 0) {
            perror("Error al crear hilo");
            free(threads);
            exit(EXIT_FAILURE);
        }
    }

    // Estresar la memoria
    estresar_memoria();

    // Estresar el disco
    estresar_disco();

    // Esperar el tiempo de duración del estrés
    sleep(stress_duration);

    // Terminar los hilos (forzar terminación ya que están en un bucle infinito)
    for (int i = 0; i < num_threads; i++) {
        pthread_cancel(threads[i]);
    }

    free(threads);
    printf("Estrés finalizado.\n");
    return 0;
}
