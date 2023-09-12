#define FORMAT_LITTLEFS_IF_FAILED true
//Create a ArduinoJson object with dynamic memory allocation

void saveJson() {
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
  schedule = doc["schedule"];
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

    JsonObject bitmapObject = schedule.createNestedObject();
    bitmapObject["type"] = "bitmap";
    textObject["data"] = "Qk02AwAAAAAAADYAAAAoAAAAEAAAABAAAAABABgAAAAAAAAAAADDDgAAww4AAAAAAAAAAAAA////////////////////////////////////////////////////////////////////AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA////////AAAAAAAA////////////////////////////////////////AAAAAAAA////////AAAAAAAA////////////////////////////////////////AAAAAAAA////////AAAAAAAA////////AAAAAAAAAAAAAAAAAAAAAAAA////////AAAAAAAA////////AAAAAAAA////////AAAAAAAAAAAAAAAAAAAAAAAA////////AAAAAAAA////////AAAAAAAAAAAAAAAAAAAAAAAA////////AAAAAAAAAAAAAAAAAAAAAAAA////////AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA////////AAAA////////////////////AAAAAAAA////////////////////AAAA////////AAAA////////AAAAAAAA////AAAAAAAA////AAAAAAAA////////AAAA////////AAAA////////////////////AAAAAAAA////////////////////AAAA////////AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA////////////////AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////";
  }
  if (!doc.containsKey("valveOffsets")) {
    JsonArray valveOffsets = doc.createNestedArray("valveOffsets");
    for (int i = 0; i < 20; i++) {
      valveOffsets.add(50);
    }
  }
  saveJson();
}


void pumpShape(const JsonArray &shapeData) {
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
      } else Serial.print("⬛");
    }
    delay(bubbleTime);
  }
}

// LED   -_,-
#define PIN 42
#define NUMPIXELS 19
int brightness = 255;
int ledSpeed = 10;
NeoPixelBusLg<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> strip(NUMPIXELS, PIN);

void setupLED() {
  strip.Begin();
  strip.Show();
  strip.SetLuminance(brightness);
}


int lowTimes = 0;

void ledLoop(void *pvParameters){
  while (true) {
    //Serial.println("led");
    rainbowLoop();
    //staticLoop();
  }
}

void stressLoop(void *pvParameters) {
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
  while (true) {
    sensorValue = analogRead(Stress);
    //Serial.println(sensorValue);
    WebSerial.println(sensorValue);
    smoothedValue = alpha * smoothedValue + (1 - alpha) * sensorValue;
    if (smoothedValue >= 200) {
      lowTimes = 0;
      digitalWrite(OUTAir,LOW);
      //analogWrite(OUTAir,0);
    } else {
      lowTimes++;
      if(lowTimes >= 3){
        digitalWrite(OUTAir,HIGH);
        //analogWrite(OUTAir,150);
      }
    }
    delay(100);
  }
}
void setupWeb() {
  server.serveStatic("/", LittleFS, "/");
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  server.on("/valveOffsets", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Get the value of the "valveOffsets" parameter from the request body
    String jsonString;
    if (request->hasParam("valveOffsets", true)) {  // Check if parameter exists and is not empty
      jsonString = request->getParam("valveOffsets", true)->value();
      Serial.println(jsonString);
    }

    // Update the valveOffsets array in the global DynamicJsonDocument
    if (doc.containsKey("valveOffsets")) {
      const size_t CAPACITY = JSON_ARRAY_SIZE(20);
      DynamicJsonDocument newData(CAPACITY);
      DeserializationError error = deserializeJson(newData, jsonString);
      if (error) {  // Check for deserialization errors
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        request->send(400, "Bad Request");
        return;
      }
      JsonArray newArray = newData.as<JsonArray>();
      
      // Clear the existing array and copy new data
      JsonArray existingArray = doc["valveOffsets"].as<JsonArray>();
      existingArray.clear();
      for (JsonVariant v : newArray) {
        existingArray.add(v);
      }
      
      saveJson(); // Save the modified JSON
    }
    request->send(200, "OK");
  });

   server.on("/valve", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("valveNumber")) {
      int valveNumber = request->getParam("valveNumber")->value().toInt();
      if (valveNumber >= 1 && valveNumber <= 20) {
        if (request->hasParam("state")) {
          String state = request->getParam("state")->value();
          if (state.equalsIgnoreCase("pump")) {
            onPump(valveNumber - 1);
            request->send(200, "text/plain", "Valve pump");
          }
          else if (state.equalsIgnoreCase("on")) {
            digitalWrite(valvePins[valveNumber - 1], HIGH);
            request->send(200, "text/plain", "Valve turned ON");
          } else if (state.equalsIgnoreCase("off")) {
            digitalWrite(valvePins[valveNumber - 1], HIGH);
            request->send(200, "text/plain", "Valve turned OFF");
          } else {
            request->send(400, "text/plain", "Invalid state parameter");
          }
        } else {
          request->send(400, "text/plain", "Missing state parameter");
        }
      } else {
        request->send(400, "text/plain", "Invalid valve number");
      }
    } else {
      request->send(400, "text/plain", "Missing valveNumber parameter");
    }
  });

  server.on("/updateConfig", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Read the JSON data as a string
    if (request->hasParam("config", true)) {  
      Serial.println("getting");
      String updatedConfig = request->getParam("config", true)->value();
      Serial.println("got");
      // Open the config.json file and update its content
      Serial.print(updatedConfig);
      File configFile = LittleFS.open("/config.json", "w");
      if (configFile) {
        configFile.print(updatedConfig);
        configFile.close();
        
        // Reinitialize the configuration
        initConfig();
        
        request->send(200, "text/plain", "Config updated and initialized");
      } else {
        request->send(500, "text/plain", "Failed to update config file");
      }
    }
  });

  server.begin();
}


//LED
uint32_t staticColor = 0x89CFF0;

void rainbowLoop() {
  static float startIndex = 0;
  

  for (int i = 0; i < NUMPIXELS; i++) {
    float hue = startIndex - (float(i) / (NUMPIXELS * 2));
    RgbColor color = HslColor(hue, 1.0f, 0.5f);
    color = color.Dim(ledDim[i]);
    strip.SetPixelColor(i, color);
  }
  startIndex = startIndex + 0.005;
  if(startIndex > 1) startIndex = 0;

  strip.Show();
  delay(10);  // Delay to control the speed of the loop
}

void staticLoop(){
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    RgbColor color = HtmlColor(staticColor);
    color = color.Dim(ledDim[i]);
    strip.SetPixelColor(i, color);
  }
  strip.Show();
  delay(10);
}