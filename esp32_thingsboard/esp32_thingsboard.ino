#include <DHTesp.h>
#include <WiFi.h>
#include <ThingsBoard.h>
#include <ArduinoJson.h>
#include <Arduino_MQTT_Client.h>

#define pinDht 15
DHTesp dhtSensor;

#define WIFI_AP "vivo Y18"
#define WIFI_PASS "00000000"

#define TB_SERVER "thingsboard.cloud"
#define TOKEN "CG201SON7aLujWRwsccB" 

constexpr uint16_t MAX_MESSAGE_SIZE = 128U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi.");
  } else {
    Serial.println("\nConnected to WiFi");
  }
}

void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");
    
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(float temp, int hum) {
  DynamicJsonDocument jsonDoc(128);
  jsonDoc["temperature"] = temp;
  jsonDoc["humidity"] = hum;

  if (!tb.sendTelemetryJson(jsonDoc, measureJson(jsonDoc))) {
    Serial.println("Failed to send data");
  } else {
    Serial.println("Data sent");
  }
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(pinDht, DHTesp::DHT11);
  connectToWiFi();
  connectToThingsBoard();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
    }

    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    float temp = data.temperature;
    int hum = data.humidity;

    if (isnan(temp) || isnan(hum)) {
        Serial.println("Failed to read from DHT sensor!");
        delay(3000);
        return;
    }

    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);

    if (!tb.connected()) {
        connectToThingsBoard();
    }

    sendDataToThingsBoard(temp, hum);

    delay(3000);
    tb.loop();
}
