# ESP32 Zigbee Multi-Sensor Development Plan

**GitHub Issue:** [#10](https://github.com/unmanned-systems-uk/homeassistant/issues/10)
**CCPM Task:** `b750d1e5-0d25-4f25-8f23-0329b6f19432`
**Created:** 2026-01-10

---

## Executive Summary

Build a production-ready ESP32-C6 Zigbee multi-sensor device for 24/7 environmental monitoring and mmWave presence detection. This device will serve as a template for future deployments across multiple locations.

**Key Goals:**
- Professional development workflow (VS Code + PlatformIO)
- Reliable Zigbee mesh networking (router/repeater)
- Multi-sensor integration (mmWave, temp, humidity, illuminance)
- OTA firmware update capability
- Complete documentation for replication

---

## 1. IDE Selection: VS Code + PlatformIO vs Arduino IDE

### Detailed Comparison

| Aspect | VS Code + PlatformIO | Arduino IDE 2.x | Winner |
|--------|---------------------|-----------------|--------|
| **Code Intelligence** | Full IntelliSense, auto-complete, symbol navigation | Basic auto-complete | 🏆 VS Code |
| **Multi-File Projects** | Excellent (proper project structure) | Awkward (tab-based) | 🏆 VS Code |
| **Library Management** | Built-in (`lib_deps` in config) | Manual via Library Manager | 🏆 VS Code |
| **Debugging** | Native JTAG/GDB support | Serial print only | 🏆 VS Code |
| **Build System** | SCons (fast, parallel) | Arduino-builder (slower) | 🏆 VS Code |
| **Multi-Board Support** | Environments in one config | Separate sketches | 🏆 VS Code |
| **Version Control** | Seamless Git integration | External tools needed | 🏆 VS Code |
| **OTA Updates** | Built-in upload methods | Manual scripts | 🏆 VS Code |
| **ESP-IDF Support** | Native framework option | Arduino framework only | 🏆 VS Code |
| **Learning Curve** | Steep (1-2 days) | Gentle (1-2 hours) | 🏆 Arduino |
| **Setup Time** | 30-60 minutes | 5-10 minutes | 🏆 Arduino |
| **Community Tutorials** | Growing | Massive | 🏆 Arduino |

### Decision: VS Code + PlatformIO

**Rationale:**
- **Production Scale**: Building multiple identical devices requires professional tooling
- **Maintainability**: Code navigation and refactoring are critical for long-term maintenance
- **Debugging**: Serial prints are insufficient for complex sensor integration
- **Reproducibility**: `platformio.ini` defines exact dependencies and build settings
- **Future-Proof**: ESP-IDF native support for advanced Zigbee features

**Arduino IDE is suitable for:**
- Quick prototyping (< 500 lines of code)
- Learning basics
- Single-file sketches

**PlatformIO is essential for:**
- Multi-sensor integration (our use case)
- Production firmware
- Team development
- Long-term projects

---

## 2. Hardware Architecture

### Component Selection

| Component | Part Number | Interface | Purpose | Notes |
|-----------|-------------|-----------|---------|-------|
| **MCU** | Waveshare ESP32-C6-Zero | - | Main controller | RISC-V, Zigbee 3.0, WiFi, BLE, compact module |
| **mmWave** | HLK-LD2450 | UART | Presence + trajectory | 24GHz radar, multi-target |
| **Temp/Humidity (Indoor)** | DHT11 | 1-Wire | Environmental | ±2°C, ±5% RH accuracy |
| **Temp (Outdoor)** | DS18B20 | 1-Wire | Outdoor temperature | ±0.5°C, waterproof probe **ORDERED** ✅ |
| **Illuminance** | BH1750 | I2C | Light level | 1-65535 lux, auto-gain **ORDERED** ✅ |
| **Power** | 5V 2A PSU | USB-C | 24/7 mains | Quality PSU required |

### Pin Allocation (Waveshare ESP32-C6-Zero)

| GPIO | Function | Sensor/Peripheral | Notes |
|------|----------|-------------------|-------|
| GPIO0 | Boot Button | - | Factory reset (hold 10s) |
| GPIO1 | I2C SDA | BH1750 | Pull-up required (4.7kΩ) |
| GPIO2 | I2C SCL | BH1750 | Pull-up required (4.7kΩ) |
| GPIO4 | DHT11 Data | DHT11 (Indoor) | 1-Wire protocol, 10kΩ pull-up |
| GPIO5 | DS18B20 Data | DS18B20 (Outdoor) | 1-Wire protocol, 4.7kΩ pull-up |
| GPIO16 | UART RX | HLK-LD2450 TX | 115200 baud |
| GPIO17 | UART TX | HLK-LD2450 RX | 115200 baud |
| GPIO15 | LED Status | Built-in LED | Network status indicator (pre-wired) |

**Note:** Waveshare ESP32-C6-Zero has castellated edges (not breadboard-friendly). See `docs/waveshare-esp32-c6-zero-notes.md` for details.

### Power Budget

| Component | Typical Current | Max Current | Notes |
|-----------|----------------|-------------|-------|
| ESP32-C6 | 50 mA | 300 mA | TX peaks |
| HLK-LD2450 | 150 mA | 200 mA | Constant |
| DHT11 (Indoor) | 0.5 mA | 2.5 mA | Polling only |
| DS18B20 (Outdoor) | 1.0 mA | 1.5 mA | Polling only |
| BH1750 | 0.12 mA | 0.18 mA | Continuous mode |
| LED | 5 mA | 60 mA | RGB at full white |
| **Total** | **~207 mA** | **~565 mA** | Use 2A PSU for headroom |

---

## 3. Software Architecture

### Framework Decision: ESP-IDF vs Arduino

| Aspect | ESP-IDF | Arduino Framework |
|--------|---------|-------------------|
| Zigbee Support | ✅ Native (esp-zigbee-lib) | ❌ Limited |
| OTA Updates | ✅ Built-in partitions | ✅ Via library |
| Learning Curve | Steep (FreeRTOS tasks) | Gentle (loop/setup) |
| Performance | Better (optimized) | Good enough |
| Libraries | ESP-specific | Massive ecosystem |
| Recommendation | **Production** | **Prototyping** |

**Decision**: Start with **Arduino framework** for sensor testing, migrate to **ESP-IDF** for Zigbee integration.

### Project Structure

```
esp32-zigbee-sensor/
├── platformio.ini          # Project configuration
├── src/
│   ├── main.cpp            # Entry point
│   ├── zigbee_handler.cpp  # Zigbee stack management
│   ├── sensor_dht11.cpp    # DHT11 driver
│   ├── sensor_bh1750.cpp   # BH1750 driver
│   ├── sensor_ld2450.cpp   # HLK-LD2450 UART parser
│   └── ota_handler.cpp     # OTA update logic
├── include/
│   └── *.h                 # Header files
├── lib/
│   └── HLK-LD2450/         # Custom library
├── data/
│   └── www/                # Web UI (optional)
├── test/
│   └── test_sensors.cpp    # Unit tests
└── docs/
    ├── assembly.md         # Hardware assembly guide
    ├── flashing.md         # Firmware flash guide
    └── troubleshooting.md  # Debug procedures
```

### Zigbee Device Definition

**Device Type**: Multi-Sensor (Router/Repeater)
**Profile**: Home Automation (0x0104)
**Multiple Endpoints**: Each sensor on dedicated endpoint

**Endpoints & Clusters**:

| Endpoint | Sensor | Clusters |
|----------|--------|----------|
| **10** | DHT11 (Indoor) | 0x0000 (Basic), 0x0402 (Temperature), 0x0405 (Humidity) |
| **11** | DS18B20 (Outdoor) | 0x0402 (Temperature) |
| **12** | BH1750 (Light) | 0x0400 (Illuminance) |
| **13** | LD2450 (Presence) | 0x0406 (Occupancy), 0xFC00 (Custom Trajectory) |

**Cluster Details**:
| Cluster ID | Name | Type | Purpose |
|------------|------|------|---------|
| 0x0000 | Basic | Server | Device info (EP10 only) |
| 0x0402 | Temperature | Server | DHT11 indoor (EP10), DS18B20 outdoor (EP11) |
| 0x0405 | Humidity | Server | DHT11 humidity (EP10) |
| 0x0400 | Illuminance | Server | BH1750 lux (EP12) |
| 0x0406 | Occupancy | Server | LD2450 presence (EP13) |
| 0xFC00 | Custom (Trajectory) | Server | LD2450 X/Y coords (EP13) |

### State Machine

```
┌─────────────┐
│   BOOT      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  INIT I/O   │ ─── GPIO, UART, I2C setup
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ ZIGBEE JOIN │ ─── Network discovery
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  SENSOR RUN │◄─── Main loop (FreeRTOS task)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  OTA UPDATE │ ─── On command from coordinator
└──────┬──────┘
       │
       ▼
   [REBOOT]
```

---

## 4. Development Workflow

### Phase 1: Environment Setup (Day 1)

**Objectives**:
- Install and configure VS Code + PlatformIO
- Test basic ESP32-C6 connectivity
- Create project template

**Steps**:
1. Install VS Code from official website
2. Install PlatformIO extension (ID: `platformio.platformio-ide`)
3. Install USB-UART drivers:
   - **Windows**: CP210x or CH340 drivers
   - **Linux**: `sudo usermod -a -G dialout $USER` (logout/login)
   - **macOS**: Built-in (no action needed)
4. Create new project:
   ```bash
   pio project init --board esp32-c6-devkitc-1
   ```
5. Test "Hello World":
   ```cpp
   void setup() {
     Serial.begin(115200);
     Serial.println("ESP32-C6 Ready!");
   }
   void loop() {
     delay(1000);
   }
   ```
6. Upload and verify serial output

**Success Criteria**:
- ✅ PlatformIO builds firmware without errors
- ✅ Upload completes successfully
- ✅ Serial monitor shows "ESP32-C6 Ready!" every second

**Documentation**: `docs/01-setup-vscode-platformio.md`

---

### Phase 2: Sensor Integration (Day 2-3)

**Order of Integration** (simple to complex):

#### 2.1 BH1750 Illuminance Sensor (Easiest)

**Library**: `claws/BH1750@^1.3.0`

```cpp
#include <BH1750.h>
BH1750 lightMeter;

void setup() {
  Wire.begin(1, 2); // SDA=GPIO1, SCL=GPIO2 (Waveshare ESP32-C6-Zero)
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.printf("Light: %.1f lux\n", lux);
  delay(1000);
}
```

**Test Plan**:
- Verify readings change with flashlight
- Confirm range: 0 (dark) to 65535 (bright)

---

#### 2.2 DHT11 Temperature/Humidity (Medium)

**Library**: `adafruit/DHT sensor library@^1.4.4`

```cpp
#include <DHT.h>
DHT dht(4, DHT11); // GPIO4 (Waveshare ESP32-C6-Zero)

void setup() {
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  Serial.printf("Temp: %.1f°C, Humidity: %.1f%%\n", temp, humidity);
  delay(2000); // DHT11 minimum sampling period
}
```

**Challenges**:
- Timing-sensitive (1-Wire protocol)
- May conflict with Zigbee interrupts → test thoroughly

**Test Plan**:
- Verify temp matches room temperature (±2°C)
- Breathe on sensor to verify humidity change

---

#### 2.3 HLK-LD2450 mmWave Sensor (Complex)

**Interface**: UART at 115200 baud (or 256000)
**Protocol**: Binary packets with header `0xAA 0xFF 0x03 0x00`

**Library**: Custom (no official Arduino library exists)

**Implementation**:
```cpp
#define RX_PIN 16
#define TX_PIN 17

HardwareSerial LD2450Serial(1); // UART1

struct Target {
  int16_t x;
  int16_t y;
  uint16_t distance;
  bool detected;
};

Target targets[3]; // Multi-target tracking

void setup() {
  LD2450Serial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  if (LD2450Serial.available()) {
    parseLD2450Packet();
  }
}

void parseLD2450Packet() {
  // Binary protocol parsing (see datasheet)
  // Header: AA FF 03 00
  // Data: Target count, X/Y coords, distance, speed
  // Checksum: Sum of all bytes
}
```

**Challenges**:
- Binary protocol (requires datasheet analysis)
- Multi-target tracking (up to 3 simultaneous targets)
- Coordinate system mapping (X/Y to room layout)

**Test Plan**:
- Verify presence detection when entering room
- Check X/Y coordinates match physical movement
- Test multi-target tracking with 2+ people

**Documentation**: Create `lib/HLK-LD2450/README.md` with protocol specs

---

### Phase 3: Zigbee Integration (Day 4-5)

**Framework Migration**: Switch to ESP-IDF for native Zigbee support

**platformio.ini**:
```ini
[env:waveshare-esp32-c6-zero]
platform = espressif32
board = esp32-c6-devkitc-1  ; Use DevKitC-1 profile (same ESP32-C6 chip)
framework = espidf
monitor_speed = 115200

; Board-specific (Waveshare ESP32-C6-Zero)
board_build.mcu = esp32c6
board_build.f_cpu = 160000000L
build_flags =
    -DCONFIG_ZB_ENABLED=1
    -DCONFIG_ZB_ZCZR=1  ; Zigbee Coordinator/Router
    -DLED_BUILTIN=15    ; Waveshare built-in LED on GPIO15

lib_deps =
    espressif/esp-zigbee-lib@^1.0.0
```

**Zigbee Initialization**:
```cpp
#include "esp_zigbee_core.h"

void zigbee_init() {
  esp_zb_platform_config_t config = {
    .radio_mode = RADIO_MODE_NATIVE,
    .host_connection_mode = HOST_CONNECTION_MODE_NONE
  };
  esp_zb_platform_config(&config);

  // Create endpoint
  esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();

  // Add temperature cluster
  esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
  esp_zb_temperature_meas_cluster_cfg_t temp_cfg = {
    .measured_value = 2000, // 20.00°C in 0.01°C units
    .min_value = -5000,
    .max_value = 12500
  };
  esp_zb_cluster_list_add_temperature_meas_cluster(
    cluster_list, &temp_cfg, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE
  );

  // Register endpoint
  esp_zb_ep_list_add_ep(ep_list, cluster_list, 10, ESP_ZB_AF_HA_PROFILE_ID);
  esp_zb_device_register(ep_list);

  // Start Zigbee stack
  esp_zb_start(false);
}
```

**Zigbee Reporting**:
```cpp
void report_sensor_data() {
  // Read sensors
  float temp = dht.readTemperature();
  int16_t temp_value = (int16_t)(temp * 100); // Convert to 0.01°C

  // Update Zigbee attribute
  esp_zb_zcl_set_attribute_val(
    10, // endpoint
    ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
    &temp_value,
    false
  );

  // Trigger report
  esp_zb_zcl_report_attr_cmd_t cmd = {
    .zcl_basic_cmd.dst_addr_u.addr_short = 0x0000, // Coordinator
    .zcl_basic_cmd.dst_endpoint = 1,
    .zcl_basic_cmd.src_endpoint = 10,
    .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    .clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    .attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
    .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE
  };
  esp_zb_zcl_report_attr_cmd_req(&cmd);
}
```

**Test Plan**:
- Join Zigbee network (Zigbee2MQTT or ZHA)
- Verify device appears in Home Assistant
- Confirm sensor entities are created
- Check attribute reporting interval (default: 60s)

---

### Phase 4: OTA Implementation (Day 6)

**Partition Table** (`partitions.csv`):
```csv
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x1F0000,
ota_0,    app,  ota_0,   0x200000,0x1F0000,
ota_1,    app,  ota_1,   0x3F0000,0x1F0000,
```

**platformio.ini**:
```ini
board_build.partitions = partitions.csv
```

**Zigbee OTA Server Setup** (Home Assistant):
1. Place firmware in `/config/zigbee_ota/`
2. Name file: `esp32-c6-sensor_v1.0.0.bin`
3. Trigger update from ZHA/Zigbee2MQTT UI

**WiFi OTA (Development Only)**:
```cpp
#include <WiFi.h>
#include <ArduinoOTA.h>

void setup_wifi_ota() {
  WiFi.begin("SSID", "password");
  ArduinoOTA.setHostname("esp32-zigbee-sensor");
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
}
```

**Test Plan**:
- Upload OTA firmware via WiFi (dev)
- Upload OTA firmware via Zigbee (production)
- Verify device restarts and retains settings

---

### Phase 5: Production Hardening (Day 7)

**Watchdog Timer**:
```cpp
#include "esp_task_wdt.h"

void setup() {
  esp_task_wdt_init(30, true); // 30s timeout
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset(); // Pet the watchdog
  // ... sensor reading code ...
}
```

**Network Reconnection**:
```cpp
void zigbee_event_handler(esp_zb_event_t event) {
  switch (event.type) {
    case ESP_ZB_EVENT_TYPE_NETWORK_LOST:
      Serial.println("Network lost, rejoining...");
      esp_zb_start(false); // Rejoin network
      break;
  }
}
```

**Factory Reset**:
```cpp
void check_factory_reset() {
  if (digitalRead(0) == LOW) { // Boot button held
    unsigned long start = millis();
    while (digitalRead(0) == LOW) {
      if (millis() - start > 10000) { // 10 seconds
        Serial.println("Factory reset!");
        esp_zb_factory_reset();
        esp_restart();
      }
    }
  }
}
```

**Status LED**:
```cpp
enum Status {
  BOOTING,      // Blink fast (blue)
  JOINING,      // Blink slow (yellow)
  CONNECTED,    // Solid green
  ERROR         // Blink fast (red)
};

void update_status_led(Status status) {
  // WS2812 RGB LED implementation
}
```

---

## 5. Documentation Templates

### 5.1 Setup Guide (`docs/01-setup-vscode-platformio.md`)

**Sections**:
1. Prerequisites (OS, Python version)
2. VS Code installation
3. PlatformIO extension setup
4. USB driver installation (OS-specific)
5. Test project creation
6. Troubleshooting (common errors)

---

### 5.2 Build Guide (`docs/02-build-firmware.md`)

**Sections**:
1. Clone repository
2. Open project in VS Code
3. Configure `platformio.ini` (board, pins)
4. Install dependencies (`pio lib install`)
5. Build firmware (`pio run`)
6. Verify build artifacts

---

### 5.3 Flash Guide (`docs/03-flash-firmware.md`)

**Sections**:
1. Connect ESP32-C6 via USB
2. Identify serial port (`pio device list`)
3. Erase flash (`pio run --target erase`)
4. Upload firmware (`pio run --target upload`)
5. Monitor serial output (`pio device monitor`)
6. Troubleshooting (upload failures)

---

### 5.4 Integration Guide (`docs/04-home-assistant-integration.md`)

**Sections**:
1. Prerequisites (Zigbee coordinator setup)
2. Put device in pairing mode
3. Join Zigbee network (ZHA/Zigbee2MQTT)
4. Verify entities in Home Assistant
5. Configure reporting intervals
6. Create automations (presence-based)

---

### 5.5 Deployment Checklist (`docs/05-deployment-checklist.md`)

**Pre-Deployment**:
- [ ] Flash latest firmware
- [ ] Test all sensors (serial monitor)
- [ ] Join Zigbee network
- [ ] Verify HA entities created
- [ ] Test OTA update
- [ ] Perform factory reset test
- [ ] Run 24-hour stability test

**Deployment**:
- [ ] Mount device in final location
- [ ] Connect to quality PSU (not USB hub)
- [ ] Verify Zigbee signal strength (LQI > 150)
- [ ] Configure HA automations
- [ ] Document device ID and location

**Post-Deployment**:
- [ ] Monitor for 7 days (check logs daily)
- [ ] Verify reporting intervals
- [ ] Test presence detection accuracy
- [ ] Update documentation with lessons learned

---

## 6. Parts Ordering

### Immediate Order (Required)

| Part | Quantity | Supplier | Est. Cost | Notes |
|------|----------|----------|-----------|-------|
| ~~ESP32-C6-DevKitC-1~~ | ~~5~~ | - | - | **User has Waveshare ESP32-C6-Zero (x2)** ✅ |
| BH1750 Module | ~~5~~ 2 | Amazon | $3 ea | **ORDERED** ✅ I2C light sensor |
| DS18B20 Waterproof Probe | 1-2 | Amazon | $8-10 ea | **ORDERED** ✅ Outdoor temp sensor |
| HLK-LD2450 | 1-2 | AliExpress | $20 ea | 24GHz mmWave radar |
| DHT11 Module | 2-5 | Amazon | $1 ea | Assume user has |
| 5V 2A USB-C PSU | 2 | Amazon | $8 ea | Quality brand (Anker) |
| ~~Breadboard~~ | - | - | - | **Not needed** - castellated module |
| ~~Jumper Wires~~ | - | - | - | **Not needed** - going to PCB |
| USB-C Cable | 2 | Amazon | $3 ea | Data-capable (not charging-only) |
| **Custom PCB** | 5-10 | JLCPCB/PCBWay | $2-5 ea | **NEW** - Required for Waveshare module |
| Castellated Adapter (opt) | 1 | AliExpress | $5 | Optional for breadboard testing |

**Total Estimated Cost**: ~$100 (parts mostly owned, PCBs are cheap)

### Future Expansion (Optional)

- Enclosures (3D printed or Hammond boxes)
- PCB fabrication (after prototyping)
- Better sensors (SHT31 instead of DHT11)
- External antennas (for better Zigbee range)

---

## 7. Risk Mitigation

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| ESP32-C6 chip shortage | High | Medium | Order extras now |
| Zigbee stack complexity | Medium | High | Start with examples, iterate |
| DHT11 timing conflicts | Medium | Medium | Test with Zigbee early |
| LD2450 protocol undocumented | High | Medium | Join community forums, reverse engineer |
| OTA brick device | High | Low | Keep USB programmer handy |
| Power supply failure | Medium | Low | Use quality PSU, surge protection |

---

## 8. Success Metrics

**Development Phase**:
- [ ] VS Code + PlatformIO setup time: < 60 minutes
- [ ] Each sensor working individually: Day 3
- [ ] Zigbee network join success rate: > 95%
- [ ] OTA update success rate: > 99%

**Production Phase**:
- [ ] 24/7 uptime without crashes: > 99.9% (< 8.6 min downtime/day)
- [ ] Presence detection latency: < 1 second
- [ ] Sensor reporting interval: 60 seconds (configurable)
- [ ] Documentation allows non-technical person to deploy in < 2 hours

---

## 9. Next Steps

**Immediate Actions** (Today):
1. ✅ Create GitHub Issue #10
2. ✅ Create CCPM task
3. [ ] Order parts (BH1750 illuminance sensor)
4. [ ] Install VS Code + PlatformIO
5. [ ] Test ESP32-C6 connectivity

**Week 1** (Learning & Prototyping):
- Days 1-3: Sensor integration on breadboard
- Days 4-5: Zigbee network joining
- Days 6-7: OTA implementation

**Week 2** (Production Hardening):
- Days 1-3: Stability testing (watchdog, reconnection)
- Days 4-5: Documentation writing
- Days 6-7: First deployment + monitoring

**Week 3** (Replication):
- Build devices #2-5 using documentation
- Refine deployment checklist
- Train team member to deploy independently

---

## 10. Questions for Review

Before proceeding, confirm:

1. **ESP32-C6 vs ESP32-H2**: Stick with C6 for WiFi debugging capability?
2. **Illuminance Sensor**: BH1750, TSL2561, or VEML7700? (BH1750 recommended)
3. **Enclosure**: 3D print custom or buy Hammond box?
4. **PCB**: Prototype on breadboard first, then design PCB later?
5. **HLK-LD2450 Firmware**: Use default or update to latest? (check compatibility)

---

## Appendix A: VS Code + PlatformIO Advantages for This Project

### 1. Multi-Sensor Codebase Management

**Without PlatformIO** (Arduino IDE):
```
Sensor_Project.ino           # 2000+ lines in one file
```

**With PlatformIO**:
```
src/
├── main.cpp                 # 50 lines (setup/loop)
├── zigbee_handler.cpp       # 300 lines
├── sensor_dht11.cpp         # 100 lines
├── sensor_bh1750.cpp        # 80 lines
└── sensor_ld2450.cpp        # 400 lines (complex protocol)
```

**Advantage**: Modularity, testability, team collaboration

---

### 2. Dependency Management

**Arduino IDE**:
- Install libraries via GUI
- No version locking → breaks on updates
- No project-specific libraries

**PlatformIO** (`platformio.ini`):
```ini
lib_deps =
    adafruit/DHT sensor library@^1.4.4
    claws/BH1750@^1.3.0
    espressif/esp-zigbee-lib@^1.0.0
```

**Advantage**: Reproducible builds across machines

---

### 3. Build Environments

**Scenario**: Test WiFi OTA (dev) vs Zigbee OTA (prod)

**PlatformIO**:
```ini
[env:dev]
build_flags = -DWIFI_OTA_ENABLED

[env:prod]
build_flags = -DZIGBEE_ONLY
```

**Command**:
```bash
pio run -e dev    # Build dev firmware
pio run -e prod   # Build prod firmware
```

**Advantage**: Switch configurations without editing code

---

### 4. Debugging

**Arduino IDE**: Serial print debugging only

**PlatformIO**:
- GDB debugger with breakpoints
- Variable inspection
- Call stack analysis

**Example**:
```cpp
// Set breakpoint here in VS Code
void parseLD2450Packet() {
  uint8_t header = Serial.read(); // Inspect `header` value
  // Step through binary protocol parsing
}
```

**Advantage**: Debug complex sensor protocols efficiently

---

### 5. Testing

**PlatformIO** (`test/test_sensors.cpp`):
```cpp
#include <unity.h>

void test_dht11_range() {
  float temp = dht.readTemperature();
  TEST_ASSERT_FLOAT_WITHIN(50, 25, temp); // 25°C ± 50°C
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_dht11_range);
  UNITY_END();
}
```

**Command**:
```bash
pio test
```

**Advantage**: Automated validation before deployment

---

## Appendix B: Learning Resources

### VS Code + PlatformIO
- Official docs: https://docs.platformio.org/en/latest/
- Video tutorial: "PlatformIO for ESP32" (Andreas Spiess)

### ESP32-C6 Zigbee
- Espressif docs: https://docs.espressif.com/projects/esp-zigbee-sdk/
- Examples: `esp-idf/examples/zigbee/`

### HLK-LD2450
- Datasheet: https://www.hlktech.net/index.php?id=988
- Community: https://github.com/topics/hlk-ld2450

### Home Assistant Zigbee
- ZHA: https://www.home-assistant.io/integrations/zha/
- Zigbee2MQTT: https://www.zigbee2mqtt.io/

---

**Status**: Ready to proceed with Phase 1 (Environment Setup)
**Estimated Total Time**: 2-3 weeks to first production deployment
