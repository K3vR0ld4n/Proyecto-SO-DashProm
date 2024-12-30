#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <curl/curl.h>

#define PORT 8080
#define BUF_SIZE 4096
#define CPU_THRESHOLD 90.0
#define MEM_THRESHOLD 80.
#define DISCO_THRESHOLD 90.0
#define TEMP_THRESHOLD 85.0

// Callback para ignorar la respuesta de Twilio
size_t no_op_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
    return size * nmemb;
}

// Función para enviar alertas mediante Twilio
void enviar_alerta(const char *cliente, const char *metrica, float valor)
{
    CURL *curl;
    CURLcode res;

    const char *account_sid = getenv("TWILIO_ACCOUNT_SID");
    const char *auth_token = getenv("TWILIO_AUTH_TOKEN");
    const char *twilio_whatsapp_number = getenv("TWILIO_WHATSAPP_NUMBER");
    const char *recipient_whatsapp_number = getenv("TWILIO_RECIPIENT_WHATSAPP_NUMBER");
    /*
        if (!account_sid || !auth_token || !twilio_whatsapp_number || !recipient_whatsapp_number)
        {
            fprintf(stderr, "Error: Faltan variables de entorno para Twilio\n");
            return;
        }
    */
    if (!account_sid)
    {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_ACCOUNT_SID\n");
        return 1;
    }
    if (!auth_token)
    {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_AUTH_TOKEN\n");
        return 1;
    }
    if (!twilio_whatsapp_number)
    {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_WHATSAPP_NUMBER\n");
        return 1;
    }
    if (!recipient_whatsapp_number)
    {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_RECIPIENT_WHATSAPP_NUMBER\n");
        return 1;
    }
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        char *encoded_from = curl_easy_escape(curl, twilio_whatsapp_number, 0);
        char *encoded_to = curl_easy_escape(curl, recipient_whatsapp_number, 0);
        char url[256];
        snprintf(url, sizeof(url), "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json", account_sid);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Construir el cuerpo del mensaje
        char message[1024];
        snprintf(message, sizeof(message),
                 "ALERTA: Cliente '%s' tiene un alto uso de %s: %.2f%%", cliente, metrica, valor);
        char postfields[2048];
        snprintf(postfields, sizeof(postfields), "From=%s&To=%s&Body=%s", encoded_from, encoded_to, message);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

        char userpwd[512];
        snprintf(userpwd, sizeof(userpwd), "%s:%s", account_sid, auth_token);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, no_op_callback);
        printf("%s\n", message);
        // Realizar la solicitud
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Limpiar
        curl_easy_cleanup(curl);
        curl_free(encoded_from);
        curl_free(encoded_to);
    }
    else
    {
        fprintf(stderr, "curl_easy_init() failed\n");
    }
    curl_global_cleanup();
}

// Función para procesar las métricas y verificar alertas
void procesar_metrica(char *mensaje)
{
    char cliente[50];
    float cpu_usage, mem_usage, disk_usage, temp_cpu, net_usage;
    int procesos;

    sscanf(mensaje,
           "CLIENTE: %49[^\n]\nCPU: %f\nMEMORIA: %f\nDISCO: %f\nPROCESOS: %d\nTEMPERATURA_CPU: %f\nRED: %f\n",
           cliente, &cpu_usage, &mem_usage, &disk_usage, &procesos, &temp_cpu, &net_usage);

    // Imprimir las métricas con símbolos correspondientes
    printf("\n--- Dashboard ---\n");
    printf("Cliente: %s\n", cliente);
    printf("Uso de CPU: %.2f%%\n", cpu_usage);     // Agregar el símbolo de porcentaje
    printf("Uso de Memoria: %.2f%%\n", mem_usage); // Agregar el símbolo de porcentaje
    printf("Uso de Disco: %.2f%%\n", disk_usage);  // Agregar el símbolo de porcentaje
    printf("Número de Procesos: %d\n", procesos);
    printf("Temperatura del CPU: %.2f°C\n", temp_cpu); // Agregar el símbolo de grados Celsius
    printf("Uso de Red: %.2f KB/s\n", net_usage);      // Indicar que está en KB/s
    printf("-----------------\n");

    // Verificar alertas
    if (cpu_usage > CPU_THRESHOLD)
    {
        enviar_alerta(cliente, "CPU", cpu_usage);
    }

    if (mem_usage > MEM_THRESHOLD)
    {
        enviar_alerta(cliente, "Memoria", mem_usage);
    }

    if (disk_usage > DISCO_THRESHOLD)
    {
        enviar_alerta(cliente, "Disco", disk_usage);
    }

    if (temp_cpu > TEMP_THRESHOLD)
    {
        enviar_alerta(cliente, "Temperatura_CPU", temp_cpu);
    }
}

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d...\n", PORT);

    while (1)
    {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            continue;
        }

        char buffer[BUF_SIZE] = {0};
        int bytes_read = read(client_socket, buffer, BUF_SIZE - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            procesar_metrica(buffer);
        }

        close(client_socket);
    }

    close(server_fd);
    return 0;
}
