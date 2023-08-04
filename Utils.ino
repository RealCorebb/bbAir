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
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setupLED() {
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
}


int lowTimes = 0;

void ledLoop(void *pvParameters){
  while (true) {
    colorWipe(strip.Color(255,   0,   0), 50); // Red
    colorWipe(strip.Color(  0, 255,   0), 50); // Green
    colorWipe(strip.Color(  0,   0, 255), 50); // Blue

    // Do a theater marquee effect in various colors...
    theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness
    theaterChase(strip.Color(127,   0,   0), 50); // Red, half brightness
    theaterChase(strip.Color(  0,   0, 127), 50); // Blue, half brightness

    rainbow(10);             // Flowing rainbow cycle along the whole strip
    theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant
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
    Serial.println(sensorValue);
    smoothedValue = alpha * smoothedValue + (1 - alpha) * sensorValue;
    if (smoothedValue >= 3830) {
      lowTimes = 0;
      digitalWrite(OUTAir,LOW);
      //analogWrite(OUTAir,0);
    } else {
      lowTimes++;
      if(lowTimes >= 8){
        digitalWrite(OUTAir,HIGH);
        //analogWrite(OUTAir,150);
      }
    }
    delay(10);
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
      JsonArray array = newData.as<JsonArray>();
      doc["valveOffsets"].set(array);
      saveJson();
    }
    request->send(200, "OK");
  });

   server.on("/valve", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("valveNumber")) {
      int valveNumber = request->getParam("valveNumber")->value().toInt();
      if (valveNumber >= 1 && valveNumber <= 20) {
        if (request->hasParam("state")) {
          String state = request->getParam("state")->value();
          if (state.equalsIgnoreCase("on")) {
            digitalWrite(valvePins[valveNumber - 1], HIGH);
            request->send(200, "text/plain", "Valve turned ON");
          } else if (state.equalsIgnoreCase("off")) {
            digitalWrite(valvePins[valveNumber - 1], LOW);
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


  server.begin();
}


//LED
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    strip.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

