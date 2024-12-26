#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 4096

// Función para recolectar métricas del sistema
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
             "CPU: %.2f\nMEMORIA: %.2f\nDISCO: %.2f\nPROCESOS: %d\nTEMPERATURA_CPU: %.2f\nRED: %.2f\n",
             cpu_usage, mem_usage, disk_usage, num_procesos, temp_cpu, net_usage);
}

int main() {
    char metrics[BUF_SIZE] = {0};
    recolectar_metricas_sistema(metrics);
    printf("%s", metrics);
    return 0;
}
