#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>  // Include DHT library

// Update these with values suitable for your network.
const char* ssid = "vivo Y18";
const char* password = "00000000";
const char* mqtt_server = "5.196.78.28"; // broker.hivemq.com test.mosquitto.org

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// Define the min and max values from your soil moisture sensor.
const int sensorMin = 800;   // Value when the sensor is in dry air
const int sensorMax = 200;   // Value when the sensor is fully submerged in water

// Define the GPIO pin for the relay and DHT11 sensor
const int relayPin = 19;
const int dhtPin = 18;
const int soilSensorPin = 34;  // ESP32 has ADC pins starting from GPIO34

// Set the moisture threshold (in percentage)
const int moistureThreshold = 30;

// Define manual control flag (false = automatic, true = manual)
bool manualMode = false;

// Current pump state (false = OFF, true = ON)
bool pumpState = false;

// Initialize DHT11
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Manual/Automatic control switch (topic: "device/mode")
  if (strcmp(topic, "device/mode") == 0) {
    if ((char)payload[0] == '1') {
      manualMode = true;
      Serial.println("Switched to manual mode");
    } else {
      manualMode = false;
      Serial.println("Switched to automatic mode");
    }
  }

  // Control the water pump manually (topic: "device/pump") only in manual mode
  if (strcmp(topic, "device/pump") == 0 && manualMode) {
    if ((char)payload[0] == '1') {
      Serial.println("Manual: Turning ON the water pump...");
      digitalWrite(relayPin, LOW);  // Activate the relay (assuming active LOW)
      pumpState = true;
    } else {
      Serial.println("Manual: Turning OFF the water pump...");
      digitalWrite(relayPin, HIGH); // Deactivate the relay
      pumpState = false;
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("device/soil", "MQTT Server is Connected");
      client.subscribe("device/mode");  // Subscribe to control mode (manual/automatic)
      client.subscribe("device/pump");  // Subscribe to manually control the pump
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(relayPin, OUTPUT);     // Relay control pin
  digitalWrite(relayPin, HIGH);  // Initialize relay to OFF (assuming active LOW)

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  dht.begin();  // Initialize DHT11 sensor
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // Read the raw analog value from the soil moisture sensor
    value = analogRead(soilSensorPin);  // ESP32 uses analogRead on pins like GPIO34
    Serial.print("Raw analog reading: ");
    Serial.println(value);

    // Map the value to percentage (0% = dry, 100% = wet)
    int soilMoisturePercent = map(value, sensorMin, sensorMax, 0, 100);

    // Constrain the value to ensure it stays within 0-100%
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    // Publish the soil moisture percentage as a simple number
    snprintf(msg, MSG_BUFFER_SIZE, "%d", soilMoisturePercent);  // Simple number, not text
    client.publish("device/soil", msg);
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisturePercent);

    // Automatic pump control based on soil moisture (if not in manual mode)
    if (!manualMode) {
      if (soilMoisturePercent < moistureThreshold) {
        Serial.println("Automatic: Soil is dry, turning on the water pump...");
        digitalWrite(relayPin, LOW);  // Activate the relay (assuming active LOW)
        pumpState = true;
      } else {
        Serial.println("Automatic: Soil is wet enough, turning off the water pump...");
        digitalWrite(relayPin, HIGH); // Deactivate the relay
        pumpState = false;
      }
    }

    // Read temperature and humidity from DHT11 sensor
    float temperature = dht.readTemperature();  // Read temperature in Celsius
    float humidity = dht.readHumidity();        // Read humidity

    // Check if readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      // Publish temperature and humidity data as simple numbers
      snprintf(msg, MSG_BUFFER_SIZE, "%.2f", temperature);
      client.publish("device/temperature", msg);
      Serial.print("Temperature: ");
      Serial.println(temperature);

      snprintf(msg, MSG_BUFFER_SIZE, "%.2f", humidity);
      client.publish("device/humidity", msg);
      Serial.print("Humidity: ");
      Serial.println(humidity);
    }
  }
}
