#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <curl/curl.h>

#define PORT 8080
#define BUF_SIZE 4096
#define INITIAL_THRESHOLD_CPU 90.0
#define INITIAL_THRESHOLD_MEM 80.0

typedef struct {
    char cliente[50];
    float cpu_usage;
    float mem_usage;
    float disk_usage;
    int num_procesos;
    float temp_cpu;
    float net_usage;
} MetricData;

MetricData *clientes = NULL;
int num_clientes = 0;

void procesar_metrica(char *mensaje) {
    MetricData metrica;
    sscanf(mensaje, "CLIENTE: %49s\nCPU: %f\nMEMORIA: %f\nDISCO: %f\nPROCESOS: %d\nTEMPERATURA_CPU: %f\nRED: %f\n",
           metrica.cliente, &metrica.cpu_usage, &metrica.mem_usage, &metrica.disk_usage,
           &metrica.num_procesos, &metrica.temp_cpu, &metrica.net_usage);

    printf("\n--- Dashboard ---\n%s", mensaje);
    printf("-----------------\n");
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d...\n", PORT);

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        char buffer[BUF_SIZE] = {0};
        int bytes_read = read(client_socket, buffer, BUF_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            procesar_metrica(buffer);
        }

        close(client_socket);
    }

    free(clientes);
    close(server_fd);
    return 0;
}
