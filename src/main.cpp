#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// UUIDs para el servicio y la característica
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define TIME_TO_SLEEP  30        // Segundos de Deep Sleep 
#define uS_TO_S_FACTOR 1000000ULL

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool dataSent = false;

// Callbacks para gestionar el estado de la conexión
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("¡Dispositivo conectado!");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Dispositivo desconectado. Reiniciando anuncios...");
      pServer->getAdvertising()->start();
    }
};

void goToDeepSleep() {
  Serial.println("Entrando en Deep Sleep...");
  Serial.flush(); // Espera a que termine de imprimir en consola antes de apagar
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}


void setup() {
  Serial.begin(115200);
  delay(2000); 

  Serial.println("Iniciando Servidor BLE...");

  // 1. Inicializar BLE
  BLEDevice::init("Maceta_Nodo_C3");

  // 2. Crear el Servidor y asignar Callbacks
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Crear el Servicio
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Crear la Característica (Lectura y Notificación)
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // 5. Iniciar Servicio y Anuncios
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("Esperando que el cliente BLE se conecte...");
// 3. VENTANA DE ESCUCHA Y TRABAJO
  unsigned long startTime = millis();
  

  while (millis() - startTime < 30000) {
    
    if (deviceConnected && !dataSent) {
  Serial.println("¡Conexión detectada! Esperando 2 segundos para que el celular se acomode...");
      delay(2000); // PAUSA 1: Le damos tiempo a la app para descubrir los servicios

      float humedad = 42.5; 
      char strHumedad[8];
      dtostrf(humedad, 1, 2, strHumedad);
      
      pCharacteristic->setValue(strHumedad);
      pCharacteristic->notify(); 
      
      Serial.printf("Enviando humedad: %s%%\n", strHumedad);
      dataSent = true; // Marcamos que ya enviamos
      
      delay(2000); // Le damos 1 segundo al chip de radio para enviar el paquete físico
      break;       // Rompemos el ciclo 'while' para irnos a dormir temprano
    }
    
    delay(10); // Pequeño respiro para que el procesador no colapse
  }

  // 4. VERIFICACIÓN Y SUEÑO
  if (!dataSent) {
    Serial.println("Nadie se conectó a tiempo. Abortando envío para ahorrar energía.");
  }

  // Se apaga todo (CPU, Bluetooth, Wi-Fi). El código termina acá.
  goToDeepSleep();
  
}

void loop() {
  
}