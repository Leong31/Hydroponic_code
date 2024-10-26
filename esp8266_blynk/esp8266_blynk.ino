#define BLYNK_TEMPLATE_NAME "AUTOMATIC PLANT WATERING SYSTEM"
#define BLYNK_TEMPLATE_ID "TMPL6iwT3eKfU"

// Include the library files
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>  // Add the DHT library

char auth[] = "AUTH_TOKEN"; // Enter your Auth token
char ssid[] = "WIFI_SSID";                         // Enter your WIFI name
char pass[] = "WIFI_PASSWORD";                         // Enter your WIFI password

BlynkTimer timer;
bool Relay = 0;

// Define component pins
#define sensor A0        // Soil moisture sensor pin
#define waterPump D1     // Water pump control pin
#define DHTPIN D2        // DHT11 sensor connected to GPIO D2
#define DHTTYPE DHT11    // Define DHT type as DHT11

DHT dht(DHTPIN, DHTTYPE);  // Initialize the DHT sensor

// Set the soil moisture threshold (adjust this value based on your sensor's calibration)
#define THRESHOLD_MOISTURE 30  // For example, 30%

bool pumpAutomaticallyOn = false;  // To track if the pump was automatically turned on

void setup() {
  Serial.begin(9600);
  pinMode(waterPump, OUTPUT);
  digitalWrite(waterPump, HIGH);  // Initially turn off the pump (HIGH = off for active LOW relay)

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  
  dht.begin();  // Initialize the DHT sensor

  // Call the function to check soil moisture and DHT11 every 3 seconds
  timer.setInterval(3000L, soilMoistureSensor);
  timer.setInterval(5000L, sendDHTData);  // Call DHT11 reading every 5 seconds
}

// Get the button value from Blynk app (manual control)
BLYNK_WRITE(V1) {
  Relay = param.asInt();

  if (Relay == 1) {
    digitalWrite(waterPump, LOW);  // Turn on the pump (active LOW)
    pumpAutomaticallyOn = false;   // Manual override, reset automatic pump state
  } else {
    digitalWrite(waterPump, HIGH); // Turn off the pump
  }
}

// Function to check soil moisture and control the pump automatically
void soilMoistureSensor() {
  int value = analogRead(sensor);              // Read the soil moisture sensor value
  value = map(value, 0, 1024, 0, 100);         // Map sensor value to percentage (0-100%)
  value = (value - 100) * -1;                  // Adjust the value so that dry = 0%, wet = 100%
  
  Blynk.virtualWrite(V0, value);               // Send soil moisture percentage to Blynk

  // Automatic control based on soil moisture level
  if (value < THRESHOLD_MOISTURE && !pumpAutomaticallyOn) {
    // If soil moisture is below the threshold and the pump is not manually turned on
    digitalWrite(waterPump, LOW);              // Turn on the pump (active LOW)
    pumpAutomaticallyOn = true;                // Set the flag indicating automatic pump control

    // Send alert to Blynk app
    Blynk.logEvent("moisture_alert", "Soil moisture is below the threshold!");
    Serial.println("Soil moisture is below the threshold! Pump turned ON automatically.");
  } else if (value >= THRESHOLD_MOISTURE && pumpAutomaticallyOn) {
    // If soil moisture is above the threshold and the pump was turned on automatically
    digitalWrite(waterPump, HIGH);             // Turn off the pump
    pumpAutomaticallyOn = false;               // Reset the automatic pump state
    Serial.println("Soil moisture is sufficient. Pump turned OFF.");
  }
}

// Function to read DHT11 data and send it to Blynk
void sendDHTData() {
  float humidity = dht.readHumidity();          // Read humidity
  float temperature = dht.readTemperature();    // Read temperature in Celsius

  // Check if any reads failed and exit early (to try again)
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Send the temperature and humidity data to Blynk
  Blynk.virtualWrite(V2, temperature);  // V2 for temperature
  Blynk.virtualWrite(V3, humidity);     // V3 for humidity

  // Print the data to the serial monitor for debugging
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void loop() {
  Blynk.run();   // Run the Blynk library
  timer.run();   // Run the Blynk timer
}
