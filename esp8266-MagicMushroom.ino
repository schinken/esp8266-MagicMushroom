#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include <ArduinoJson.h>
#include <vector>

#include "StaticResponse.h"
#include "config.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BUTTON_PIN D5
#define MAX_SOUNDS 10
#define MAX_FILENAME_LENGTH 100

char convertBuffer[MAX_FILENAME_LENGTH];

WiFiClient wifiClient;
PubSubClient mqttClient;
ESP8266WebServer webServer(80);

Bounce debouncer = Bounce();

uint8_t currentSoundIndex = 0;
std::vector<String> sounds;

void bootstrapWebServer(void);

void setup() {

  WiFi.hostname("ESP-MagicMushroom");
  WiFi.mode(WIFI_STA);

  sounds.push_back("funes-nein.ogg");
  sounds.push_back("funes-doch.ogg");
  sounds.push_back("funes-oh.ogg");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  debouncer.attach(BUTTON_PIN);
  debouncer.interval(100);

  bootstrapWebServer();
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


void bootstrapWebServer() {
  webServer.on("/", handleRoot);
  webServer.on("/files", handleFiles);
  webServer.on("/add", handleAdd);
  webServer.on("/delete", handleDelete);
  
  webServer.onNotFound(handleNotFound);
  
  webServer.begin();
}

void connectMqtt() {

  while (!mqttClient.connected()) {
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(mqttHost, 1883);
    mqttClient.connect("magicPushButton");
    
    delay(1000);
  }
}


void loop() {
  
  connectMqtt();
  debouncer.update();
  mqttClient.loop();
  webServer.handleClient();
 
  if(debouncer.fell() && sounds.size() > 0) {
    currentSoundIndex = currentSoundIndex % sounds.size();
    sounds[currentSoundIndex].toCharArray(convertBuffer, MAX_FILENAME_LENGTH);
    
    mqttClient.publish("psa/sound", convertBuffer);

    currentSoundIndex++;
  }
}
