#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define BUF_SIZE 4096

// Mutex para proteger el acceso a las métricas
pthread_mutex_t mutex;

// Recolecta métricas del sistema
void recolectar_metricas_sistema(char *buffer) {
    char temp[256];
    FILE *fp;

    pthread_mutex_lock(&mutex);

    // Comando para obtener el uso de CPU
    fp = popen("top -bn1 | grep 'Cpu' | awk '{print $2}'", "r");
    if (fp == NULL) {
        perror("Error al ejecutar top");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float cpu_usage = atof(temp);

    // Comando para obtener la memoria usada y disponible
    fp = popen("free | grep Mem | awk '{print $3/$2 * 100.0}'", "r");
    if (fp == NULL) {
        perror("Error al ejecutar free");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float mem_usage = atof(temp);

    // Comando para obtener el uso del disco
    fp = popen("df / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
    if (fp == NULL) {
        perror("Error al ejecutar df");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float disk_usage = atof(temp);

    // Número de procesos activos
    fp = popen("ps aux | wc -l", "r");
    if (fp == NULL) {
        perror("Error al ejecutar ps");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    int num_procesos = atoi(temp);

    // Temperatura del CPU
    fp = popen("sensors | grep 'Package id 0' | awk '{print $4}' | sed 's/+//;s/°C//'", "r");
    if (fp == NULL) {
        perror("Error al ejecutar sensors");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float temp_cpu = atof(temp);

    // Uso de red
    fp = popen("cat /proc/net/dev | awk 'NR > 2 {rx+=$2; tx+=$10} END {print (rx+tx)/1024}'", "r");
    if (fp == NULL) {
        perror("Error al calcular uso de red");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float net_usage = atof(temp);

    // Espacio de intercambio usado (%)
    fp = popen("free | grep Swap | awk '{print $3/$2 * 100.0}'", "r");
    if (fp == NULL) {
        perror("Error al ejecutar free para swap");
        pthread_mutex_unlock(&mutex);
        return;
    }
    fgets(temp, sizeof(temp), fp);
    pclose(fp);
    float swap_usage = atof(temp);

    // Construir la salida final
    snprintf(buffer, BUF_SIZE,
             "CPU: %.2f\nMEMORIA: %.2f\nDISCO: %.2f\nPROCESOS: %d\nTEMPERATURA_CPU: %.2f\nRED: %.2f KB/s\nSWAP: %.2f\n",
             cpu_usage, mem_usage, disk_usage, num_procesos, temp_cpu, net_usage, swap_usage);

    pthread_mutex_unlock(&mutex);
}

int main() {
    char metrics[BUF_SIZE] = {0};
    pthread_mutex_init(&mutex, NULL);
    recolectar_metricas_sistema(metrics);
    printf("%s", metrics);
    pthread_mutex_destroy(&mutex);
    return 0;
}
