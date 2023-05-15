#define FORMAT_LITTLEFS_IF_FAILED true
//Create a ArduinoJson object with dynamic memory allocation

void saveJson(){
  File configFile = LittleFS.open("/config.json", "w");
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to file");
  }
  configFile.close();
}

void initConfig() {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Check if config.json file exists
  if (!LittleFS.exists("/config.json")) {
    Serial.println("Creating config file");
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Failed to create config file");
      return;
    }
    configFile.close();
  }

  // Read the config.json file
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }
  
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to read config file, using default configuration");
  }

  configFile.close();

  // Check if any keys are missing and add default data if necessary
  if (!doc.containsKey("pump")) {
    doc["pump"] = true;
  }
  if (!doc.containsKey("led")) {
    doc["led"] = true;
  }
  if (!doc.containsKey("schedule")) {
    // Add default data
    JsonArray schedule = doc.createNestedArray("schedule");
    JsonObject textObject = schedule.createNestedObject();
    textObject["type"] = "text";
    textObject["data"] = "Hello";

    JsonObject shapeObject = schedule.createNestedObject();
    shapeObject["type"] = "shape";
    JsonArray shapeData = shapeObject.createNestedArray("data");
    shapeData.add(559240);
    shapeData.add(279620);
  }
  if (!doc.containsKey("valveOffsets")) {
    JsonArray valveOffsets = doc.createNestedArray("valveOffsets");
    for (int i = 0; i < 20; i++) {
      valveOffsets.add(100);
    }
  }
  saveJson();
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

void ledLoop(void *pvParameters){
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    strip.rainbow(firstPixelHue);
    strip.show(); // Update strip with new contents
    delay(ledSpeed);  // Pause for a moment
  }
}

bool lastOpen = false;
void stressLoop(void *pvParameters){
    //0 MPA    3770
    //0.02 MPA
    //0.04 MPA  3830
    //0.08MPA 3870
    /*
    while(true){
      int sensorValue = analogRead(Stress);
      input = sensorValue;
      myPID.Compute();
      int pumpSpeed = int(output);
      if(pumpSpeed < 60) pumpSpeed = 0;
      analogWrite(OUTAir, pumpSpeed);   // Set PWM output
      Serial.print(sensorValue);
      Serial.print(",");
      Serial.println(pumpSpeed);
      delay(5);
    }*/
    while(true){
      sensorValue = analogRead(Stress);
        smoothedValue = alpha * smoothedValue + (1 - alpha) * sensorValue;
        if (smoothedValue >= 3830) {
            digitalWrite(OUTAir, LOW);
            lastOpen = false;           
        } else {
          if(!lastOpen){
            digitalWrite(OUTAir, HIGH);
            lastOpen = true;
          }
          else{
            digitalWrite(OUTAir, LOW);
            lastOpen = false;
          }
        }
        delay(25);
    }
}
void setupWeb(){
  server.serveStatic("/", LittleFS, "/");
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  server.on("/valveOffsets", HTTP_POST, [](AsyncWebServerRequest *request){
    // Get the value of the "valveOffsets" parameter from the request body
    String jsonString;
    if(request->hasParam("valveOffsets", true)) { // Check if parameter exists and is not empty
      jsonString = request->getParam("valveOffsets", true)->value();
      Serial.println(jsonString);
    }

    // Update the valveOffsets array in the global DynamicJsonDocument
    if (doc.containsKey("valveOffsets")) {
        const size_t CAPACITY = JSON_ARRAY_SIZE(20);
        DynamicJsonDocument newData(CAPACITY);
        DeserializationError error = deserializeJson(newData, jsonString);
        if (error) { // Check for deserialization errors
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            request->send(400, "Bad Request");
            return;
        }
        JsonArray array = newData.as<JsonArray>();
        doc["valveOffsets"].set(array);
    }

    request->send(200, "OK");
});


  server.begin();
}
