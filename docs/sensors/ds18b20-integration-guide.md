# DS18B20 Outdoor Temperature Sensor - Integration Guide

**Date:** 2026-01-12
**Status:** Sensor ordered, awaiting delivery
**Purpose:** Outdoor temperature monitoring for ESP32-C6 Zigbee multi-sensor

---

## Hardware Specifications

### DS18B20 Digital Temperature Sensor

| Specification | Value |
|--------------|-------|
| **Interface** | 1-Wire (Dallas/Maxim protocol) |
| **Accuracy** | ±0.5°C (-10°C to +85°C) |
| **Resolution** | 9-12 bits (0.0625°C at 12-bit) |
| **Temperature Range** | -55°C to +125°C |
| **Conversion Time** | 750ms (12-bit resolution) |
| **Supply Voltage** | 3.0V - 5.5V (3.3V compatible) |
| **Current Draw** | 1.0 mA (active), 1 µA (idle) |
| **Package** | Waterproof stainless steel probe |
| **Cable Length** | Typically 1-2m |

---

## Wiring Diagram

### ESP32-C6 Connection

```
DS18B20 Probe                    ESP32-C6-Zero
┌─────────────┐
│  Waterproof │
│   Probe     │                   ┌──────────────┐
│             │                   │              │
└──────┬──────┘                   │              │
       │ Cable (1-2m)             │              │
       │                          │              │
       ├─ RED    ────────────────►│ 3.3V         │
       │                          │              │
       ├─ YELLOW ────────────────►│ GPIO5 (Data) │
       │             │            │              │
       │             └──[4.7kΩ]──►│ 3.3V         │
       │                          │              │
       └─ BLACK  ────────────────►│ GND          │
                                  │              │
                                  └──────────────┘
```

**Wire Colors** (Standard, verify your probe):
- **RED:** VCC (3.3V)
- **YELLOW/WHITE:** Data (to GPIO5)
- **BLACK:** Ground

**Pull-Up Resistor:** 4.7kΩ between Data and VCC (often included in probe module)

---

## Pin Assignment

| GPIO | Function | Notes |
|------|----------|-------|
| **GPIO5** | DS18B20 Data | 1-Wire bus, 4.7kΩ pull-up required |
| GPIO4 | DHT11 Data | Indoor sensor (already assigned) |

**Benefit:** Both DHT11 and DS18B20 use 1-Wire protocol, similar driver patterns.

---

## PlatformIO Configuration

### platformio.ini

```ini
[env:waveshare-esp32-c6-zero]
platform = espressif32
board = esp32-c6-devkitc-1
framework = arduino
monitor_speed = 115200

; Libraries
lib_deps =
    adafruit/DHT sensor library@^1.4.4        ; DHT11 (indoor)
    claws/BH1750@^1.3.0                       ; BH1750 (illuminance)
    paulstoffregen/OneWire@^2.3.7             ; 1-Wire protocol
    milesburton/DallasTemperature@^3.11.0     ; DS18B20 driver

build_flags =
    -DLED_BUILTIN=15
    -DWAVESHARE_ESP32_C6_ZERO=1
```

---

## Arduino Framework Example Code

### Simple Test (Verify Sensor Works)

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_PIN 5  // GPIO5

// Setup 1-Wire bus
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== DS18B20 Outdoor Temperature Test ===");

  // Initialize sensor
  sensors.begin();

  // Check if sensor is detected
  int deviceCount = sensors.getDeviceCount();
  Serial.printf("Found %d DS18B20 sensor(s)\n", deviceCount);

  if (deviceCount == 0) {
    Serial.println("ERROR: No DS18B20 found! Check wiring.");
    Serial.println("- Verify RED to 3.3V");
    Serial.println("- Verify YELLOW to GPIO5");
    Serial.println("- Verify BLACK to GND");
    Serial.println("- Check 4.7kΩ pull-up resistor");
  }

  // Set resolution (9-12 bits)
  sensors.setResolution(12);  // 0.0625°C precision
}

void loop() {
  // Request temperature
  sensors.requestTemperatures();

  // Read temperature
  float temp_c = sensors.getTempCByIndex(0);

  // Check if reading is valid
  if (temp_c == DEVICE_DISCONNECTED_C) {
    Serial.println("ERROR: Sensor disconnected!");
  } else {
    Serial.printf("Outdoor Temperature: %.2f°C (%.2f°F)\n",
                  temp_c, (temp_c * 9.0/5.0) + 32.0);
  }

  delay(2000);  // Read every 2 seconds
}
```

**Expected Output:**
```
=== DS18B20 Outdoor Temperature Test ===
Found 1 DS18B20 sensor(s)
Outdoor Temperature: 18.37°C (65.07°F)
Outdoor Temperature: 18.31°C (64.96°F)
Outdoor Temperature: 18.44°C (65.19°F)
```

---

## Multi-Sensor Integration

### Combined Indoor + Outdoor Temperature

```cpp
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHT_PIN 4       // GPIO4 - Indoor
#define DS18B20_PIN 5   // GPIO5 - Outdoor

DHT dht(DHT_PIN, DHT11);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

void setup() {
  Serial.begin(115200);

  dht.begin();
  ds18b20.begin();
  ds18b20.setResolution(12);

  Serial.println("Multi-Sensor Temperature Monitor");
  Serial.println("Indoor: DHT11 | Outdoor: DS18B20");
}

void loop() {
  // Read indoor (DHT11)
  float indoor_temp = dht.readTemperature();
  float indoor_humidity = dht.readHumidity();

  // Read outdoor (DS18B20)
  ds18b20.requestTemperatures();
  float outdoor_temp = ds18b20.getTempCByIndex(0);

  // Display
  Serial.println("─────────────────────────────────");
  Serial.printf("Indoor:  %.1f°C | %.1f%% RH\n", indoor_temp, indoor_humidity);
  Serial.printf("Outdoor: %.2f°C\n", outdoor_temp);
  Serial.printf("Delta:   %.1f°C\n", indoor_temp - outdoor_temp);

  delay(5000);  // Update every 5 seconds
}
```

---

## ESP-IDF Native Implementation

### Using ESP-IDF Drivers

```cpp
#include "ds18b20.h"  // ESP-IDF OneWire driver

#define DS18B20_GPIO GPIO_NUM_5

// FreeRTOS task for outdoor temperature
void outdoor_temp_task(void *pvParameters) {
  ds18b20_device_t ds18b20;

  // Initialize 1-Wire bus
  ESP_ERROR_CHECK(ds18b20_init(&ds18b20, DS18B20_GPIO));

  while (1) {
    float temperature;
    esp_err_t err = ds18b20_get_temperature(&ds18b20, &temperature);

    if (err == ESP_OK) {
      ESP_LOGI("DS18B20", "Outdoor Temperature: %.2f°C", temperature);

      // Update Zigbee attribute (Endpoint 11, Temperature cluster)
      update_zigbee_temperature(11, temperature);
    } else {
      ESP_LOGE("DS18B20", "Failed to read temperature: %s", esp_err_to_name(err));
    }

    vTaskDelay(pdMS_TO_TICKS(60000));  // Update every 60 seconds
  }
}
```

---

## Zigbee Integration

### Endpoint 11: Outdoor Temperature

**Cluster:** 0x0402 (Temperature Measurement)

**Attributes:**
- `MeasuredValue`: Temperature in 0.01°C units (int16)
- `MinMeasuredValue`: -5500 (-55.00°C)
- `MaxMeasuredValue`: 12500 (125.00°C)

**Reporting Interval:** 60 seconds (configurable)

### ESP-IDF Zigbee Cluster Setup

```cpp
// Create endpoint 11 for outdoor temperature
esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();

// Temperature cluster configuration
esp_zb_temperature_meas_cluster_cfg_t outdoor_temp_cfg = {
  .measured_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MEASURED_VALUE_INVALID,
  .min_value = -5500,   // -55.00°C
  .max_value = 12500    // 125.00°C
};

// Create cluster list for endpoint 11
esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
esp_zb_cluster_list_add_temperature_meas_cluster(
  cluster_list,
  &outdoor_temp_cfg,
  ESP_ZB_ZCL_CLUSTER_SERVER_ROLE
);

// Register endpoint 11
esp_zb_ep_list_add_ep(
  ep_list,
  cluster_list,
  11,  // Endpoint ID
  ESP_ZB_AF_HA_PROFILE_ID,
  ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID
);
```

### Update Zigbee Attribute

```cpp
void update_outdoor_temperature(float temp_celsius) {
  // Convert to Zigbee format (0.01°C units)
  int16_t temp_value = (int16_t)(temp_celsius * 100.0);

  // Update attribute
  esp_zb_zcl_set_attribute_val(
    11,  // Endpoint 11 (outdoor)
    ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
    &temp_value,
    false
  );

  ESP_LOGI("ZIGBEE", "Updated outdoor temp: %.2f°C", temp_celsius);
}
```

---

## Home Assistant Integration

### Expected Entities

After Zigbee pairing, Home Assistant will create:

```yaml
# Indoor Temperature (Endpoint 10)
sensor.esp32_multisensor_indoor_temperature:
  friendly_name: "Cinema Room Indoor Temperature"
  unit_of_measurement: "°C"
  device_class: temperature

# Outdoor Temperature (Endpoint 11)
sensor.esp32_multisensor_outdoor_temperature:
  friendly_name: "Cinema Room Outdoor Temperature"
  unit_of_measurement: "°C"
  device_class: temperature

# Indoor Humidity (Endpoint 10)
sensor.esp32_multisensor_indoor_humidity:
  friendly_name: "Cinema Room Humidity"
  unit_of_measurement: "%"
  device_class: humidity
```

### Example Automation (Temperature Delta Alert)

```yaml
automation:
  - alias: "Large Indoor/Outdoor Temperature Delta"
    trigger:
      - platform: template
        value_template: >
          {{ (states('sensor.esp32_multisensor_indoor_temperature')|float -
              states('sensor.esp32_multisensor_outdoor_temperature')|float)|abs > 15 }}
    action:
      - service: notify.mobile_app
        data:
          message: >
            Large temperature difference detected:
            Indoor: {{ states('sensor.esp32_multisensor_indoor_temperature') }}°C
            Outdoor: {{ states('sensor.esp32_multisensor_outdoor_temperature') }}°C
```

---

## Troubleshooting

### Sensor Not Detected

**Problem:** `Found 0 DS18B20 sensor(s)`

**Checks:**
1. **Power:** Verify RED wire to 3.3V (not 5V, ESP32-C6 is 3.3V!)
2. **Ground:** Verify BLACK wire to GND
3. **Data:** Verify YELLOW wire to GPIO5
4. **Pull-Up:** Check 4.7kΩ resistor between Data and VCC
5. **Solder:** If DIY probe, check solder joints
6. **Cable:** Test for continuity with multimeter

**Test Commands:**
```cpp
// Try different GPIO pin
#define DS18B20_PIN 6  // Try GPIO6 instead

// Try slower resolution
sensors.setResolution(9);  // 9-bit (faster, less precise)

// Check OneWire bus
oneWire.reset();
if (!oneWire.search(addr)) {
  Serial.println("No 1-Wire devices found");
}
```

---

### Reading DEVICE_DISCONNECTED_C

**Problem:** Sensor found but returns -127°C (DEVICE_DISCONNECTED_C)

**Causes:**
1. Insufficient conversion time (need 750ms)
2. Weak pull-up resistor (try 3.3kΩ instead of 4.7kΩ)
3. Long cable (>3m causes issues, add stronger pull-up)
4. Power supply noise (add 100nF decoupling capacitor)

**Fix:**
```cpp
// Increase delay after request
sensors.requestTemperatures();
delay(1000);  // Wait 1 second (safe)
float temp = sensors.getTempCByIndex(0);
```

---

### Readings Fluctuating

**Problem:** Temperature jumps around (e.g., 18.3°C → 19.1°C → 17.8°C)

**Causes:**
1. Probe not fully immersed/in thermal contact
2. Self-heating from frequent reads
3. Electrical noise on 1-Wire bus

**Fix:**
```cpp
// Average multiple readings
float get_stable_temperature() {
  float sum = 0;
  int valid_readings = 0;

  for (int i = 0; i < 5; i++) {
    sensors.requestTemperatures();
    delay(1000);
    float temp = sensors.getTempCByIndex(0);

    if (temp != DEVICE_DISCONNECTED_C) {
      sum += temp;
      valid_readings++;
    }
    delay(500);
  }

  return valid_readings > 0 ? sum / valid_readings : DEVICE_DISCONNECTED_C;
}
```

---

## Outdoor Installation Tips

### Cable Management

1. **Weatherproofing:** Use IP67+ junction box for ESP32-C6 enclosure
2. **Cable Entry:** Silicone grommet or cable gland
3. **Strain Relief:** Secure cable near entry point
4. **Sunlight:** Keep probe out of direct sun (radiative heating skews reading)

### Optimal Probe Placement

**Good Locations:**
- North-facing wall (no direct sun in Northern Hemisphere)
- Under eaves (rain/snow protected)
- 1.5-2m above ground (standard meteorological height)
- Away from heat sources (AC units, dryer vents, chimneys)

**Bad Locations:**
- Direct sunlight (reads 5-10°C high)
- Inside enclosure with ESP32 (heat from electronics)
- Near ground (microclimates, not representative)
- Against warm building walls

### Cable Length Considerations

| Cable Length | Pull-Up Resistor | Notes |
|-------------|------------------|-------|
| < 1m | 4.7kΩ | Standard, no issues |
| 1-3m | 3.3kΩ - 4.7kΩ | May need stronger pull-up |
| 3-10m | 2.2kΩ - 3.3kΩ | Stronger pull-up recommended |
| > 10m | 1.5kΩ - 2.2kΩ | Signal integrity concerns |

---

## Performance Characteristics

### Temperature Resolution

| Bits | Resolution | Conversion Time |
|------|-----------|----------------|
| 9-bit | 0.5°C | 93.75ms |
| 10-bit | 0.25°C | 187.5ms |
| 11-bit | 0.125°C | 375ms |
| 12-bit | 0.0625°C | 750ms |

**Recommendation:** 12-bit for outdoor monitoring (highest accuracy)

### Update Frequency

**Zigbee Reporting:** 60 seconds (matches indoor DHT11)

**Rationale:**
- Outdoor temperature changes slowly
- Reduces Zigbee network traffic
- Extends device lifetime (less wear on components)
- 60s matches meteorological standards

---

## Next Steps (When Sensor Arrives)

### Week 1: Hardware Validation
1. ✅ DS18B20 ordered
2. [ ] Unbox and inspect probe
3. [ ] Test wiring with multimeter (continuity)
4. [ ] Connect to ESP32-C6 (GPIO5, 3.3V, GND)
5. [ ] Upload test code (see "Simple Test" above)
6. [ ] Verify temperature readings

### Week 2: Integration
1. [ ] Add to multi-sensor sketch (with DHT11, BH1750)
2. [ ] Test both indoor and outdoor sensors simultaneously
3. [ ] Verify no I/O conflicts

### Week 3: Zigbee Integration (ESP-IDF)
1. [ ] Create Endpoint 11 (outdoor temp cluster)
2. [ ] Update Zigbee attributes from DS18B20 readings
3. [ ] Test in Home Assistant (verify entity creation)

### Production Deployment
1. [ ] Route cable through enclosure
2. [ ] Mount probe in optimal outdoor location
3. [ ] Seal cable entry (weatherproofing)
4. [ ] Test 24/7 stability

---

## Reference Links

**DS18B20 Datasheet:**
https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf

**Arduino Libraries:**
- OneWire: https://github.com/PaulStoffregen/OneWire
- DallasTemperature: https://github.com/milesburton/Arduino-Temperature-Control-Library

**ESP-IDF Examples:**
- OneWire driver: https://github.com/DavidAntliff/esp32-ds18b20-example

**Home Assistant:**
- Zigbee Temperature Sensor: https://www.home-assistant.io/integrations/zha/

---

**Status:** Ready for sensor arrival and testing
**Updated:** 2026-01-12
