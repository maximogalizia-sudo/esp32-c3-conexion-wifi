#include <Arduino.h>  //librerias de arduino
#include <WiFi.h>   //La librería oficial para manejar el stack de red del ESP32.

const char* ssid = "TU_RED";     // Puntero a una cadena de caracteres con el nombre del Wi-Fi.
const char* password = "TU_PASS"; // Contraseña de la red.

void setup() {
  Serial.begin(115200); // Inicia el puerto serie.
  delay(2000);          // Delay de cortesía para que el USB CDC de la PC se estabilice.

  WiFi.mode(WIFI_STA);  // Configura el chip como "Station". 
                        // El ESP32 puede ser AP (crear red) o STA (conectarse a una).
  
  WiFi.begin(ssid, password); // Intenta el "handshake" con el router/gateway.

  // Este es un bucle de bloqueo positivo
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); // Imprime puntos hasta que el estado cambie a "WL_CONNECTED".
  }

  Serial.println(WiFi.localIP()); // Una vez conectado, el router nos da una IP local.
}

void loop() {
  // Verificación de persistencia de red
  if (WiFi.status() != WL_CONNECTED) {
    // Si el router se apaga o hay interferencia, entramos aquí.
    Serial.println("Reconectando...");
    WiFi.disconnect(); // Limpiamos el estado previo.
    WiFi.reconnect();  // Intentamos reenganchar la sesión.
    
    delay(5000); // Esperamos 5 segundos antes de reintentar para no bloquear el procesador.
  }
}