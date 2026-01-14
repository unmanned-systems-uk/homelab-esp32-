# ESP32 Zigbee Multi-Sensor - Sprint Tasks

**Project:** Home Assistant
**Sprint:** HA-S1: Foundation Setup
**GitHub Issue:** [#10](https://github.com/unmanned-systems-uk/homeassistant/issues/10)
**Total Tasks:** 31
**Created:** 2026-01-10

---

## Task Organization

Tasks are organized into 9 phases (P1-P9) representing the development lifecycle from environment setup through production deployment.

### Phase Summary

| Phase | Name | Tasks | Focus |
|-------|------|-------|-------|
| **P1** | Environment Setup | 3 | VS Code, PlatformIO, debugging framework |
| **P2** | Sensor Integration | 4 | BH1750, DHT11, HLK-LD2450 (hardware focus) |
| **P3** | Zigbee Integration | 5 | ESP-IDF, Zigbee stack, clusters (complex software) |
| **P4** | Multi-Sensor | 2 | Unified reporting, interval tuning |
| **P5** | OTA Updates | 3 | Partition table, WiFi OTA, Zigbee OTA |
| **P6** | Production Hardening | 4 | Watchdog, reconnection, reset, LED |
| **P7** | Testing & Optimization | 2 | 24h stability test, bug fixes |
| **P8** | Documentation | 5 | Setup, build, integration, deployment guides |
| **P9** | Hardware & Deployment | 3 | 3D enclosure, PCB design, first deployment |

---

## Phase 1: Environment Setup (Critical Foundation)

### [ESP32-P1] Install VS Code + PlatformIO and test ESP32-C6
**Priority:** CRITICAL | **Status:** TODO

Install development environment and verify ESP32-C6 connectivity.

**Key Steps:**
1. Install VS Code
2. Install PlatformIO extension
3. Install USB-UART drivers
4. Create test project
5. Upload "Hello World"
6. Verify serial output

**Success Criteria:** Serial monitor shows "ESP32-C6 Ready!" message

**Note for Electronics Engineers:** PlatformIO may seem complex initially, but it's worth the learning curve for production work. Think of it as the "proper tooling" equivalent in software.

---

### [ESP32-P1] Create project structure and configure platformio.ini
**Priority:** HIGH | **Status:** TODO

Set up professional modular project structure.

**Creates:**
- `src/` - Source code files
- `include/` - Header files
- `lib/` - Custom libraries
- `docs/` - Documentation

**Success Criteria:** Project builds cleanly with organized file structure

---

### [ESP32-P1] Implement basic serial debugging framework
**Priority:** MEDIUM | **Status:** TODO

Create debug logging system with levels (ERROR, WARN, INFO, DEBUG).

**Why This Matters:** Good debug output is critical when software debugging gets difficult. This framework will help throughout the project.

**Success Criteria:** Consistent debug messages with timestamps and module names

---

## Phase 2: Sensor Integration (Hardware → Software)

### [ESP32-P2] Integrate BH1750 illuminance sensor (I2C)
**Priority:** HIGH | **Status:** TODO

**START HERE** - This is the easiest sensor to build confidence.

**Hardware:**
- SDA → GPIO4
- SCL → GPIO5
- I2C protocol (simple)

**Software:**
- Library: `claws/BH1750@^1.3.0`
- Simple API: `lightMeter.readLightLevel()`

**Test:** Cover sensor (0 lux) vs flashlight (>1000 lux)

**Success Criteria:** Lux values respond to light changes

---

### [ESP32-P2] Integrate DHT11 temperature/humidity sensor (1-Wire)
**Priority:** HIGH | **Status:** TODO

**Medium difficulty** - timing-sensitive protocol.

**Hardware:**
- DATA → GPIO6
- 10kΩ pull-up resistor

**Software:**
- Library: `adafruit/DHT sensor library@^1.4.4`
- Minimum 2-second polling interval

**Gotcha:** May conflict with Zigbee timing later - we'll test this thoroughly.

**Success Criteria:** Stable temp (~20-25°C) and humidity (30-60%) readings

---

### [ESP32-P2] Implement HLK-LD2450 UART communication
**Priority:** HIGH | **Status:** TODO

Establish UART connection and receive raw data.

**Hardware:**
- Sensor TX → ESP32 RX (GPIO16)
- Sensor RX → ESP32 TX (GPIO17)
- 115200 baud

**Software Focus:** Get raw UART data first, don't worry about parsing yet.

**Test:** Print raw hex bytes, verify data stream changes when moving

**Success Criteria:** Raw UART data received, packet headers visible (0xAA 0xFF)

---

### [ESP32-P2] Implement HLK-LD2450 binary protocol parser
**Priority:** HIGH | **Status:** TODO

**Most Complex Sensor** - Binary protocol requires careful parsing.

**Protocol:**
- Header: `0xAA 0xFF 0x03 0x00`
- Multi-target tracking (up to 3 targets)
- X/Y coordinates, distance, speed
- Checksum validation

**Software Challenge:** State machine for packet parsing, binary data handling.

**Take your time here** - this is the hardest software task in Phase 2.

**Success Criteria:** Presence detection works, X/Y coordinates track movement

---

## Phase 3: Zigbee Integration (Major Learning Curve)

### [ESP32-P3] Migrate to ESP-IDF framework for Zigbee support
**Priority:** ~~CRITICAL~~ N/A | **Status:** ✅ SKIPPED

**DECISION UPDATE:** Started with ESP-IDF framework from the beginning (no migration needed).

**Using ESP-IDF from day one provides:**
- Native Zigbee support without framework conversion
- Production-ready patterns from the start
- Single learning path (no context switching)
- Direct access to FreeRTOS and ESP-IDF features

**ESP-IDF Key Concepts:**
- `app_main()` - Entry point (replaces Arduino setup/loop)
- FreeRTOS tasks - Concurrent sensor reading
- `ESP_LOGI()` - Logging macros
- ESP-IDF component drivers - Native sensor libraries

**Success Criteria:** ✅ ESP-IDF environment configured and operational

---

### [ESP32-P3] Initialize Zigbee stack and join network
**Priority:** CRITICAL | **Status:** TODO

Initialize Zigbee and join existing network.

**Configuration:**
- Device type: **Router** (always powered, extends mesh)
- Profile: Home Automation (0x0104)
- Manufacturer: Custom
- Model: ESP32-MultiSensor-V1

**Test:** Put HA coordinator in pairing mode, device should join

**Success Criteria:** Device appears in Home Assistant

---

### [ESP32-P3] Create Zigbee clusters for temperature/humidity
**Priority:** HIGH | **Status:** TODO

Create standard Zigbee clusters for DHT11 data.

**Clusters:**
- 0x0402: Temperature Measurement
- 0x0405: Relative Humidity Measurement

**Data Format:**
- Temperature: int16 in 0.01°C units (2000 = 20.00°C)
- Humidity: uint16 in 0.01% units (5500 = 55.00%)

**Success Criteria:** Temperature and humidity entities appear in HA

---

### [ESP32-P3] Create Zigbee cluster for illuminance
**Priority:** HIGH | **Status:** TODO

Create standard Zigbee cluster for BH1750 light sensor.

**Cluster:**
- 0x0400: Illuminance Measurement

**Data Format:**
- Illuminance: uint16 in lux (0-65535)

**Success Criteria:** Illuminance sensor entity appears in HA

---

### [ESP32-P3] Create custom Zigbee cluster for presence/trajectory
**Priority:** MEDIUM | **Status:** TODO

Create clusters for HLK-LD2450 presence and trajectory data.

**Approach:**
1. Standard Occupancy cluster (0x0406) for binary presence
2. Custom cluster (0xFC00) for trajectory (X/Y coordinates)

**Note:** Custom clusters may need Zigbee2MQTT converter or ZHA quirk for full HA integration.

**Success Criteria:** Presence detection works in HA (trajectory is bonus)

---

## Phase 4: Multi-Sensor Integration

### [ESP32-P4] Integrate all sensors with Zigbee reporting
**Priority:** HIGH | **Status:** TODO

Combine all sensors into unified system.

**Implement:**
- FreeRTOS tasks for each sensor (separate timing)
- Mutex for I2C bus sharing (if needed)
- Error handling (sensor failures shouldn't crash device)
- Watchdog timer

**Test:** All sensors reporting simultaneously, unplug sensors, rapid changes

**Success Criteria:** All 4 sensor types report reliably, no crashes

---

### [ESP32-P4] Test and tune reporting intervals
**Priority:** MEDIUM | **Status:** TODO

Optimize reporting intervals for responsiveness vs network load.

**Default Intervals:**
- Temperature/Humidity: 60s (slow-changing)
- Illuminance: 30s (medium)
- Presence: 1s (critical for automation)
- Trajectory: 2s (less critical)

**Success Criteria:** Presence latency < 1s, acceptable network load

---

## Phase 5: OTA Implementation

### [ESP32-P5] Configure OTA partition table
**Priority:** HIGH | **Status:** TODO

Set up flash partitions for OTA updates.

**Partition Layout:**
- nvs: 24KB
- phy_init: 4KB
- factory: 2MB
- ota_0: 2MB
- ota_1: 2MB

**How OTA Works:** Alternates between ota_0 and ota_1 partitions for safe updates.

**Success Criteria:** Partition table flashed, device boots normally

---

### [ESP32-P5] Implement WiFi OTA for development
**Priority:** MEDIUM | **Status:** TODO

WiFi-based OTA for fast development iteration.

**Benefits:**
- No USB cable during development
- Faster uploads
- Network-based updates

**Can be disabled in production** (Zigbee-only mode).

**Success Criteria:** Upload firmware wirelessly from PlatformIO

---

### [ESP32-P5] Implement Zigbee OTA for production
**Priority:** HIGH | **Status:** TODO

Zigbee-based OTA for production deployment.

**Benefits:**
- No WiFi required
- Updates via HA coordinator
- Production-ready

**Test:** Update firmware via Zigbee (no USB, no WiFi)

**Success Criteria:** Firmware updates via Zigbee coordinator

---

## Phase 6: Production Hardening

### [ESP32-P6] Implement watchdog timer and crash recovery
**Priority:** HIGH | **Status:** TODO

Automatic recovery from crashes for 24/7 operation.

**Implement:**
- 30-second watchdog timeout
- Panic handler logging
- Boot counter (detect crash loops)
- Safe mode after 3 consecutive crashes

**Success Criteria:** Device recovers automatically from crashes

---

### [ESP32-P6] Implement network reconnection logic
**Priority:** HIGH | **Status:** TODO

Automatic Zigbee reconnection after network loss or power cycle.

**Implement:**
- Event handler for network loss
- Exponential backoff retry (1s, 2s, 4s, 8s, max 60s)
- Store network credentials in NVS
- Auto-rejoin on boot

**Success Criteria:** Device rejoins network automatically after power cycle

---

### [ESP32-P6] Implement factory reset mechanism
**Priority:** MEDIUM | **Status:** TODO

Factory reset via boot button (hold 10 seconds).

**Clears:**
- NVS (all settings)
- Zigbee network credentials

**Useful for:** Troubleshooting, redeployment

**Success Criteria:** Factory reset works, device can rejoin as new

---

### [ESP32-P6] Implement status LED indicators
**Priority:** LOW | **Status:** TODO

Visual status indicators for troubleshooting without serial console.

**LED States:**
- Fast blink blue: Booting
- Slow blink yellow: Joining network
- Solid green: Connected
- Fast blink red: Error
- Pulsing cyan: OTA update

**Hardware:** WS2812 RGB LED on GPIO8 or 3x discrete LEDs

**Success Criteria:** LED clearly indicates device state

---

## Phase 7: Testing & Optimization

### [ESP32-P7] 24-hour stability test and monitoring
**Priority:** HIGH | **Status:** TODO

Comprehensive stability test for 24/7 operation validation.

**Monitor:**
- Heap memory (should be stable, no leaks)
- Reconnections (should be minimal)
- Sensor failures (should be rare)
- Crashes/reboots (should be zero)

**Success Criteria:** 24 hours uptime, no crashes, stable memory

---

### [ESP32-P7] Fix bugs and optimize performance
**Priority:** HIGH | **Status:** TODO

Address issues found during stability testing.

**Common Issues:**
- Memory leaks
- Stack overflows
- Sensor timeouts
- DHT11 timing conflicts with Zigbee

**Success Criteria:** All stability test issues resolved

---

## Phase 8: Documentation (Critical for Replication)

### [ESP32-P8] Write VS Code + PlatformIO setup guide
**Priority:** HIGH | **Status:** TODO

Complete setup guide with screenshots.

**Target Audience:** Electronics engineer with limited software experience.

**File:** `docs/01-setup-vscode-platformio.md`

**Success Criteria:** Non-software person can follow and get working environment

---

### [ESP32-P8] Write firmware build and flash guide
**Priority:** HIGH | **Status:** TODO

Step-by-step build and flash guide.

**Covers:**
- Clone repo
- Build firmware
- Flash via USB
- Monitor serial output
- Troubleshooting

**File:** `docs/02-build-and-flash.md`

**Success Criteria:** Firmware can be flashed without prior PlatformIO knowledge

---

### [ESP32-P8] Write Home Assistant integration guide
**Priority:** HIGH | **Status:** TODO

HA integration via Zigbee guide.

**Covers:**
- Pairing device
- Verifying entities
- Configuring settings
- Creating automations
- Troubleshooting

**File:** `docs/03-home-assistant-integration.md`

**Success Criteria:** Device can be integrated with HA following guide

---

### [ESP32-P8] Write deployment checklist
**Priority:** MEDIUM | **Status:** TODO

Pre-deployment, deployment, and post-deployment checklists.

**Ensures:**
- No deployment steps missed
- Consistent deployments
- Quality validation

**File:** `docs/04-deployment-checklist.md`

**Success Criteria:** Checklist prevents deployment errors

---

### [ESP32-P8] Write troubleshooting guide
**Priority:** MEDIUM | **Status:** TODO

Comprehensive troubleshooting for common issues.

**Categories:**
- Build/flash issues
- Sensor issues
- Zigbee issues
- Runtime issues

**File:** `docs/05-troubleshooting.md`

**Success Criteria:** Common problems can be diagnosed and fixed

---

## Phase 9: Hardware & Deployment

### [ESP32-P9] Design and test 3D printed enclosure
**Priority:** MEDIUM | **Status:** TODO

Design custom 3D printed enclosure.

**Requirements:**
- DHT11 ventilation
- BH1750 light window
- LD2450 radar opening
- USB-C access
- Boot button access
- LED visibility
- Wall-mount option

**Deliverables:**
- STL files
- Assembly instructions
- BOM for mounting hardware

**Success Criteria:** Enclosure protects hardware, sensors work correctly

---

### [ESP32-P9] Design PCB layout and order
**Priority:** LOW | **Status:** TODO

**This is your strength as an electronics engineer!**

Custom PCB to replace breadboard.

**Requirements:**
- ESP32-C6 module
- USB-C connector
- I2C bus with pull-ups
- UART for LD2450
- Status LED
- Voltage regulation
- ESD protection

**Deliverables:**
- Schematic PDF
- PCB layout files
- Gerber files
- BOM
- Assembly instructions

**Success Criteria:** PCB works identically to breadboard prototype

---

### [ESP32-P9] First production deployment and validation
**Priority:** HIGH | **Status:** TODO

Deploy first production device and validate over 2+ weeks.

**Validation Metrics:**
- Uptime: >99.9%
- Presence accuracy: >95%
- Temperature accuracy: ±2°C
- Zero crashes

**Deliverables:**
- Installation documentation
- Lessons learned
- Issues and resolutions
- Future deployment recommendations

**Success Criteria:** Device operates reliably for 2+ weeks

---

## Task Dependencies (Suggested Order)

### Week 1: Foundation & Sensors
1. P1 Tasks (1-3): Environment setup
2. P2 Tasks (4-7): Sensor integration

### Week 2: Zigbee Integration
3. P3 Tasks (8-12): Zigbee stack and clusters
4. P4 Tasks (13-14): Multi-sensor integration

### Week 3: OTA & Hardening
5. P5 Tasks (15-17): OTA updates
6. P6 Tasks (18-21): Production hardening
7. P7 Tasks (22-23): Testing and optimization

### Week 4: Documentation & Deployment
8. P8 Tasks (24-28): Complete documentation
9. P9 Task (31): First deployment
10. P9 Tasks (29-30): Hardware design (parallel/ongoing)

---

## Notes for Electronics Engineers

**Your Strengths:**
- Phase 2: Sensor hardware integration (bread and butter)
- Phase 9: PCB design (you'll breeze through this)
- Hardware debugging (oscilloscope, logic analyzer)

**Software Challenges (where to focus learning):**
- Phase 3: ESP-IDF framework (biggest learning curve)
- Phase 3: Zigbee stack (complex software)
- Phase 2: Binary protocol parsing (LD2450)
- Phase 6: FreeRTOS tasks (multithreading concepts)

**Learning Resources:**
- ESP-IDF docs: https://docs.espressif.com/projects/esp-idf/
- PlatformIO docs: https://docs.platformio.org/
- FreeRTOS basics: https://www.freertos.org/
- Zigbee cluster library: https://zigbeealliance.org/

**When Stuck on Software:**
- Use debug logging heavily
- Break problems into smaller pieces
- Test incrementally (don't change too much at once)
- Community forums: ESP32.com, Home Assistant forums

---

## CCPM Tracking

All 31 tasks are tracked in CCPM:
- **Project:** Home Assistant (`15a25f74-b74a-41cd-8250-0abd61e304ca`)
- **Sprint:** HA-S1: Foundation Setup (`86f030d6-09c1-4ddc-8c08-47304c7c11b6`)
- **Assigned To:** Anthony (`7563bfda-6e47-4e50-b37a-90ccdc47311a`)
- **GitHub Issue:** #10

View tasks: http://10.0.1.210:8000/docs

---

**Next Action:** Start with Task 1 - Install VS Code + PlatformIO and test ESP32-C6

**Estimated Timeline:** 3-4 weeks to first production deployment (depending on software learning curve)
