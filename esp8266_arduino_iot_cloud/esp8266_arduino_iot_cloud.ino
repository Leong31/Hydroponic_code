#include "thingProperties.h"

const int soilMoisturePin = A0; // Analog input pin for soil moisture sensor
//const int pumpPin = 2;  

#define pumpPin D1         // Digital output pin to control the relay module

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize the pins
  pinMode(soilMoisturePin, INPUT);
  pinMode(pumpPin, OUTPUT);
  
  // Initialize the properties
  initProperties();
  
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  // Set the debug message level
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  // Update Arduino IoT Cloud
  ArduinoCloud.update();
  
  // Read soil moisture level
  int rawMoistureLevel = analogRead(soilMoisturePin);
  
  // Print raw moisture level to Serial Monitor
  Serial.print("Raw Soil Moisture Level: ");
  Serial.println(rawMoistureLevel);
  
  // Convert the analog reading (which goes from 0 - 1023) to a percentage (0 - 100)
  int moistureLevelValue = map(rawMoistureLevel, 0, 1023, 100, 0);
  
  // Print mapped moisture level to Serial Monitor
  Serial.print("Mapped Soil Moisture Level: ");
  Serial.println(moistureLevelValue);
  
  // Update the cloud variable
  moistureLevel = moistureLevelValue;
  
  // Control the pump based on the mode
  if (manualMode) {
    // Manual mode: Control the pump based on manualPumpControl variable
    if (manualPumpControl) {
      digitalWrite(pumpPin, HIGH); // Turn on the pump
      pumpState = true;
    } else {
      digitalWrite(pumpPin, LOW);  // Turn off the pump
      pumpState = false;
    }
  } else {
    // Automatic mode: Control the pump based on soil moisture level
    if (moistureLevelValue < 50) {
      digitalWrite(pumpPin, HIGH); // Turn on the pump
      pumpState = true;
    } else {
      digitalWrite(pumpPin, LOW);  // Turn off the pump
      pumpState = false;
    }
  }
  
  // Small delay to prevent excessive reading
  delay(1000);
}

// Arduino IoT Cloud functions
void onMoistureLevelChange() {
  // This function is triggered when the moisture level changes
}

void onPumpStateChange() {
  // This function is triggered when the pump state changes
}

void onManualPumpControlChange() {
  // This function is triggered when the manual pump control changes
  manualMode = true; // Enable manual mode when manual control is used
}

void onManualModeChange() {
  // This function is triggered when the manual mode changes
}
