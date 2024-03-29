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
#include <TridentTD_Base64.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>

U8G2_FOR_ADAFRUIT_GFX gfx;
#include "pixelcorebb.h"

#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

Preferences preferences;
TaskHandle_t SecondCoreTask;

DynamicJsonDocument doc(4096);
JsonArray schedule;

const char* ntpServerName = "pool.ntp.org";
int timeZone = 0;  // Initialize with a default timezone offset in hours

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName, timeZone * 3600, 60000);

String ledMode = "rainbow";
uint32_t staticColor = 0x89CFF0;

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
int lineTime = 2500;
int pumpTime = 100;
int pumpNums = 1;

uint8_t valvePins[20] = {OUT1,OUT2,OUT3,OUT4,OUT5,OUT6,OUT7,OUT8,OUT9,OUT10,OUT11,OUT12,OUT13,OUT14,OUT15,OUT16,OUT17,OUT18,OUT19,OUT20};
float  multiply[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
uint8_t ledDim[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//For better result in the situation that has multiple bubbles on the same col in a short time.
uint8_t curCounts[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned long lastPumpTime[20] = {0};

Ticker valveDelayTickers[20];
Ticker valveTickers[20];
Ticker ledTickers[20];

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
  //analogSetAttenuation(ADC_0db);
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
  timeClient.begin();
  //WiFi

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
  /*
  xTaskCreatePinnedToCore(
    stressLoop, // Task function
    "StressTask",      // Task name
    10000,                 // Stack size (words)
    NULL,                  // Task parameters
    1,                     // Task priority
    NULL,       // Task handle
    0                      // Core to run the task on (1 = second core)
  );*/
  
  xTaskCreatePinnedToCore(
    ledLoop, // Task function
    "LedTask",      // Task name
    10000,                 // Stack size (words)
    NULL,                  // Task parameters
    1,                     // Task priority
    NULL,       // Task handle
    1                      // Core to run the task on (1 = second core)
  );
}


/////////////////////////////////
void onPump(int no,float multi = 1){  //just like JavaScript's setTimeout(), i am so smart thanks to chatGPT.
  unsigned long currentTime = millis() + (lineTime + lineTime * 0.2);

  // Check if enough time has passed since the last pump
  if (currentTime - lastPumpTime[no] <= (lineTime + lineTime * 0.1)) {
    curCounts[no]++; // Increment count if within lineTime
  } else {
    curCounts[no] = 0; // Reset count if longer than lineTime
  }

  // Update the last pump time
  lastPumpTime[no] = currentTime;
  int delayTime = doc["valveDelays"][no].as<int>();

  valveDelayTickers[no].once_ms(delayTime,onPumpFun,no);

  valveTickers[no].once_ms(int(multi * doc["valveOffsets"][0][no].as<int>()) + delayTime, offPump, no);  //curCounts[no]
  ledTickers[no].once_ms(3000 + delayTime,offLed,no);
}

void onPumpFun(int no){
  digitalWrite(valvePins[no],HIGH);
  ledDim[no] = 255;
}

void offPump(int no){
  digitalWrite(valvePins[no],LOW);
}

void offLed(int no){
  if(ledDim[no] > 0){
    ledDim[no] -= 15;
    ledTickers[no].once_ms(20, offLed, no);
  }
}

void pumpAll(int time = bubbleTime){
  for(int i = 0;i<maxPumps;i++){
    onPump(i);
  }
}

void pumpOneByOne(int time = bubbleTime){
  for(int i = 0;i<maxPumps;i++){
    /*
    digitalWrite(valvePins[i],HIGH);
    delay(int(doc["valveOffsets"][i].as<int>()));
    digitalWrite(valvePins[i],LOW);*/
    onPump(i);
    delay(300);
  }
}


void pumpText(String text){
  int fontWidth = 4; // width of each character in the font   //4 is ASCII    8 is Chinese
  int fontHeight = 8; // height of each character in the font

  int customLength = 0;
  for (int i = 0; i < text.length(); i++) {
    if ((text[i] & 0xC0) != 0x80) {
      // Increment the count for the first byte of a UTF-8 character
      customLength++;
      
      if ((text[i] & 0xE0) == 0xE0) {
        // If the byte starts with '1110', it's a 3-byte UTF-8 character (like Chinese)
        // Increment the count again to treat it as double the length of an ASCII character
        customLength++;
      }
    }
  }

  int bitmapWidth = maxPumps;
  int bitmapHeight = fontHeight;
  GFXcanvas1 canvas(bitmapWidth, bitmapHeight);
  gfx.begin(canvas);
  // Draw text on bitmap
  int fw;
  gfx.setFont(pixelcorebb);
  fw = gfx.getUTF8Width(text.c_str());
  Serial.print("Width: ");
  Serial.println(fw);
  gfx.setCursor(int((maxPumps - fw) / 2), fontHeight-1);
  gfx.println(text);
   
  for (int y = 0; y < bitmapHeight; y++) {
    //if(y > 1 && y < 7){

      pumpNums = 0;
      for (int x = 0; x < bitmapWidth; x++) {
        int pixel = canvas.getPixel(x, y);
        if (pixel == 1) {
          pumpNums += 1;
        }
      }
      if(pumpNums > 0){
        for (int x = 0; x < bitmapWidth; x++) {
          int pixel = canvas.getPixel(x, y);
          if (pixel == 1) {
            Serial.print("⬜");
            onPump(x,multiply[pumpNums - 1]);
          }
          else {
            Serial.print("⬛");
          }
        }
        delay(lineTime);
      }
    //}    
    
    Serial.println("");
  }
}


void pumpBitmap(String base64Data){
  int bitmapWidth = 19;
  int bitmapHeight = 19;
  GFXcanvas1 canvas(bitmapWidth, bitmapHeight);

  size_t decode_len = TD_BASE64.getDecodeLength(base64Data);
  uint8_t decoded_data[decode_len];
  
  TD_BASE64.decode(base64Data, decoded_data);
  canvas.drawBitmap(0, 0, decoded_data, 19, 19, 0xffff);

  int pumpNums = 0; // Declare and initialize pumpNums

  for (int y = 0; y < bitmapHeight; y++) {
    pumpNums = 0;
    for (int x = 0; x < bitmapWidth; x++) {
      int pixel = canvas.getPixel(x, y);
      if (pixel == 1) {
        pumpNums += 1;
      }
    }
    //if (pumpNums > 0) {
      for (int x = 0; x < bitmapWidth; x++) {
        int pixel = canvas.getPixel(x, y);
        if (pixel == 1) {
          //Serial.print("⬜");
          onPump(x, multiply[pumpNums - 1]);
        } else {
          //Serial.print("⬛");
        }
      }
      //Serial.println("");
      delay(lineTime);
    //}
    
  }
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
float multiNum = 1;
void SecondCoreTaskFunction(void *pvParameters) {
  //pumpAll();
  while (true) {

    /*  DEBUG TESTING 用来检查每一个电磁阀有没有正常打开关闭*/

    //pumpOneByOne();
    //delay(5000);
    testText += 1;

    /* 按照template.json的内容显示东西 */
    for (JsonObject item : schedule) {
      const char* type = item["type"];  // Get the type
      const char* data = item["data"];  // Get the data
      if (item.containsKey("led")) {
        if(strcmp(item["led"], "rainbow") == 0){
          ledMode = "rainbow";
        }
        else{
          ledMode = "static";
          staticColor = strtoul(item["led"].as<String>().c_str(), NULL, 16);
        }
      }
      else{
        ledMode = doc["ledMode"].as<String>();
        staticColor = strtoul(doc["staticColor"].as<String>().c_str(), NULL, 16);
      }
      Serial.print("type: ");
      Serial.println(type);
      Serial.print("data: ");
      Serial.println(data);
      // Check the type and call the appropriate function
      if (strcmp(type, "text") == 0) {
        pumpText(data);
      } else if (strcmp(type, "bitmap") == 0) {
        pumpBitmap(data);
      }
      else if (strcmp(type, "time") == 0){
        timeClient.update();
        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();       
        // Format the time as "hours:minutes"
        String formattedTime = String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes);
        pumpText(formattedTime);
      }
      else if (strcmp(type, "json") == 0) {
          const char* url = item["data"]["url"];  // Get the URL
          String target = item["data"]["target"];  // Get the JSON target field

          // Perform an HTTP request to the URL
          HTTPClient http;
          http.begin(url);

          int httpCode = http.GET();
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
              String response = http.getString();

              // Parse the JSON response
              DynamicJsonDocument doc(1024);  // Adjust the size as needed
              DeserializationError error = deserializeJson(doc, response);

              if (error) {
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
              } else {
                // Use the "target" string to navigate to the desired value in the JSON document.
                // For example, if "target" is "[result][data][nums]", you can access it as follows:
                JsonVariant nested = doc;

                // Split the key into components
                String delimiter = ".";
                int pos = 0;
                String component;

                while (!target.isEmpty()) {
                    pos = target.indexOf(delimiter);
                    if (pos == -1) {
                        component = target;
                        target = "";  // Clear the key to exit the loop
                    } else {
                        component = target.substring(0, pos);
                        target = target.substring(pos + delimiter.length());
                    }
                    nested = nested[component];
                }
                
                String extractedValue = nested.as<String>();
                
                // Now you have the extracted value, and you can do whatever you need with it.
                //Serial.print("Extracted Value: ");
                //Serial.println(extractedValue);

                pumpText(extractedValue);
              }
            } else {
              Serial.print("HTTP request failed with error code: ");
              Serial.println(httpCode);
            }
          } else {
            Serial.println("HTTP request failed");
          }

          http.end();  // Close the HTTP connection
        }
      delay(3000);
    }
    
  }
}