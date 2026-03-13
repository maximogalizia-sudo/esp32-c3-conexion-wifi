#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // <-- 1. LIBRERÍA CLAVE PARA LAS NOTIFICACIONES

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SENSOR_PIN          0
#define TIME_TO_SLEEP       15
#define uS_TO_S_FACTOR      1000000ULL

// --- VALORES DE CALIBRACIÓN ---
const int VALOR_SECO = 3408;  // El valor que obtuvimos al aire
const int VALOR_AGUA = 1500;  // Valor estimado para humedad máxima (Ajustar luego)
//al momento de enterrar el sensor este 1500 debera ajustarse 

// Memoria RTC: Se mantiene viva durante el Deep Sleep
RTC_DATA_ATTR int bootCount = 0; 

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool dataSent = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void goToDeepSleep() {
  Serial.printf("\n--- FIN CICLO %d ---\n", bootCount);
  Serial.println("Entrando en Deep Sleep...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  bootCount++; // Aumentamos el contador en cada reinicio
  
  delay(3000); // Ventana de seguridad para programar
  Serial.printf("\n--- INICIO CICLO %d ---\n", bootCount);

  analogReadResolution(12);
  int lecturaCruda = analogRead(SENSOR_PIN);
  
  // Transformamos la lectura cruda a porcentaje (0-100)
  int porcentajeHumedad = map(lecturaCruda, VALOR_SECO, VALOR_AGUA, 0, 100);
  
  // Limitamos para evitar porcentajes negativos o mayores a 100
  porcentajeHumedad = constrain(porcentajeHumedad, 0, 100);

  Serial.printf("Lectura cruda: %d | Humedad calculada: %d%%\n", lecturaCruda, porcentajeHumedad);

  // 2. Setup BLE
  BLEDevice::init("Maceta_C3_V2"); // Cambié el nombre para forzar al celu a refrescar
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ | 
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  // Publicidad agresiva para conexión rápida
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true); 
  pAdvertising->setMinPreferred(0x02); // Intervalo más rápido
  pAdvertising->start();

  Serial.println("Anunciando... Tenés 20 seg para conectar.");

  unsigned long startTime = millis();
  while (millis() - startTime < 20000) {
   if (deviceConnected && !dataSent) {
      Serial.println("Estabilizando conexión...");
      
      // Esperamos pero no asumimos que sigue conectado
      delay(1500); 
      
      // --- 3. DOBLE CHEQUEO DE CONEXIÓN ---
      // Verificamos si después del delay el celu no se fue
      if (deviceConnected) {
          char strData[10];
          sprintf(strData, "%d", porcentajeHumedad);
    
          pCharacteristic->setValue(strData);
          pCharacteristic->notify(); 
          
          Serial.printf("Dato mandado: %s\n", strData);
          dataSent = true;
          
          delay(2500); // Tiempo seguro para que el paquete viaje por el aire
          break; 
      } else {
          Serial.println("El cliente se desconectó antes de recibir el dato. Reintentando...");
      }
    }
    delay(10);
  
  }

  if (!dataSent) Serial.println("Timeout: Nadie se conectó.");
  delay(5000);
  goToDeepSleep();
}

void loop() {}