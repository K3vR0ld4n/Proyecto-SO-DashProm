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
#define MAX_ALERTS 5

typedef struct {
    char cliente[50];
    float cpu_usage;
    float mem_usage;
    float disk_usage;
    int num_procesos;
    float temp_cpu;
    float net_usage;
    int cpu_alert_count;
    int mem_alert_count;
} MetricData;

MetricData *clientes = NULL;
int num_clientes = 0;

void enviar_alerta(const char *cliente, const char *metrica, float valor) {
    CURL *curl;
    CURLcode res;

    const char *account_sid = getenv("TWILIO_ACCOUNT_SID");
    const char *auth_token = getenv("TWILIO_AUTH_TOKEN");
    const char *twilio_whatsapp_number = getenv("TWILIO_WHATSAPP_NUMBER");
    const char *recipient_whatsapp_number = getenv("TWILIO_RECIPIENT_WHATSAPP_NUMBER");

    if (!account_sid || !auth_token || !twilio_whatsapp_number || !recipient_whatsapp_number) {
        fprintf(stderr, "Faltan variables de entorno para Twilio\n");
        return;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        char url[256];
        snprintf(url, sizeof(url), "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json", account_sid);

        char message[1024];
        snprintf(message, sizeof(message), "Alerta: Cliente '%s' tiene un alto uso de %s: %.2f%%", cliente, metrica, valor);

        char postfields[2048];
        snprintf(postfields, sizeof(postfields), "From=whatsapp:%s&To=whatsapp:%s&Body=%s",
                 twilio_whatsapp_number, recipient_whatsapp_number, message);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

        char userpwd[512];
        snprintf(userpwd, sizeof(userpwd), "%s:%s", account_sid, auth_token);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void procesar_metrica(char *mensaje) {
    MetricData metrica;
    sscanf(mensaje, "CLIENTE: %49s\nCPU: %f\nMEMORIA: %f\nDISCO: %f\nPROCESOS: %d\nTEMPERATURA_CPU: %f\nRED: %f\n",
           metrica.cliente, &metrica.cpu_usage, &metrica.mem_usage, &metrica.disk_usage,
           &metrica.num_procesos, &metrica.temp_cpu, &metrica.net_usage);

    int encontrado = 0;
    for (int i = 0; i < num_clientes; i++) {
        if (strcmp(clientes[i].cliente, metrica.cliente) == 0) {
            clientes[i] = metrica;
            encontrado = 1;
            break;
        }
    }

    if (!encontrado) {
        num_clientes++;
        clientes = realloc(clientes, num_clientes * sizeof(MetricData));
        clientes[num_clientes - 1] = metrica;
    }

    if (metrica.cpu_usage > INITIAL_THRESHOLD_CPU) {
        enviar_alerta(metrica.cliente, "CPU", metrica.cpu_usage);
    }

    if (metrica.mem_usage > INITIAL_THRESHOLD_MEM) {
        enviar_alerta(metrica.cliente, "Memoria", metrica.mem_usage);
    }
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

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        char buffer[BUF_SIZE] = {0};
        int bytes_read = read(client_socket, buffer, BUF_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Mensaje recibido:\n%s\n", buffer);
            procesar_metrica(buffer);
        }

        close(client_socket);
    }

    free(clientes);
    close(server_fd);
    return 0;
}
