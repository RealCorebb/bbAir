#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoJson.h>
#include <NeoPixelBusLg.h>
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
#include <LittleFS.h>
#include "Ticker.h"

U8G2_FOR_ADAFRUIT_GFX gfx;
#include "pixelcorebb.h"

#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

Preferences preferences;
TaskHandle_t SecondCoreTask;

DynamicJsonDocument doc(1024);

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define maxPumps 19

#define OUT1 16
#define OUT2 15
#define OUT3 7
#define OUT4 6
#define OUT5 5
#define OUT6 38
#define OUT7 13
#define OUT8 12
#define OUT9 11
#define OUT10 10
#define OUT11 9
#define OUT12 8
#define OUT13 18
#define OUT14 39
#define OUT15 40
#define OUT16 37
#define OUT17 36
#define OUT18 35
#define OUT19 21

#define OUT20 14   //Not Use
#define OUTAir 17   //Air Pump

#define Stress 4  //Stress sensor

double setpoint = 3830;
double input, output;
double Kp = 10;
double Ki = 0.001;
double Kd = 0;

const float alpha = 0.6;
int sensorValue = 0;
float smoothedValue = 0;
;


int bubbleTime = 0;
int lineTime = 250;
int pumpTime = 100;
int pumpNums = 1;

uint8_t valvePins[20] = {OUT1,OUT2,OUT3,OUT4,OUT5,OUT6,OUT7,OUT8,OUT9,OUT10,OUT11,OUT12,OUT13,OUT14,OUT15,OUT16,OUT17,OUT18,OUT19,OUT20};
uint8_t valveOffsets[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float  multiply[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
uint8_t ledDim[20] = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
Ticker valveTickers[20];

void loop() {
  ArduinoOTA.handle();
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
  analogWriteFrequency(500);
  analogSetAttenuation(ADC_6db);
  preferences.begin("bbAir", false);
  // put your setup code here, to run once:
  for (int i = 0; i < 20; i++) {
    pinMode(valvePins[i], OUTPUT);
    digitalWrite(valvePins[i],LOW);
  }
  pinMode(OUTAir, OUTPUT);
  Serial.begin(115200);
  Serial.println("bbAir");

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
  setupWeb();
  setupLED();
  //WiFi

  //pumpText("你好");
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
  initConfig();
  //OTA


  xTaskCreatePinnedToCore(
    SecondCoreTaskFunction, // Task function
    "SecondCoreTask",      // Task name
    10000,                 // Stack size (words)
    NULL,                  // Task parameters
    1,                     // Task priority
    &SecondCoreTask,       // Task handle
    0                      // Core to run the task on (1 = second core)
  );
  
  xTaskCreatePinnedToCore(
    ledLoop, // Task function
    "StressTask",      // Task name
    10000,                 // Stack size (words)
    NULL,                  // Task parameters
    1,                     // Task priority
    NULL,       // Task handle
    0                      // Core to run the task on (1 = second core)
  );
}


/////////////////////////////////

void onPump(int no,float multi = 1){  //just like JavaScript's setTimeout(), i am so smart thanks to chatGPT.
  digitalWrite(valvePins[no],HIGH); 
  valveTickers[no].once_ms(int(multi * doc["valveOffsets"][no].as<int>()), offPump, no);
}

void offPump(int no){
  digitalWrite(valvePins[no],LOW);
}

void pumpAll(int time = bubbleTime){
  for(int i = 0;i<maxPumps;i++){
    onPump(i);
  }
}

void pumpOneByOne(int time = bubbleTime){
  for(int i = 0;i<maxPumps;i++){
    digitalWrite(valvePins[i],HIGH);
    delay(int(doc["valveOffsets"][i].as<int>()));
    digitalWrite(valvePins[i],LOW);
    delay(300);
  }
}

void pumpText(String text){
  int fontWidth = 4; // width of each character in the font
  int fontHeight = 8; // height of each character in the font

  int bitmapWidth = text.length() * fontWidth;
  int bitmapHeight = fontHeight;
  /*
  Serial.print("Length:");
  Serial.println(text.length());
  Serial.print("Width:");
  Serial.println(bitmapWidth);
  Serial.print("Height:");
  Serial.println(bitmapHeight);  */
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
    if(y > 1 && y < 7){

      pumpNums = 0;
      for (int x = 0; x < bitmapWidth; x++) {
        int pixel = canvas.getPixel(x, y);
        if (pixel == 1) {
          pumpNums += 1;
        }
      }

      for (int x = 0; x < bitmapWidth; x++) {
        int pixel = canvas.getPixel(x, y);
        if (pixel == 1) {
          bitmap[y * bitmapWidth + x] = 1;
          //Serial.print("⬜");
          onPump(x,multiply[pumpNums - 1]);
        }
      //else Serial.print("⬛");
      }
      delay(lineTime);
      
    }    
    
    //Serial.println("");
  }
  

  // Do something with the bitmap, e.g. send it to a server or save it to a file

  delete[] bitmap; // free bitmap memory
}

void textTest(){
  delay(100);
  onPump(1);
  delay(200);
  for(int i =0;i<5;i++){
    onPump(0);
    onPump(2);
    delay(400);
  }
  onPump(0);
  onPump(1);
  onPump(2);
}

int testText = 0;
void SecondCoreTaskFunction(void *pvParameters) {
  //pumpAll();
  while (true) {
    //textTest();
    //Serial.println(smoothedValue);
    //pumpAll();
    pumpOneByOne();    
    //Serial.println("pump");
    //pumpText(String(testText));
    testText += 1;
    //if (testText > 9) testText = 0;
    delay(2000);
  }
}