#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// UUIDs para el servicio y la característica
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

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
}

void loop() {
  if (deviceConnected) {
    // Simulamos la lectura de humedad
    float humedad = 42.5; 
    
    char strHumedad[8];
    dtostrf(humedad, 1, 2, strHumedad);
    
    pCharacteristic->setValue(strHumedad);
    pCharacteristic->notify(); // Notificamos el cambio al Gateway
    
    Serial.printf("Enviando humedad simulada: %s%%\n", strHumedad);
    delay(5000); 
  }
}