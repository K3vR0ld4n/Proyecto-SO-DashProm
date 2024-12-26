#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/utsname.h>

#define PORT 8080
#define BUF_SIZE 4096
#define INTERVALO_ACTUALIZACION 5 // Intervalo de actualización en segundos

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
    FILE *fp = popen("./agente", "r");
    if (fp == NULL) {
        perror("Error al ejecutar el agente");
        exit(EXIT_FAILURE);
    }

    char buffer[BUF_SIZE];
    size_t bytes_read = fread(buffer, 1, BUF_SIZE - 1, fp);
    buffer[bytes_read] = '\0';
    strcpy(output, buffer);

    pclose(fp);
}

// Función que envía el dashboard al servidor
void enviar_dashboard_al_servidor(char *dashboard) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error al crear el socket \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Dirección no válida o no soportada \n");
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error en la conexión \n");
        return;
    }

    send(sock, dashboard, strlen(dashboard), 0);
    printf("Dashboard enviado:\n%s\n", dashboard);

    close(sock);
}

int main(int argc, char *argv[]) {
    char dashboard[BUF_SIZE] = {0};
    char nombre_pc[50] = {0};

    // Determinar el nombre del cliente
    if (argc > 1) {
        strncpy(nombre_pc, argv[1], 50);
    } else {
        obtener_nombre_pc(nombre_pc);
    }

    while (1) {
        ejecutar_agente(dashboard);

        // Agregar el nombre del cliente al mensaje
        char mensaje_final[BUF_SIZE];
        snprintf(mensaje_final, BUF_SIZE, "CLIENTE: %s\n%s", nombre_pc, dashboard);

        enviar_dashboard_al_servidor(mensaje_final);

        sleep(INTERVALO_ACTUALIZACION);
    }

    return 0;
}
