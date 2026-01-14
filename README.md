# HomeLab ESP32 Zigbee Multi-Sensor

**Hardware:** Waveshare ESP32-C6-Zero
**Purpose:** Zigbee router/repeater with multi-sensor integration for Home Assistant
**Framework:** Native ESP-IDF (production) / Arduino (testing)
**Project:** Home Assistant IoT Sensor Network

---

## 📋 Project Status

**Current Phase:** Zigbee Integration (Phase 3)
**Sprint:** HA-S1 Foundation Setup
**Decision:** Native ESP-IDF approach confirmed (ESPHome lacks Zigbee support)

| Phase | Status | Notes |
|-------|--------|-------|
| Environment Setup | ✅ Complete | VS Code + PlatformIO working |
| Sensor Testing | ✅ Complete | BH1750, DS18B20, DHT11 all operational |
| Multi-Sensor Integration | ✅ Complete | All 3 sensors working simultaneously |
| Zigbee Integration | 🔄 In Progress | ESP-IDF Zigbee stack integration |
| HLK-LD2450 Integration | ⏳ Pending | 3 units in stock, awaiting integration |
| Production Deployment | ⏳ Pending | PCB design + enclosure |

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
├── tests/                      # Sensor test projects (Arduino)
│   ├── bh1750-test/            # BH1750 illuminance
│   ├── ds18b20-test/           # DS18B20 outdoor temp
│   ├── dht11-test/             # DHT11 indoor temp/humidity
│   ├── multi-sensor-test/      # All sensors together
│   ├── hello-world/            # Basic UART test
│   └── neopixel-rgb/           # RGB LED test
│
├── zigbee-multi-sensor/        # Production firmware (ESP-IDF)
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

### Test First Sensor: BH1750

```bash
cd tests/bh1750-test
pio run --target upload && pio device monitor
```

See test README files for wiring instructions.

---

## 🧪 Sensor Testing Order

**Start here:**
1. **BH1750** (easiest - I2C digital)
2. **DS18B20** (1-Wire digital)
3. **DHT11** (1-Wire, timing sensitive)
4. **Multi-sensor** (all together)

Each test directory has:
- `README.md` - Wiring diagram, setup instructions
- `platformio.ini` - Build configuration
- `src/main.cpp` - Test code

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

## 🔗 Related Projects

- **[homeassistant](https://github.com/unmanned-systems-uk/homeassistant)** - HA configuration
- **[HomeLab](https://github.com/unmanned-systems-uk/homelab)** - Infrastructure

---

**Last Updated:** 2026-01-14
