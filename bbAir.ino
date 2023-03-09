#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
U8G2_FOR_ADAFRUIT_GFX gfx;
#include "pixelcorebb.h"

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
  if (Serial.available()) {
    String command = Serial.readString();
    if (command == "reset") {
      ESP.restart();
    }
  }
  
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
  Serial.begin(9600);

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