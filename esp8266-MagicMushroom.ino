#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <ArduinoJson.h>
#include <vector>

#include "StaticResponse.h"
#include "settings.h"

char convertBuffer[MAX_FILENAME_LENGTH];

WiFiClient wifiClient;
PubSubClient mqttClient;
ESP8266WebServer webServer(80);

Bounce debouncer = Bounce();

uint8_t currentSoundIndex = 0;
std::vector<String> sounds;

void bootstrapWebServer(void);

void setup() {

  Serial.begin(115200);
  delay(10);

  EEPROM.begin(EEPROM_SIZE);
  
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);

  eepromRead();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, 1883);
  
  ArduinoOTA.setHostname(WIFI_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();

  debouncer.attach(BUTTON_PIN);
  debouncer.interval(100);

  bootstrapWebServer();
}

void eepromRead() {

  if (EEPROM.read(EEPROM_IDX_DIRTY) != 0x42) {    
    for(uint16_t i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(i, 0x00);
    }
    
    EEPROM.write(EEPROM_IDX_DIRTY, 0x42);
    EEPROM.commit();
  }
  
  uint8_t numSounds = EEPROM.read(EEPROM_IDX_NUM_SOUNDS);
  if (numSounds == 0) {
    return;
  }
  
  sounds.clear();

  char filename[MAX_FILENAME_LENGTH];
  for (uint8_t i = 0; i < numSounds; i++) {
    uint16_t address = (EEPROM_SOUNDS + (i * MAX_FILENAME_LENGTH)) + 1;    
    EEPROM.get(address, filename);
    
    sounds.push_back(String(filename));
  }
}

void eepromWrite() {

  uint8_t numSounds = (uint8_t) sounds.size();
  if (numSounds == 0) {
    return;  
  }

  EEPROM.write(EEPROM_IDX_NUM_SOUNDS, numSounds);

  char filename[MAX_FILENAME_LENGTH];
  for (uint8_t i = 0; i < numSounds; i++) {
    uint16_t address = (EEPROM_SOUNDS + (i * MAX_FILENAME_LENGTH)) + 1;
    sounds[i].toCharArray(filename, sizeof(filename));
    
    EEPROM.put(address, filename);
  }
  
  EEPROM.commit();
}

void handleRoot() {
  webServer.send_P(200, TEXT_HTML, STATIC_INDEX_HTML);
}

void handleNotFound() {
  webServer.send_P(404, TEXT_HTML, STATIC_404_HTML);
}

void handleFiles() {
  DynamicJsonBuffer jsonBuffer;
  JsonArray& array = jsonBuffer.createArray();

  uint8_t index = 0;
  for(auto sound : sounds) {
    JsonObject& jsonSound = jsonBuffer.createObject();
    jsonSound["index"] = index++;
    jsonSound["filename"] = sound;
    
    array.add(jsonSound);
  }

  String response = "";
  array.printTo(response);

  webServer.send(200, "application/json", response);
}

void handleAdd() {
  String filename = webServer.arg("filename");

  if(sounds.size() == MAX_SOUNDS) {
    return webServer.send(400, "text/plain", "Maximum number of sounds reached."); 
  }
  
  if(filename.length() == 0) {
    return webServer.send(400, "text/plain", "Filename is empty");    
  }
  
  if(filename.length() > MAX_FILENAME_LENGTH) {
    return webServer.send(413, "text/plain", "Filename is too long");    
  }

  currentSoundIndex = 0;
  
  sounds.push_back(filename);

  webServer.sendHeader("Location", "/");
  webServer.send(301, "text/plain", "Sound added"); 
}

void handleDelete() {
  uint8_t index = webServer.arg("index").toInt();
  
  if(index >= sounds.size()) {
    return webServer.send(400, "text/plain", "Index out of range");  
  }

  currentSoundIndex = 0;
  
  sounds.erase(sounds.begin() + index);

  webServer.sendHeader("Location", "/");
  webServer.send(301, "text/plain", "Sound deleted"); 
}

void handleStore() {
  eepromWrite();
  
  webServer.sendHeader("Location", "/");
  webServer.send(301, "text/plain", "Stored to EEPROM"); 
}

void handleLoad() {
  eepromRead();
  
  webServer.sendHeader("Location", "/");
  webServer.send(301, "text/plain", "Loaded from EEPROM"); 
}

void bootstrapWebServer() {
  webServer.on("/", handleRoot);

  webServer.on("/store", handleStore);
  webServer.on("/load", handleLoad);
 
  webServer.on("/files", handleFiles);
  webServer.on("/add", handleAdd);
  webServer.on("/delete", handleDelete);
  
  webServer.onNotFound(handleNotFound);
  
  webServer.begin();
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(WIFI_HOSTNAME, MQTT_TOPIC_STATE, 1, true, "disconnected")) {
      mqttClient.publish(MQTT_TOPIC_STATE, "connected", true);
    } else {
      Serial.println("MQTT connect failed!");
      delay(1000);
    }
  }
}


void loop() {
 
  if(debouncer.fell() && sounds.size() > 0) {
    currentSoundIndex = currentSoundIndex % sounds.size();
    sounds[currentSoundIndex].toCharArray(convertBuffer, MAX_FILENAME_LENGTH);
    
    mqttClient.publish(MQTT_TOPIC_SOUND, convertBuffer);

    currentSoundIndex++;
  }
    
  connectMqtt();
  debouncer.update();
  mqttClient.loop();
  webServer.handleClient();
  ArduinoOTA.handle();
}
