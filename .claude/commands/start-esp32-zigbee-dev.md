# ESP32 Zigbee Development Startup

Initialize agent context for ESP32-C6 Zigbee firmware development.

**Purpose:** Load ESP32 Zigbee development skill and set up working environment.

---

## Load ESP32 Zigbee Development Skill

**Skill Location:** `/home/homelab/homelab-esp32-/.claude/skills/esp32-zigbee-dev/SKILL.md`

**Critical Knowledge Loaded:**
- PlatformIO project structure requirements (src/ not main/)
- ESP-Zigbee-SDK 1.0.9 API breaking changes
- Component dependency management
- Build system troubleshooting
- Zigbee network integration with Home Assistant ZHA

---

## Project Context

```bash
echo "=== ESP32-C6 Zigbee Multi-Sensor Project ==="
echo ""
cd /home/homelab/homelab-esp32- 2>/dev/null || echo "ERROR: homelab-esp32- repo not found"
echo "📁 Working Directory: $(pwd)"
echo ""
echo "📊 Git Status:"
git status -sb 2>/dev/null || echo "Not a git repo"
echo ""
echo "📝 Recent Commits (last 3):"
git log --oneline -3 2>/dev/null || echo "No commits"
```

---

## Hardware Inventory

```bash
echo ""
echo "=== Hardware Status ==="
echo ""
echo "ESP32-C6 Boards:"
echo "  • Waveshare ESP32-C6-Zero: ✅ In stock (2+ units)"
echo ""
echo "Sensors - Integrated & Working:"
echo "  • BH1750 Illuminance (I2C): ✅ Operational"
echo "  • DS18B20 Temperature (1-Wire): ✅ Operational"
echo "  • DHT11 Temp/Humidity (1-Wire): ✅ Operational"
echo ""
echo "Sensors - Ready for Integration:"
echo "  • HLK-LD2450 24GHz mmWave Radar (UART): 📦 In Stock (3 units)"
echo ""
echo "Home Assistant Integration:"
echo "  • ZHA Coordinator: SONOFF Zigbee 3.0 USB Dongle"
echo "  • ESP32-Zigbee-Dev: ✅ Paired, reporting to HA"
echo "  • Entity Count: 4 sensors (temp indoor, temp outdoor, humidity, illuminance)"
```

---

## Project Structure

```bash
echo ""
echo "=== Project Structure ==="
echo ""
ls -lh /home/homelab/homelab-esp32- 2>/dev/null | grep -E "^d" | awk '{print "  " $9}'
echo ""
echo "Key Directories:"
echo "  • zigbee-multi-sensor/    - Production firmware (Phase 3 complete)"
echo "  • tests/                  - Sensor testing code"
echo "  • docs/                   - Project documentation"
echo "  • .claude/skills/         - ESP32 Zigbee development skill"
```

---

## Development Phase Status

```bash
echo ""
echo "=== Current Development Phase ==="
echo ""
echo "Phase 1: Environment Setup         ✅ COMPLETE"
echo "  • VS Code + PlatformIO configured"
echo "  • ESP-IDF 5.5.0 installed"
echo "  • ESP32-C6 USB connectivity verified"
echo ""
echo "Phase 2: Sensor Testing             ✅ COMPLETE"
echo "  • BH1750 (I2C) tested"
echo "  • DS18B20 (1-Wire Dallas) tested"
echo "  • DHT11 (1-Wire DHT) tested"
echo "  • Multi-sensor integration verified"
echo ""
echo "Phase 3: Zigbee Integration         ✅ COMPLETE"
echo "  • ESP-IDF Zigbee stack initialized"
echo "  • Multi-endpoint architecture (EP 10, 11, 12, 14)"
echo "  • Successfully paired with Home Assistant ZHA"
echo "  • All sensors reporting to HA entities"
echo "  • WS2812 RGB LED visual indicators"
echo "  • Runtime-switchable reporting modes"
echo ""
echo "Phase 4: HLK-LD2450 Integration     ⏳ NEXT"
echo "  • UART protocol implementation"
echo "  • Binary packet parsing"
echo "  • Custom Zigbee cluster (0xFC00)"
echo "  • Multi-target tracking"
echo ""
echo "Phase 5: Production Deployment      ⏳ FUTURE"
echo "  • PCB design"
echo "  • Enclosure design"
echo "  • OTA firmware update"
echo "  • Watchdog + auto-reconnect"
```

---

## Quick Reference

```bash
echo ""
echo "=== Quick Commands ==="
echo ""
echo "Build and Upload:"
echo "  cd /home/homelab/homelab-esp32-/zigbee-multi-sensor"
echo "  pio run                    # Build firmware"
echo "  pio run --target upload    # Upload to ESP32-C6"
echo "  pio device monitor         # Serial monitor"
echo ""
echo "Clean Build:"
echo "  pio run --target fullclean"
echo ""
echo "Factory Reset (Erase NVS):"
echo "  pio run --target erase"
echo ""
echo "Monitor with GUI:"
echo "  python3 monitor.py"
```

---

## Critical Reminders

```bash
echo ""
echo "=== Critical Knowledge ==="
echo ""
echo "⚠️  PlatformIO MUST use src/ directory (NOT main/)"
echo "⚠️  ESP-Zigbee-SDK requires BOTH libraries:"
echo "    - esp-zigbee-lib (high-level API)"
echo "    - esp-zboss-lib (low-level stack)"
echo "⚠️  ESP-Zigbee-SDK 1.0.9 has breaking API changes"
echo "⚠️  Create sensor tasks BEFORE esp_zb_main_loop_iteration()"
echo "⚠️  Use logarithmic encoding for illuminance values"
```

---

## Documentation Links

```bash
echo ""
echo "=== Documentation ==="
echo ""
echo "Local Docs:"
echo "  • Skill: /home/homelab/homelab-esp32-/.claude/skills/esp32-zigbee-dev/SKILL.md"
echo "  • Firmware README: /home/homelab/homelab-esp32-/zigbee-multi-sensor/README.md"
echo "  • Project Docs: /home/homelab/homelab-esp32-/docs/"
echo ""
echo "Official Resources:"
echo "  • ESP-Zigbee-SDK: https://github.com/espressif/esp-zigbee-sdk"
echo "  • ESP-IDF Guide: https://docs.espressif.com/projects/esp-idf/"
echo "  • ZCL Spec: https://zigbeealliance.org/developer_resources/"
```

---

## Home Assistant Integration

```bash
echo ""
echo "=== Home Assistant Status ==="
echo ""
ping -c 1 -W 1 10.0.1.150 &>/dev/null && echo "  • HA Host: ONLINE ✓" || echo "  • HA Host: OFFLINE ✗"
echo "  • HA Address: http://10.0.1.150:8123"
echo "  • Integration: ZHA (Zigbee Home Automation)"
echo "  • Coordinator: SONOFF Zigbee 3.0 USB Dongle"
echo "  • ESP32 Device: ESP32-Zigbee-Dev (paired)"
```

---

## Working Directory

```bash
echo ""
echo "=== Ready for ESP32 Zigbee Development ==="
echo ""
cd /home/homelab/homelab-esp32-
echo "Current directory: $(pwd)"
echo ""
echo "Suggested Next Steps:"
echo "  1. Review skill: cat .claude/skills/esp32-zigbee-dev/SKILL.md"
echo "  2. Check firmware status: cd zigbee-multi-sensor && pio run"
echo "  3. Monitor device: python3 monitor.py"
echo "  4. Plan HLK-LD2450 integration (Phase 4)"
echo ""
echo "Focus: ESP32-C6 Zigbee firmware development with Home Assistant integration"
echo "Hardware Ready: 3 sensors operational + 3 mmWave radars in stock"
```

---

**Context Initialized:** ESP32-C6 Zigbee Development
**Skill Loaded:** esp32-zigbee-dev (v1.0.0)
**Working Directory:** `/home/homelab/homelab-esp32-/`
