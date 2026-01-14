# HomeLab ESP32 Zigbee Multi-Sensor

**Hardware:** Waveshare ESP32-C6-Zero
**Purpose:** Zigbee router/repeater with multi-sensor integration for Home Assistant
**Framework:** Native ESP-IDF (production) / Arduino (testing)
**Project:** Home Assistant IoT Sensor Network

---

## 📋 Project Status

**Current Phase:** Infrastructure Hardening (Phase 4-7)
**Sprint:** HA-S1 Foundation Setup
**Decision:** Native ESP-IDF approach confirmed (ESPHome lacks Zigbee support)

| Phase | Status | Notes |
|-------|--------|-------|
| **Phase 1:** Environment Setup | ✅ Complete | VS Code + PlatformIO working |
| **Phase 2:** Sensor Testing | ✅ Complete | BH1750, DS18B20, DHT11 all operational |
| **Phase 3:** Zigbee Integration | ✅ Complete | Device paired to HA, all sensors reporting |
| **Phase 4-7:** Infrastructure Hardening | 🔄 In Progress | Watchdog, OTA, stability testing |
| **Phase 8:** HLK-LD2450 Integration | ⏳ Pending | 3 units in stock, awaiting integration |
| **Phase 9:** Production Deployment | ⏳ Pending | PCB design + enclosure |

---

## 🔧 Hardware

### Waveshare ESP32-C6-Zero

- **MCU:** ESP32-C6 (RISC-V 32-bit @ 160MHz)
- **Wireless:** Zigbee 3.0, WiFi 802.11n, BLE 5.0
- **Flash:** 4MB | **RAM:** 512KB SRAM
- **Form Factor:** Compact module (25.4mm × 20.8mm)
- **USB:** USB-C (programming + power)

### Sensors

| Sensor | Type | Interface | Purpose | Status |
|--------|------|-----------|---------|--------|
| **BH1750** | Illuminance | I2C | Light level (0-65535 lux) | ✅ Tested |
| **DS18B20** | Temperature | 1-Wire | Outdoor temp (-55°C to +125°C) | ✅ Tested |
| **DHT11** | Temp/Humidity | 1-Wire | Indoor climate (±2°C, ±5% RH) | ✅ Tested |
| **HLK-LD2450** | mmWave Radar | UART | Presence + trajectory (3 units) | 📦 In Stock |

### Pin Assignments

| GPIO | Function | Sensor | Notes |
|------|----------|--------|-------|
| GPIO1 | I2C SDA | BH1750 | 4.7kΩ pull-up |
| GPIO2 | I2C SCL | BH1750 | 4.7kΩ pull-up |
| GPIO4 | DHT11 Data | DHT11 (Indoor) | 10kΩ pull-up |
| GPIO5 | DS18B20 Data | DS18B20 (Outdoor) | 4.7kΩ pull-up |
| GPIO16 | UART RX | HLK-LD2450 TX | 115200 baud |
| GPIO17 | UART TX | HLK-LD2450 RX | 115200 baud |
| GPIO15 | LED Status | Built-in LED | Pre-wired |

---

## 📁 Repository Structure

```
homelab-esp32/
├── tests/                      # Sensor test projects (ESP-IDF)
│   └── multi-sensor-test/      # BH1750 + DS18B20 + DHT11 (all 3 sensors)
│
├── zigbee-multi-sensor/        # Production firmware (ESP-IDF + Zigbee)
│   ├── src/                    # Source code
│   ├── include/                # Headers
│   └── platformio.ini          # ESP-IDF config
│
├── docs/                       # Documentation
│   ├── sensors/                # Sensor guides
│   ├── hardware/               # Wiring diagrams
│   └── guides/                 # How-to guides
│
├── schematics/                 # PCB designs
├── binaries/                   # Firmware releases
└── README.md                   # This file
```

---

## 🚀 Quick Start

### Install Development Tools

```bash
# Install VS Code + PlatformIO
# See docs/guides/setup.md for full instructions

# Clone repository
git clone https://github.com/unmanned-systems-uk/homelab-esp32-.git
cd homelab-esp32-
```

### Test All Sensors: Multi-Sensor Test

```bash
cd tests/multi-sensor-test
pio run --target upload && pio device monitor
```

**Tests all 3 sensors simultaneously:**
- BH1750 (light sensor via I2C)
- DS18B20 (outdoor temperature via 1-Wire)
- DHT11 (indoor temperature/humidity via 1-Wire)

See `tests/multi-sensor-test/README.md` for complete wiring instructions and testing procedure.

---

## 🧪 Sensor Testing Status

**All sensors validated and working:**
- ✅ **BH1750** - I2C digital light sensor
- ✅ **DS18B20** - 1-Wire outdoor temperature
- ✅ **DHT11** - 1-Wire indoor temp/humidity
- ✅ **Multi-sensor integration** - All 3 sensors working simultaneously

Test directory:
- `tests/multi-sensor-test/` - Complete integrated test
  - `README.md` - Wiring diagram, setup, troubleshooting
  - `platformio.ini` - ESP-IDF build configuration
  - `src/main.c` - Multi-sensor test code

---

## 🏠 Home Assistant Integration

**Zigbee Endpoints:**

| EP | Sensor | Zigbee Clusters |
|----|--------|-----------------|
| 10 | DHT11 (Indoor) | Temperature, Humidity |
| 11 | DS18B20 (Outdoor) | Temperature |
| 12 | BH1750 (Light) | Illuminance |
| 13 | LD2450 (Presence) | Occupancy, Custom Trajectory |

**Expected HA Entities:**
- `sensor.esp32_multisensor_indoor_temperature`
- `sensor.esp32_multisensor_outdoor_temperature`
- `sensor.esp32_multisensor_humidity`
- `sensor.esp32_multisensor_illuminance`
- `binary_sensor.esp32_multisensor_presence`

---

## 📚 Documentation

### Project Guides
- **[ESP32 Zigbee Approach Analysis](docs/esp32-zigbee-approach-analysis.md)** - ESPHome vs ESP-IDF decision
- **[ESP32 Zigbee Development Plan](docs/esp32-zigbee-development-plan.md)** - Full development roadmap
- **[ESP32 Zigbee Sprint Tasks](docs/esp32-zigbee-sprint-tasks.md)** - Task breakdown

### Hardware Documentation
- **[Waveshare ESP32-C6-Zero Notes](docs/hardware/waveshare-esp32-c6-zero-notes.md)** - Pinout, wiring, specifications

### Sensor Integration Guides
- **[DS18B20 Integration Guide](docs/sensors/ds18b20-integration-guide.md)** - 1-Wire temperature sensor setup

---

## 🎯 Project Goals

1. ✅ **Reliable 24/7 operation** - Watchdog, auto-reconnect
2. ✅ **Multi-sensor integration** - Indoor/outdoor climate, presence
3. ✅ **Zigbee mesh repeater** - Extend network coverage
4. ✅ **OTA firmware updates** - Via Zigbee coordinator
5. ✅ **Replicable design** - Template for multiple devices

---

## 🚧 Infrastructure Improvements To-Do

### Priority 1: Production Hardening (Phase 6) ⭐ RECOMMENDED NEXT
**Goal:** Make the device production-ready for 24/7 reliability

- [ ] **Watchdog Timer Implementation**
  - [ ] Configure ESP32 hardware watchdog (30s timeout)
  - [ ] Add watchdog reset calls in main loop
  - [ ] Test crash recovery behavior
  - [ ] Log watchdog reset events

- [ ] **Network Reconnection Logic**
  - [ ] Detect Zigbee network loss
  - [ ] Implement exponential backoff retry (1s, 2s, 4s, 8s, max 60s)
  - [ ] Track connection uptime statistics
  - [ ] LED visual feedback for connection states

- [ ] **Factory Reset Mechanism**
  - [ ] Detect GPIO0 (boot button) held for 10 seconds
  - [ ] Erase Zigbee NVS storage
  - [ ] Erase WiFi credentials (if applicable)
  - [ ] LED flash pattern confirmation
  - [ ] Automatic restart after reset

- [ ] **Error Handling & Resilience**
  - [ ] Graceful sensor failure handling (continue with remaining sensors)
  - [ ] I2C bus recovery on communication failure
  - [ ] 1-Wire bus recovery mechanisms
  - [ ] Memory leak detection and prevention

**Estimated Time:** 1 week
**CCPM Tasks:** [ESP32-P6] Implement watchdog timer, network reconnection, factory reset

---

### Priority 2: OTA Firmware Updates (Phase 5)
**Goal:** Enable remote firmware updates without physical access

- [ ] **WiFi OTA for Development**
  - [ ] Implement ArduinoOTA library integration
  - [ ] Add WiFi fallback mode (if Zigbee fails)
  - [ ] Password-protected OTA access
  - [ ] Test update procedure (with rollback)

- [ ] **Zigbee OTA for Production**
  - [ ] ✅ Configure OTA partition table (already done)
  - [ ] Implement Zigbee OTA cluster (0x0019)
  - [ ] Test OTA via Home Assistant ZHA
  - [ ] Verify rollback on failed update
  - [ ] Document OTA procedure for users

- [ ] **OTA Safety Features**
  - [ ] Version checking (prevent downgrades)
  - [ ] Firmware signature verification (optional)
  - [ ] Battery level check (if battery-powered in future)
  - [ ] Automatic rollback on boot failure (>3 failed boots)

**Estimated Time:** 1 week
**CCPM Tasks:** [ESP32-P5] WiFi OTA, Zigbee OTA

---

### Priority 3: Stability & Testing (Phase 7)
**Goal:** Validate reliability before production deployment

- [ ] **24-Hour Stability Test**
  - [ ] Monitor for crashes, reboots, memory leaks
  - [ ] Log all sensor readings to SD card or serial
  - [ ] Track heap usage over time
  - [ ] Verify Zigbee attribute reporting consistency

- [ ] **Stress Testing**
  - [ ] Rapid sensor polling (1s intervals)
  - [ ] Network congestion simulation
  - [ ] Temperature cycling tests (-10°C to +50°C)
  - [ ] Power cycling tests (100 cycles)

- [ ] **Performance Profiling**
  - [ ] CPU usage monitoring (target <50% average)
  - [ ] Memory usage analysis (heap fragmentation)
  - [ ] Zigbee latency measurements
  - [ ] Sensor read timing optimization

- [ ] **Bug Fixes & Optimization**
  - [ ] Address issues found during testing
  - [ ] Optimize sensor polling intervals
  - [ ] Reduce power consumption (if battery-powered)
  - [ ] Improve error messages and logging

**Estimated Time:** 1-2 weeks
**CCPM Tasks:** [ESP32-P7] 24-hour stability test, performance optimization

---

### Priority 4: Web UI for Monitoring & Configuration 🌐 NEW
**Goal:** Replace unreliable serial monitoring with web-based interface

- [ ] **Embedded Web Server**
  - [ ] Implement HTTP server (ESP-IDF `esp_http_server`)
  - [ ] Serve static HTML/CSS/JavaScript
  - [ ] WebSocket for real-time sensor updates
  - [ ] mDNS discovery (esp32-zigbee.local)

- [ ] **Dashboard Features**
  - [ ] Real-time sensor readings (BH1750, DS18B20, DHT11)
  - [ ] Zigbee network status (connected, channel, PAN ID, LQI, RSSI)
  - [ ] Device information (firmware version, uptime, heap usage)
  - [ ] Historical data charts (last 1hr, 24hr, 7d)

- [ ] **Configuration Interface**
  - [ ] Sensor calibration offsets (temperature, humidity)
  - [ ] Reporting interval adjustments
  - [ ] Zigbee channel selection
  - [ ] WiFi credentials (for dual-mode operation)
  - [ ] Factory reset button

- [ ] **Advanced Features**
  - [ ] Firmware update via web upload
  - [ ] Network diagnostics (ping coordinator, scan neighbors)
  - [ ] Export data as CSV/JSON
  - [ ] Dark mode toggle
  - [ ] Mobile-responsive design

- [ ] **Security**
  - [ ] HTTP Basic Auth or session cookies
  - [ ] HTTPS support (optional, self-signed cert)
  - [ ] API rate limiting
  - [ ] CSRF protection

**Estimated Time:** 2-3 weeks
**Tech Stack:** ESP-IDF HTTP server, Chart.js, vanilla JavaScript
**Reference:** ESP-IDF examples (`protocols/http_server`)

---

### Priority 5: Reporting & Configuration (Phase 4)
**Goal:** Optimize Zigbee data flow and configurability

- [ ] **Tune Reporting Intervals**
  - [ ] Test different intervals (10s, 30s, 60s, 5min)
  - [ ] Measure network impact (packet rate, collisions)
  - [ ] Balance responsiveness vs power consumption
  - [ ] Document optimal settings

- [ ] **Report on Change (Delta Reporting)**
  - [ ] Only send updates when value changes >threshold
  - [ ] Temperature: ±0.5°C
  - [ ] Humidity: ±2%
  - [ ] Illuminance: ±10%
  - [ ] Reduce network traffic by 50-70%

- [ ] **Configurable Attributes via Home Assistant**
  - [ ] Reporting interval (number entity)
  - [ ] Delta thresholds (number entity)
  - [ ] Sensor enable/disable (switch entity)
  - [ ] Temperature offset (number entity)

- [ ] **Zigbee Binding Support**
  - [ ] Allow lights to bind to illuminance sensor
  - [ ] Direct device-to-device control (no coordinator)
  - [ ] Faster response times for automations

**Estimated Time:** 1 week
**CCPM Tasks:** [ESP32-P4] Test and tune reporting intervals

---

### Priority 6: Advanced Zigbee Features (Optional)
**Goal:** Enhanced mesh performance and diagnostics

- [ ] **Network Diagnostics as Sensors**
  - [ ] LQI (Link Quality Indicator) sensor
  - [ ] RSSI (signal strength) sensor
  - [ ] Neighbor count sensor
  - [ ] Parent device information

- [ ] **Power Management**
  - [ ] Sleep modes for battery operation (future)
  - [ ] Wake on Zigbee command
  - [ ] Deep sleep between readings (if battery-powered)

- [ ] **Zigbee Group Addressing**
  - [ ] Support group commands (all lights in room)
  - [ ] Scene support
  - [ ] Broadcast commands

- [ ] **Multiple Coordinators**
  - [ ] Support mesh topology with multiple coordinators
  - [ ] Automatic parent selection (best signal)
  - [ ] Network healing mechanisms

**Estimated Time:** 2-3 weeks (low priority)

---

### Priority 7: Documentation (Phase 8)
**Goal:** Enable others to replicate and deploy

- [ ] **Setup Guides**
  - [ ] VS Code + PlatformIO installation (Windows, macOS, Linux)
  - [ ] ESP-IDF version compatibility matrix
  - [ ] USB driver installation
  - [ ] Project structure explanation

- [ ] **Build & Flash Guide**
  - [ ] Clone repository steps
  - [ ] Dependency installation
  - [ ] Build commands
  - [ ] Upload troubleshooting

- [ ] **Home Assistant Integration Guide**
  - [ ] ZHA setup and configuration
  - [ ] Zigbee network pairing procedure
  - [ ] Entity customization (friendly names, icons)
  - [ ] Automation examples

- [ ] **Deployment Checklist**
  - [ ] Pre-deployment testing steps
  - [ ] Installation procedures
  - [ ] Post-deployment validation
  - [ ] Troubleshooting common issues

- [ ] **Troubleshooting Guide**
  - [ ] Connection failures
  - [ ] Sensor read errors
  - [ ] OTA update failures
  - [ ] Factory reset procedure

**Estimated Time:** 1 week
**CCPM Tasks:** [ESP32-P8] Write setup, build, integration, deployment, troubleshooting guides

---

### Priority 8: HLK-LD2450 mmWave Integration (Phase 8)
**Goal:** Add presence detection and trajectory tracking

*(Deferred until infrastructure is solid)*

- [ ] **UART Protocol Implementation**
  - [ ] Research binary packet structure
  - [ ] Implement packet parser (header, data, checksum)
  - [ ] Multi-target tracking (up to 3 simultaneous)
  - [ ] X/Y coordinate mapping

- [ ] **Custom Zigbee Cluster (0xFC00)**
  - [ ] Define custom attributes (X, Y, distance, speed)
  - [ ] Implement cluster server
  - [ ] Test with Home Assistant custom integration

- [ ] **Add Endpoint 13**
  - [ ] Occupancy cluster (0x0406)
  - [ ] Custom trajectory cluster (0xFC00)
  - [ ] Test and validate

**Estimated Time:** 2-3 weeks
**CCPM Tasks:** [ESP32-P2] LD2450 UART/parser, [ESP32-P3] Custom cluster

---

### Priority 9: Production Deployment (Phase 9)
**Goal:** Physical design and manufacturing

- [ ] **PCB Design**
  - [ ] Schematic capture (KiCad)
  - [ ] PCB layout (4-layer recommended)
  - [ ] Component placement optimization
  - [ ] Design review and DRC checks

- [ ] **3D Printed Enclosure**
  - [ ] CAD design (sensor mounting, ventilation)
  - [ ] Prototype printing (PETG or ABS)
  - [ ] Test fit and thermal testing
  - [ ] Iterate design

- [ ] **First Production Run**
  - [ ] Order 5-10 units
  - [ ] Assembly and testing
  - [ ] Deployment to locations
  - [ ] Monitor for issues

**Estimated Time:** 4-6 weeks
**CCPM Tasks:** [ESP32-P9] PCB design, enclosure, production deployment

---

## 🔗 Related Projects

- **[homeassistant](https://github.com/unmanned-systems-uk/homeassistant)** - HA configuration
- **[HomeLab](https://github.com/unmanned-systems-uk/homelab)** - Infrastructure

---

**Last Updated:** 2026-01-14 (Infrastructure improvements added)
