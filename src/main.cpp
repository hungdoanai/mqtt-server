#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include "time.h"

//================= SENSOR =================
ClosedCube_HDC1080 hdc1080;
#define SOIL_PIN 34

//================= RELAY =================
#define FAN_PIN 26
#define PUMP_PIN 27

//================= WIFI =================
const char* ssid = "Phong so 2";
const char* password = "hoilamgi";

//================= MQTT =================
const char* mqtt_server = "4477cf883375485e9156af4ed6ab121b.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "hung00";
const char* mqtt_password = "0565241132aA";

WiFiClientSecure espClient;
PubSubClient client(espClient);

//================= NTP =================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // VN
const int   daylightOffset_sec = 0;

//================= BUFFER =================
#define MAX_BUFFER 20
String bufferQueue[MAX_BUFFER];
int bufferCount = 0;

//================= TIME =================
unsigned long timeUpdate = 0;

//================= WIFI =================
void setup_wifi() {
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
}

//================= TIME =================
void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("Đang lấy thời gian...");
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    Serial.println("Chưa có thời gian...");
    delay(1000);
  }

  Serial.println("Đã đồng bộ thời gian!");
}

//================= MQTT =================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");

    String clientID = "ESP32-";
    clientID += String(random(0xffff), HEX);

    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

      client.subscribe("esp32/control");

      // 🔥 resend buffer
      for (int i = 0; i < bufferCount; i++) {
        client.publish("esp32/sensor", bufferQueue[i].c_str(), true);
        Serial.println("Resend: " + bufferQueue[i]);
      }

      bufferCount = 0;
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

//================= CALLBACK =================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  DynamicJsonDocument doc(128);
  deserializeJson(doc, msg);

  String device = doc["device"];
  bool state = doc["state"];

  if (device == "fan") digitalWrite(FAN_PIN, state);
  if (device == "pump") digitalWrite(PUMP_PIN, state);
}

//================= BUFFER =================
void saveToBuffer(String data) {
  if (bufferCount < MAX_BUFFER) {
    bufferQueue[bufferCount++] = data;
  } else {
    for (int i = 1; i < MAX_BUFFER; i++) {
      bufferQueue[i - 1] = bufferQueue[i];
    }
    bufferQueue[MAX_BUFFER - 1] = data;
  }
}

//================= SETUP =================
void setup() {
  Serial.begin(9600);

  Wire.begin(32, 33);
  hdc1080.begin(0x40);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);

  setup_wifi();
  setupTime(); // 🔥 lấy giờ thật

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

//================= LOOP =================
void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - timeUpdate > 60000) {

    float temperature = hdc1080.readTemperature();
    float humidity = hdc1080.readHumidity();
    int soil_raw = analogRead(SOIL_PIN);
    int soil = map(soil_raw, 4095, 0, 0, 100);

    // 🔥 LẤY GIỜ THẬT
    struct tm timeinfo;
    char timeString[30];

    if (getLocalTime(&timeinfo)) {
      strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
      strcpy(timeString, "no-time");
    }

    // 🔥 JSON
    DynamicJsonDocument doc(256);
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["soil"] = soil;
    doc["time"] = timeString;

    char buffer[128];
    serializeJson(doc, buffer);

    String payload = String(buffer);

    // 🔥 gửi hoặc lưu buffer
    if (client.connected()) {
      if (client.publish("esp32/sensor", payload.c_str(), true)) {
        Serial.println("Sent: " + payload);
      } else {
        Serial.println("Publish fail → save buffer");
        saveToBuffer(payload);
      }
    } else {
      Serial.println("MQTT mất → save buffer");
      saveToBuffer(payload);
    }

    timeUpdate = millis();
  }
}