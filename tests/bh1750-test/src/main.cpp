#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

#define SDA_PIN 1   // GPIO1
#define SCL_PIN 2   // GPIO2

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial monitor

  Serial.println("\n\n=================================");
  Serial.println("BH1750 Illuminance Sensor Test");
  Serial.println("Waveshare ESP32-C6-Zero");
  Serial.println("=================================");
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.println("=================================\n");

  // Initialize I2C with custom pins for Waveshare ESP32-C6-Zero
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize BH1750
  Serial.println("Initializing BH1750...");
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("✓ BH1750 initialized successfully");
    Serial.println("  Address: 0x23");
    Serial.println("  Mode: CONTINUOUS_HIGH_RES (1 lux resolution)");
  } else {
    Serial.println("✗ ERROR: BH1750 not found!");
    Serial.println("\nTroubleshooting:");
    Serial.println("  1. Check VCC → 3.3V (NOT 5V!)");
    Serial.println("  2. Check GND → GND");
    Serial.println("  3. Check SCL → GPIO2");
    Serial.println("  4. Check SDA → GPIO1");
    Serial.println("  5. Verify 4.7kΩ pull-up resistors on SDA/SCL");
    Serial.println("\nScanning I2C bus...");

    for(byte address = 1; address < 127; address++) {
      Wire.beginTransmission(address);
      byte error = Wire.endTransmission();
      if (error == 0) {
        Serial.printf("  Found device at 0x%02X\n", address);
      }
    }
    Serial.println("Scan complete.\n");
  }

  Serial.println("=================================\n");
  Serial.println("Starting measurements (every 2 seconds)...\n");
}

void loop() {
  float lux = lightMeter.readLightLevel();

  if (lux < 0) {
    Serial.println("ERROR: Failed to read from BH1750 sensor");
  } else {
    // Print lux value with 1 decimal place
    Serial.printf("Light: %6.1f lux  ", lux);

    // Human-readable interpretation
    if (lux < 1) {
      Serial.print("│ Pitch Black      ");
    } else if (lux < 50) {
      Serial.print("│ Very Dim         ");
    } else if (lux < 200) {
      Serial.print("│ Dim Indoor       ");
    } else if (lux < 500) {
      Serial.print("│ Normal Indoor    ");
    } else if (lux < 1000) {
      Serial.print("│ Bright Indoor    ");
    } else if (lux < 10000) {
      Serial.print("│ Overcast/Shade   ");
    } else if (lux < 32000) {
      Serial.print("│ Full Daylight    ");
    } else {
      Serial.print("│ Direct Sunlight  ");
    }

    // Visual bar graph (scaled to 50 lux per character)
    Serial.print("│ ");
    int bar_length = min((int)(lux / 50), 40);  // Max 40 chars
    for(int i = 0; i < bar_length; i++) {
      Serial.print("█");
    }

    Serial.println();
  }

  delay(2000);  // Update every 2 seconds
}
