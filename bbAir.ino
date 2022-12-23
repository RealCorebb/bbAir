#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};


#define pumpTime 40

#define OUT1 4
#define OUT2 5

void setup() {
  // put your setup code here, to run once:
  pinMode(OUT1,OUTPUT);
  pinMode(OUT2,OUTPUT);
  Serial.begin(115200);

  //BLE -_,-
  BLEDevice::init("bbAir");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  //BLE
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("OUT1 HIGH");
  digitalWrite(OUT1,HIGH);
  delay(pumpTime);
  Serial.println("OUT1 LOW");
  digitalWrite(OUT1,LOW);  

  for(int i = 0;i<3;i++){
    digitalWrite(OUT2,HIGH);
    delay(20);
    digitalWrite(OUT2,LOW);
digitalWrite(OUT1,HIGH);    
    delay(pumpTime);
digitalWrite(OUT1,LOW);  
delay(200);    
  }
  
  
  delay(2000);
  
}
