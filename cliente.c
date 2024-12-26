#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 4096
#define INTERVALO_ACTUALIZACION 5 // Intervalo de actualización en segundos

// Función que recolecta métricas del sistema
void recolectar_metricas_sistema(char *buffer) {
    char temp[256];
    FILE *fp;

    // Comando para obtener el uso de CPU
    fp = popen("top -bn1 | grep 'Cpu' | awk '{print $2}'", "r");
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float cpu_usage = atof(temp);

    // Comando para obtener la memoria usada y disponible
    fp = popen("free | grep Mem | awk '{print $3/$2 * 100.0}'", "r");
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float mem_usage = atof(temp);

    // Comando para obtener el uso del disco
    fp = popen("df / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float disk_usage = atof(temp);

    // Número de procesos activos
    fp = popen("ps aux | wc -l", "r");
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    int num_procesos = atoi(temp);

    // Temperatura del CPU
    fp = popen("sensors | grep 'Package id 0' | awk '{print $4}' | sed 's/+//;s/°C//'", "r");
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float temp_cpu = atof(temp);

    // Porcentaje de uso de la red
    fp = popen("ifstat 1 1 | awk 'NR==3 {print $1}'", "r");
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float net_usage = atof(temp);

    snprintf(buffer, BUF_SIZE,
             "CLIENTE: %s\nCPU: %.2f\nMEMORIA: %.2f\nDISCO: %.2f\nPROCESOS: %d\nTEMPERATURA_CPU: %.2f\nRED: %.2f\n",
             "cliente1", cpu_usage, mem_usage, disk_usage, num_procesos, temp_cpu, net_usage);
}

// Función que envía el dashboard al servidor
void enviar_dashboard_al_servidor(char *dashboard) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error al crear el socket \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir dirección IP a formato binario
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Dirección no válida o no soportada \n");
        return;
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error en la conexión \n");
        return;
    }

    // Enviar el dashboard al servidor
    send(sock, dashboard, strlen(dashboard), 0);
    printf("Dashboard enviado:\n%s\n", dashboard);

    close(sock);
}

int main() {
    char dashboard[BUF_SIZE] = {0};

    while (1) {
        recolectar_metricas_sistema(dashboard);
        enviar_dashboard_al_servidor(dashboard);
        sleep(INTERVALO_ACTUALIZACION);
    }

    return 0;
}
