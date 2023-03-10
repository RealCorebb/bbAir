#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#define FORMAT_LITTLEFS_IF_FAILED true
//Create a ArduinoJson object with dynamic memory allocation
DynamicJsonDocument doc(1024);

void initConfig() {
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
      Serial.println("LittleFS Mount Failed");
      return;
  }
  //Open the SPIFFS file system
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
    //Create a config.json if it doesn't exist
    if (!LittleFS.exists("/config.json")) {
    //Serial.println("Creating config file");
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to create config file");
    }
    //default Data
    JsonArray schedule = doc.createNestedArray("schedule");
    JsonObject textObject = schedule.createNestedObject();
    textObject["type"] = "text";
    textObject["data"] = "Hello";

    JsonObject shapeObject = schedule.createNestedObject();
    shapeObject["type"] = "shape";
    JsonArray shapeData = shapeObject.createNestedArray("data");
    shapeData.add(559240);
    shapeData.add(279620);
    //Create a JSON object with the data
    JsonObject obj = doc.to<JsonObject>();
    obj["pump"] = true;
    obj["led"] = true;
    //Write the JSON object to the config.json file
        if (serializeJson(doc, configFile) == 0) {
            Serial.println("Failed to write to file");
        }
    }
    else {
    //Serial.println("Config file exists");
    //Open the config.json file
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
        //Serial.println("Reading config file");
        //Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, configFile);
        if (error) {
        Serial.println("Failed to read config file, using default configuration");
        }
    }
    }
}

void pumpShape(const JsonArray& shapeData){
  //loop shapeData
    for (int i = 0; i < shapeData.size(); i++) {
      int shapeDecimal = shapeData[i];
      int num_bits = 0;
      int temp = shapeDecimal;
      while (temp > 0) {
          num_bits++;
          temp >>= 1;
      }
      for (int i = 0; i < num_bits; i++) {
        int pixel = bitRead(shapeDecimal, i);
        if (pixel == 1) {
          Serial.print("⬜");
        }
        else Serial.print("⬛");
      }
      delay(bubbleTime);
    }
}

// LED   -_,-
#define PIN       41
#define NUMPIXELS 19
int brightness = 50;
int ledSpeed = 10;
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setupLED(){
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
}

void ledLoop(){
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    strip.rainbow(firstPixelHue);
    strip.show(); // Update strip with new contents
    delay(ledSpeed);  // Pause for a moment
  }
}