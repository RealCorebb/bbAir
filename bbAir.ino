#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>
#include <WiFi.h>

#include "font.h"

Preferences preferences;



#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define pumpTime 40

#define OUT1 4
#define OUT2 5


void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("OUT1 HIGH");

  // PUMP TEST  -_,-//
    /*
    digitalWrite(OUT1,HIGH);
    delay(pumpTime);
    //Serial.println("OUT1 LOW");
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
    
    
    delay(2000);*/
  // PUMP TEST//

  //Font Test -_,-//
  Serial.println(testFontBlocks[0].startingNum);
  //Font Test//
  
}

void setupWifi(){
  String ssid = preferences.getString("ssid","null");
  String passwd = preferences.getString("passwd","null");
  Serial.print("WIFI:");
  Serial.print(ssid);
  Serial.print(" ");
  Serial.println(passwd);
  //if ssid and passwd not null
  if(ssid != "null" && passwd != "null"){
    //convert String to char array
    WiFi.begin(ssid.c_str(),passwd.c_str());
  }
}

void newCredentials(String ssid,String passwd){
  preferences.putString("ssid",ssid);
  preferences.putString("passwd",passwd);
}


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      String ssid = "";
      String passwd = "";
      bool passFlag = false;
      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++){
          Serial.print(value[i]);
          if(value[i] != ' '){
            if(!passFlag)
              ssid += value[i];
            else
              passwd += value[i];
          }
          else passFlag = true;
        }

        Serial.println();
        Serial.println("*********");

        newCredentials(ssid,passwd);
        setupWifi();
        pCharacteristic->setValue("");
      }
    }
};


void setup() {
  preferences.begin("bbAir", false);
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

  //WiFi -_,-
  setupWifi();
  //WiFi
}