# ESP32-C6 Zigbee Multi-Sensor Documentation

**Project:** homelab-esp32-
**Hardware:** Waveshare ESP32-C6-Zero
**Framework:** Native ESP-IDF (FreeRTOS)
**Purpose:** Zigbee router/repeater with multi-sensor integration for Home Assistant

---

## 📖 Documentation Structure

### Project Planning & Architecture

| Document | Description |
|----------|-------------|
| [ESP32 Zigbee Approach Analysis](esp32-zigbee-approach-analysis.md) | **Decision analysis** - ESPHome vs Native ESP-IDF comparison, production requirements, custom cluster needs |
| [ESP32 Zigbee Development Plan](esp32-zigbee-development-plan.md) | **Full roadmap** - Phase-by-phase development plan, hardware BOM, testing procedures |
| [ESP32 Zigbee Sprint Tasks](esp32-zigbee-sprint-tasks.md) | **Task breakdown** - Granular sprint tasks for CCPM tracking |

### Hardware Documentation

| Document | Description |
|----------|-------------|
| [Waveshare ESP32-C6-Zero Notes](hardware/waveshare-esp32-c6-zero-notes.md) | **Hardware reference** - Pin assignments, specifications, PlatformIO config, development workflow |

### Sensor Integration

| Document | Description |
|----------|-------------|
| [DS18B20 Integration Guide](sensors/ds18b20-integration-guide.md) | **1-Wire temperature sensor** - Wiring, code examples, troubleshooting |

---

## 🎯 Quick Navigation

### Getting Started
1. Read: [ESP32 Zigbee Approach Analysis](esp32-zigbee-approach-analysis.md) - Understand why native ESP-IDF
2. Read: [Waveshare ESP32-C6-Zero Notes](hardware/waveshare-esp32-c6-zero-notes.md) - Hardware setup
3. Follow: [ESP32 Zigbee Development Plan](esp32-zigbee-development-plan.md) - Step-by-step guide

### Active Development
- **Current Phase:** Zigbee Integration (Phase 3)
- **Sprint Tasks:** See [ESP32 Zigbee Sprint Tasks](esp32-zigbee-sprint-tasks.md)
- **Test Code:** `../tests/bh1750-test/src/main.c` (multi-sensor integration)

### Hardware Status
- ✅ **Tested:** BH1750 (I2C), DS18B20 (1-Wire), DHT11 (1-Wire)
- 📦 **In Stock:** 3x HLK-LD2450 24GHz mmWave radar
- 🔄 **Next:** Zigbee stack integration

---

## 📋 Documentation Guidelines

### Adding New Documentation

**Project guides** → Root `docs/` directory
- Development plans, architecture decisions, approach analysis

**Hardware docs** → `docs/hardware/`
- Board specifications, pinouts, wiring diagrams, PCB designs

**Sensor guides** → `docs/sensors/`
- Sensor-specific integration guides, wiring, code examples

**General guides** → `docs/guides/`
- Setup instructions, troubleshooting, workflows

### File Naming Convention

- Use kebab-case: `esp32-zigbee-development-plan.md`
- Be descriptive: `waveshare-esp32-c6-zero-notes.md` not `board-notes.md`
- Include scope: `ds18b20-integration-guide.md` not just `temperature-sensor.md`

---

## 🔗 External References

### Official Documentation
- **ESP-IDF:** https://docs.espressif.com/projects/esp-idf/
- **ESP-Zigbee-SDK:** https://docs.espressif.com/projects/esp-zigbee-sdk/
- **Waveshare ESP32-C6-Zero:** https://www.waveshare.com/wiki/ESP32-C6-Zero

### Home Assistant Integration
- **ZHA:** https://www.home-assistant.io/integrations/zha/
- **Zigbee2MQTT:** https://www.zigbee2mqtt.io/

### Related Repositories
- **Home Assistant Config:** https://github.com/unmanned-systems-uk/homeassistant
- **HomeLab Infrastructure:** https://github.com/unmanned-systems-uk/homelab

---

## 🗂️ Directory Structure

```
docs/
├── README.md                              # This file
├── esp32-zigbee-approach-analysis.md      # ESPHome vs ESP-IDF decision
├── esp32-zigbee-development-plan.md       # Full development roadmap
├── esp32-zigbee-sprint-tasks.md           # Sprint task breakdown
│
├── hardware/
│   └── waveshare-esp32-c6-zero-notes.md   # Hardware specifications & pinout
│
├── sensors/
│   └── ds18b20-integration-guide.md       # DS18B20 temperature sensor guide
│
└── guides/                                # General how-to guides (TBD)
```

---

**Last Updated:** 2026-01-14
**Maintained by:** HomeAssistant-Agent
