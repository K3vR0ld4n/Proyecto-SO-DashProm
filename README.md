# **Proyecto_Parcial_SO_LokiLite**

Este README explica cómo utilizar el sistema de monitoreo de métricas que hemos desarrollado. El sistema incluye un **agente**, un **cliente**, un **servidor**, y un programa de **prueba de estrés**. Este sistema es capaz de monitorear métricas del sistema en tiempo real, enviar alertas a través de WhatsApp cuando se superan ciertos umbrales, y simular situaciones de alto consumo de recursos para validar su funcionamiento.

---

### **Requisitos previos**

- **Twilio API**: Para enviar alertas por WhatsApp, necesitas una cuenta de Twilio con tu **`ACCOUNT_SID`**, **`AUTH_TOKEN`**, y los números de teléfono correspondientes.
- **Linux**: El proyecto está diseñado para sistemas basados en Linux.
- **Dependencias**:
  - `libcurl`: Utilizada para interactuar con la API de Twilio.
  - Herramientas del sistema como `free`, `sensors`, `df`, y `ip` deben estar disponibles.
  - Opcional: `sensors-detect` para configurar monitoreo de temperatura.

---

### **Estructura del proyecto**

1. **`agente.c`**:
   - Recolecta las métricas del sistema, incluyendo:
     - Uso de CPU.
     - Uso de memoria.
     - Uso de disco.
     - Número de procesos en ejecución.
     - Temperatura del CPU.
     - Uso de red.
   - Genera un dashboard tabular con las métricas.
   - Soporta el paso de parámetros para personalizar el nombre del cliente.

2. **`cliente.c`**:
   - Ejecuta periódicamente al **agente** y captura su salida.
   - Transmite los datos recolectados al **servidor** a través de sockets TCP.
   - Permite personalizar el intervalo de actualización a través de un argumento.

3. **`servidor.c`**:
   - Recibe y procesa las métricas enviadas por los clientes.
   - Monitorea si alguna métrica supera los umbrales definidos.
   - Envía alertas vía WhatsApp utilizando la API de Twilio.
   - Ajusta dinámicamente los umbrales para evitar alertas excesivas.

4. **`prueba_estres.c`**:
   - Simula cargas intensas de CPU, memoria y disco para validar el sistema.
   - Los parámetros de estrés son configurables (memoria, hilos, duración, archivo en disco).

---

### **Cómo ejecutar el proyecto**

#### **1. Compilación**

Compila los archivos del proyecto utilizando el `Makefile` incluido:

```bash
make
```

Esto generará los ejecutables: **`agente`**, **`cliente`**, **`servidor`**, y **`prueba_estres`**.

---

#### **2. Ejecución**

1. **Ejecuta el servidor**:
   En una terminal, inicia el servidor que recibirá los datos de los clientes:

   ```bash
   ./servidor
   ```

2. **Ejecuta el cliente**:
   Inicia el cliente especificando las métricas a monitorear y el intervalo de actualización (en segundos):

   ```bash
   ./cliente <nombre_cliente> <intervalo_actualizacion>
   ```

   - Si no se especifica un nombre, se usará el nombre del host.
   - Si no se especifica el intervalo, se utilizará un valor predeterminado de 5 segundos.

   **Ejemplo**:
   ```bash
   ./cliente Cliente1 10
   ```

3. **Ejecuta el programa de prueba de estrés**:
   Simula altas cargas en el sistema para validar el comportamiento del servidor:

   ```bash
   ./prueba_estres -m 4000 -t 16 -d 60 -f 2000
   ```

   - **`-m`**: Memoria en MB.
   - **`-t`**: Número de hilos.
   - **`-d`**: Duración en segundos.
   - **`-f`**: Tamaño del archivo de disco en MB.
   - Para ayuda:
     ```bash
     ./prueba_estres -h
     ```

---

### **Configuración de Twilio**

Antes de ejecutar el servidor, configura las variables de entorno necesarias:

1. Edita tu archivo de configuración:
   ```bash
   nano ~/.bashrc  # o ~/.bash_profile
   ```

2. Agrega las siguientes líneas al final:
   ```bash
   export TWILIO_ACCOUNT_SID="tu_account_sid"
   export TWILIO_AUTH_TOKEN="tu_auth_token"
   export TWILIO_WHATSAPP_NUMBER="whatsapp:+14155238886"
   export TWILIO_RECIPIENT_WHATSAPP_NUMBER="whatsapp:+1234567890"
   ```

3. Guarda el archivo y recarga las variables:
   ```bash
   source ~/.bashrc
   ```

---

### **Alertas y Umbrales**

- El servidor envía alertas cuando una métrica supera un umbral predefinido.
- Se envían hasta **5 alertas** consecutivas por servicio. Si se alcanzan, el umbral se duplica para evitar alertas excesivas.
- Las alertas incluyen:
  - Nombre del cliente.
  - Métrica que excedió el umbral.
  - Valor actual y umbral definido.

---

### **Dependencias**

1. **Librerías necesarias**:
   - Instala `libcurl`:
     ```bash
     sudo apt-get install libcurl4-openssl-dev
     ```

2. **Herramientas del sistema**:
   - Asegúrate de tener disponibles los comandos:
     - `free`: Para monitorear el uso de memoria.
     - `df`: Para monitorear el uso de disco.
     - `sensors`: Para monitorear la temperatura del CPU.
     - `ip`: Para monitorear el uso de red.
   - Configura `sensors` si es necesario:
     ```bash
     sudo sensors-detect
     ```

---
