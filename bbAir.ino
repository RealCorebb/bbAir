#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
U8G2_FOR_ADAFRUIT_GFX gfx;
#include "pixelcorebb.h"

Preferences preferences;


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define pumpTime 40
#define maxPumps 3

#define OUT1 4
#define OUT2 5
#define OUT3 6
#define OUT4 7
#define OUT5 15
#define OUT6 16
#define OUT7 17
#define OUT8 18
#define OUT9 8
#define OUT10 9
#define OUT11 10
#define OUT12 11
#define OUT13 12
#define OUT14 13
#define OUT15 14
#define OUT16 21
#define OUT17 35
#define OUT18 36
#define OUT19 37
#define OUT20 38
#define OUT21 39

uint8_t valvePins[20] = {OUT1,OUT2,OUT3,OUT4,OUT5,OUT6,OUT7,OUT8,OUT9,OUT10,OUT11,OUT12,OUT13,OUT14,OUT15,OUT16,OUT17,OUT18,OUT19,OUT20};


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
  if (Serial.available()) {
    String command = Serial.readString();
    if (command == "reset") {
      ESP.restart();
    }
  }
  ArduinoOTA.handle();
  pumpAll(20);
  delay(980);
  
}

void setupWifi(){
  String ssid = preferences.getString("ssid","Hollyshit_A");
  String passwd = preferences.getString("passwd","00197633");
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
  for (int i = 0; i < 20; i++) {
    pinMode(valvePins[i], OUTPUT);
  }
  pinMode(OUT21,OUTPUT);
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

  pumpText("你好");
  //Debugging OTA -_,-
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();  
  //OTA
}


/////////////////////////////////

void pumpText(String text){
  int fontWidth = 8; // width of each character in the font
  int fontHeight = 8; // height of each character in the font

  int bitmapWidth = text.length() * fontWidth;
  int bitmapHeight = fontHeight;
  Serial.print("Length:");
  Serial.println(text.length());
  Serial.print("Width:");
  Serial.println(bitmapWidth);
  Serial.print("Height:");
  Serial.println(bitmapHeight);  
  GFXcanvas1 canvas(bitmapWidth, bitmapHeight);
  gfx.begin(canvas);
  byte* bitmap = new byte[bitmapWidth * bitmapHeight]; // create bitmap array

  // Clear bitmap
  memset(bitmap, 0, bitmapWidth * bitmapHeight);

  // Draw text on bitmap
  gfx.setFont(pixelcorebb);
  gfx.setCursor(0, fontHeight-1);
  gfx.println(text);

  // Convert bitmap to 0/1 array
  for (int y = 0; y < bitmapHeight; y++) {
    for (int x = 0; x < bitmapWidth; x++) {
      int pixel = canvas.getPixel(x, y);
      if (pixel == 1) {
        bitmap[y * bitmapWidth + x] = 1;
        Serial.print("⬜");
      }
      else Serial.print("⬛");
    }
    Serial.println("");
  }

  // Do something with the bitmap, e.g. send it to a server or save it to a file

  delete[] bitmap; // free bitmap memory
}

void pumpAll(int time){
  digitalWrite(OUT21,HIGH);
  delay(pumpTime);  
  for(int i = 0;i<maxPumps;i++){
    digitalWrite(valvePins[i],HIGH);
    delay(time);
    digitalWrite(valvePins[i],LOW);
  }
  digitalWrite(OUT21,LOW);
}