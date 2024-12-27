#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#define PORT 8080
#define BUF_SIZE 4096
#define DEFAULT_INTERVAL 5 // Intervalo de actualización por defecto

int intervalo_actualizacion = DEFAULT_INTERVAL;

// Función para obtener el nombre del PC si no se proporciona uno
void obtener_nombre_pc(char *nombre_pc) {
    struct utsname uts;
    if (uname(&uts) == 0) {
        strncpy(nombre_pc, uts.nodename, 50);
    } else {
        strcpy(nombre_pc, "Desconocido");
    }
}

// Función que ejecuta el agente para recolectar métricas
void ejecutar_agente(char *output) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO); 
        close(pipefd[1]);
        execl("./agente", "./agente", NULL);
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else { 
        close(pipefd[1]); 
        FILE *stream = fdopen(pipefd[0], "r");
        fread(output, 1, BUF_SIZE, stream);
        fclose(stream);
        close(pipefd[0]);
        wait(NULL); 
    }
}

// Función que envía el dashboard al servidor
void enviar_dashboard_al_servidor(const char *dashboard) {
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Dirección no válida o no soportada");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error en la conexión");
        close(sock);
        return;
    }

    send(sock, dashboard, strlen(dashboard), 0);
    printf("Dashboard enviado:\n%s\n", dashboard);
    close(sock);
}

// Hilo para recolectar métricas y enviarlas al servidor periódicamente
void *ciclo_recoleccion(void *nombre_cliente) {
    char dashboard[BUF_SIZE];
    char mensaje_final[BUF_SIZE];

    while (1) {
        ejecutar_agente(dashboard);
        snprintf(mensaje_final, BUF_SIZE, "CLIENTE: %s\n%s", (char *)nombre_cliente, dashboard);
        enviar_dashboard_al_servidor(mensaje_final);
        sleep(intervalo_actualizacion);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    char nombre_cliente[50] = {0};
    pthread_t hilo_recoleccion;

    // Determinar el nombre del cliente
    if (argc > 1) {
        strncpy(nombre_cliente, argv[1], 50);
    } else {
        obtener_nombre_pc(nombre_cliente);
    }

    // Determinar el intervalo de actualización
    if (argc > 2) {
        intervalo_actualizacion = atoi(argv[2]);
        if (intervalo_actualizacion <= 0) {
            fprintf(stderr, "El intervalo debe ser mayor a 0. Usando el valor por defecto.\n");
            intervalo_actualizacion = DEFAULT_INTERVAL;
        }
    }

    // Crear el hilo para la recolección de métricas
    pthread_create(&hilo_recoleccion, NULL, ciclo_recoleccion, (void *)nombre_cliente);
    pthread_join(hilo_recoleccion, NULL);

    return 0;
}
