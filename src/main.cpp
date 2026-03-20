#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define SENSOR_PIN       0
#define TIME_TO_SLEEP    15
#define uS_TO_S_FACTOR   1000000ULL
#define TIMEOUT_CONEXION 20000

const int VALOR_SECO = 3408;
const int VALOR_AGUA = 1500;

RTC_DATA_ATTR int bootCount = 0;

BLECharacteristic* pCharacteristic;
bool deviceConnected = false;
bool dataSent = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("[BLE] Gateway conectado.");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("[BLE] Gateway desconectado.");
  }
};

void goToDeepSleep() {
  Serial.printf("\n--- FIN CICLO %d | Deep Sleep por %d seg ---\n", bootCount, TIME_TO_SLEEP);
  Serial.flush();
  BLEDevice::deinit(true);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void setup() {

  Serial.begin(115200);
  bootCount++;
  delay(3000); // ← 3 segundos para abrir el monitor
  Serial.printf("\n=== CICLO %d ===\n", bootCount);


 

  analogReadResolution(12);
  int lecturaCruda = analogRead(SENSOR_PIN);
  int humedad = map(lecturaCruda, VALOR_SECO, VALOR_AGUA, 0, 100);
  humedad = constrain(humedad, 0, 100);
  Serial.printf("Lectura cruda: %d | Humedad: %d%%\n", lecturaCruda, humedad);

  BLEDevice::init("Maceta_C3");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());

  char strData[10];
  sprintf(strData, "%d", humedad);
  pCharacteristic->setValue(strData);

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->start();

  Serial.println("[BLE] Anunciando... esperando al gateway.");

  unsigned long startTime = millis();
  while (millis() - startTime < TIMEOUT_CONEXION) {
    if (deviceConnected && !dataSent) {
      delay(500);
      if (deviceConnected) {
        pCharacteristic->setValue(strData);
        pCharacteristic->notify();
        Serial.printf("[BLE] Dato enviado: %s%%\n", strData);
        dataSent = true;
        delay(1500);
        break;
      }
    }
    delay(10);
  }

  if (!dataSent) {
    Serial.println("[WARN] Timeout: ningún gateway se conectó.");
  }
  delay(5000);
  goToDeepSleep();
}

void loop() {}