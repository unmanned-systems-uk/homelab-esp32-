# Zigbee Multi-Sensor Production Firmware

**Hardware:** Waveshare ESP32-C6-Zero
**Framework:** ESP-IDF native + ESP-Zigbee-SDK
**Device Type:** Router/Repeater (always powered, extends mesh network)
---

## Lessons Learned

### ESP-IDF + ESP-Zigbee-SDK Integration Issues

**Date:** 2026-01-14
**ESP-IDF Version:** 5.5.0
**ESP-Zigbee-SDK Version:** 1.0.9

#### 1. Project Structure Requirements

**Issue:** Build failed with "Failed to resolve component 'esp_zigbee_core'"

**Root Cause:** PlatformIO + ESP-IDF requires a specific directory structure that differs from native ESP-IDF projects.

**CRITICAL: PlatformIO MUST use `src/` directory (NOT `main/`)**
- Native ESP-IDF projects use `main/` directory
- **PlatformIO projects MUST use `src/` directory** - if you use `main/`, PlatformIO will not recognize the project and will not initiate
- Place `idf_component.yml` in the `src/` directory (not root)
- Place `CMakeLists.txt` in the `src/` directory

**Correct Structure for PlatformIO + ESP-IDF:**
  ```
  zigbee-multi-sensor/
  ├── src/                      ← MUST be src/ for PlatformIO
  │   ├── CMakeLists.txt
  │   ├── idf_component.yml    ← Component dependencies
  │   └── main.c
  ├── CMakeLists.txt
  ├── platformio.ini
  └── sdkconfig.defaults        ← Zigbee configuration
  ```

#### 2. Missing ZBOSS Library Dependency

**Issue:** `fatal error: zb_vendor.h: No such file or directory`

**Root Cause:** ESP-Zigbee-SDK has two components:
- `esp-zigbee-lib` (high-level API)
- `esp-zboss-lib` (low-level Zigbee stack, contains `zb_vendor.h`)

The high-level library alone is insufficient.

**Solution:** Add both libraries to `main/idf_component.yml`:
```yaml
dependencies:
  espressif/esp-zigbee-lib:
    version: "~1.0.9"
  espressif/esp-zboss-lib:
    version: "~1.0.0"
```

And to `main/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp-zigbee-lib esp-zboss-lib driver nvs_flash)
```

#### 3. ESP-Zigbee-SDK 1.0.9 API Changes

**Issue:** Multiple compilation errors due to API changes from earlier examples/documentation.

**Breaking Changes in 1.0.9:**

##### a) Endpoint Configuration API
```c
// OLD API (doesn't exist in 1.0.9):
esp_zb_endpoint_config_t ep_config = {
    .endpoint = 10,
    .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
    .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
    .app_device_version = 0,
};
esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_config);

// NEW API (1.0.9):
esp_zb_ep_list_add_ep(ep_list, cluster_list,
                      10,  // endpoint
                      ESP_ZB_AF_HA_PROFILE_ID,
                      ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID);
```

##### b) Router Configuration
```c
// OLD API (doesn't exist in 1.0.9):
esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();

// NEW API (1.0.9):
esp_zb_cfg_t zb_nwk_cfg = {
    .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,
    .install_code_policy = INSTALLCODE_POLICY_ENABLE,
    .nwk_cfg = {
        .zczr_cfg = {
            .max_children = 10,
        },
    },
};
```

##### c) Thread Safety Locks
```c
// OLD API (doesn't exist in 1.0.9):
esp_zb_lock_acquire(portMAX_DELAY);
esp_zb_zcl_set_attribute_val(...);
esp_zb_lock_release();

// NEW API (1.0.9):
esp_zb_zcl_set_attribute_val(...);  // Thread safety is internal
```

##### d) Device Type Constants
```c
// OLD constant (doesn't exist in 1.0.9):
ESP_ZB_HA_LIGHT_SENSOR_DEVICE_ID

// NEW constant (1.0.9):
ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID  // Generic sensor type
```

#### 4. Zigbee Configuration via sdkconfig

**Issue:** Zigbee support not enabled at framework level.

**Solution:** Create `sdkconfig.defaults` in project root:
```ini
CONFIG_ZB_ENABLED=y
CONFIG_ZB_ZCZR=y
CONFIG_ZB_RADIO_MODE_NATIVE=y
CONFIG_ZB_CHANNEL=11
CONFIG_IEEE802154_ENABLED=y
```

This configures ESP-IDF to include Zigbee stack components.

#### 5. PlatformIO Workspace Management

**Issue:** PlatformIO building wrong project (multi-sensor-test instead of zigbee-multi-sensor).

**Root Cause:** VSCode workspace with multiple PlatformIO projects causes confusion about active project.

**Solutions:**
- **Option 1:** Open project directly: `code /path/to/zigbee-multi-sensor`
- **Option 2:** Use terminal commands: `cd zigbee-multi-sensor && pio run`
- **Option 3:** Click project name in PlatformIO sidebar to switch active project

#### 6. ESP-IDF Version Compatibility

**Minimum Requirements:**
- ESP-IDF ≥ 5.1.0 (for ESP32-C6 Zigbee support)
- ESP-Zigbee-SDK ≥ 1.0.0
- PlatformIO platform `espressif32` ≥ 6.0.0

**Verify versions:**
```bash
cat ~/.platformio/packages/framework-espidf/version.txt
```

Our setup: ESP-IDF 5.5.0 ✅ (confirmed working)

---

### Key Takeaways

1. **Always check API documentation version** - Espressif examples online may use older API versions
2. **Component dependencies are transitive** - Need both high-level and low-level Zigbee libraries
3. **Directory structure matters** - ESP-IDF is strict about `main/` directory naming
4. **Configuration files are required** - `sdkconfig.defaults` and `idf_component.yml` are not optional
5. **API changes are significant** - Version 1.0.9 has breaking changes from earlier versions

### Useful Resources

- [ESP-Zigbee-SDK GitHub](https://github.com/espressif/esp-zigbee-sdk)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/)
- [Zigbee Cluster Library Specification](https://zigbeealliance.org/developer_resources/zigbee-cluster-library/)

---

## Device Features

### Multi-Endpoint Architecture

| Endpoint | Device Type | Function | Clusters | Update Interval |
|----------|-------------|----------|----------|-----------------|
| **EP 10** | DHT11 (Indoor) | Temperature + Humidity | Temperature (0x0402), Humidity (0x0405), Basic (0x0000) | 60s |
| **EP 11** | DS18B20 (Outdoor) | Temperature | Temperature (0x0402), Basic (0x0000) | 60s |
| **EP 12** | BH1750 | Illuminance | Illuminance (0x0400), Basic (0x0000) | 30s |
| **EP 14** | Mode Switch | Reporting Control | On/Off (0x0006), Basic (0x0000) | N/A |
| **EP 13** | HLK-LD2450 (Future) | mmWave Occupancy | Occupancy (0x0406), Custom (0xFC00) | TBD |

### WS2812 RGB LED Visual Indicators

The onboard WS2812 LED (GPIO8) provides real-time visual feedback:

| Color | Pattern | Meaning |
|-------|---------|---------|
| **White** | Flash | System startup |
| **Purple/Magenta** | Solid | Searching for Zigbee network |
| **Cyan** | Flash | Successfully joined network |
| **Green** | Brief flash | Sensor read successful |
| **Red** | Brief flash | Sensor read error |
| **Blue** | Brief flash | Zigbee data transmitted |

### Runtime-Switchable Reporting Modes

The device supports two Zigbee attribute reporting modes, switchable via Home Assistant:

**Automatic Mode (Default - Switch OFF):**
- Respects Home Assistant's configured reporting intervals (30s-15min)
- Lower network traffic, better for production
- Uses `esp_zb_zcl_set_attribute_val()` with change notifications

**Explicit Mode (Debug - Switch ON):**
- Sends immediate reports on every sensor update
- Higher data rate for debugging and testing
- Uses `esp_zb_zcl_report_attr_cmd_req()` for instant delivery

Toggle the "Reporting Mode" switch entity in Home Assistant to change modes without reflashing firmware.

### Indoor/Outdoor Temperature Labels

Temperature sensors include **Location Description** attributes in their Basic clusters:
- **EP 10 (DHT11):** "Indoor"
- **EP 11 (DS18B20):** "Outdoor"

This helps identify sensors in Home Assistant's entity list.

### ZCL-Compliant Illuminance Encoding

Illuminance values use logarithmic encoding per Zigbee Cluster Library specification:
```
ZCL MeasuredValue = 10000 × log₁₀(lux) + 1
```

This ensures accurate light readings across the full range (1 to 100,000 lux) in Home Assistant.

---

## Build Instructions

### Prerequisites
- VS Code + PlatformIO installed
- ESP-IDF framework (automatically installed by PlatformIO)
- Waveshare ESP32-C6-Zero board

### Build and Upload

```bash
cd zigbee-multi-sensor

# Build firmware
pio run

# Upload to ESP32-C6
pio run --target upload

# Monitor serial output
pio device monitor
```

---

## Zigbee Network Setup

### 1. Put Zigbee Coordinator in Pairing Mode

**Using Zigbee2MQTT:**
```bash
# Enable pairing mode (allow new devices to join)
mosquitto_pub -t 'zigbee2mqtt/bridge/request/permit_join' -m '{"value": true}'
```

**Using Home Assistant ZHA:**
- Go to **Settings → Devices & Services → ZHA**
- Click **Add Device**
- Device will enter pairing mode for 60 seconds

### 2. Flash and Boot ESP32-C6

```bash
pio run --target upload
```

The device will automatically:
1. Initialize Zigbee stack
2. Search for nearby networks
3. Join the coordinator's network
4. LED turns ON when successfully joined

### 3. Verify in Home Assistant

New entities should appear:
- `sensor.esp32_multisensor_indoor_temperature` (DHT11 - EP10)
- `sensor.esp32_multisensor_indoor_humidity` (DHT11 - EP10)
- `sensor.esp32_multisensor_outdoor_temperature` (DS18B20 - EP11)
- `sensor.esp32_multisensor_illuminance` (BH1750 - EP12)
- `switch.esp32_multisensor_reporting_mode` (Mode Switch - EP14)

**Note:** Temperature sensors will show their location ("Indoor"/"Outdoor") in the device information.

---

## Project Status

### ✅ Completed (Production-Ready)
- [x] Zigbee stack initialization (Router mode)
- [x] 5-endpoint architecture (EP 10, 11, 12, 14 + future EP 13)
- [x] Cluster configurations (Temperature, Humidity, Illuminance, On/Off)
- [x] FreeRTOS task structure and coordination
- [x] **Sensor driver integration (BH1750, DS18B20, DHT11)** ✅
- [x] **Real sensor data collection** ✅
- [x] **Zigbee attribute reporting (automatic + explicit modes)** ✅
- [x] **WS2812 RGB LED visual indicators** ✅
- [x] **Runtime-switchable reporting via Home Assistant** ✅
- [x] **Indoor/Outdoor location descriptions** ✅
- [x] **ZCL-compliant logarithmic illuminance encoding** ✅

### ⏳ Pending (Future Enhancements)
- [ ] HLK-LD2450 mmWave sensor integration (EP 13)
- [ ] OTA firmware update testing
- [ ] Watchdog timer implementation
- [ ] Network reconnection logic
- [ ] Factory reset mechanism (hold button 10s)
- [ ] 24-hour stability testing

### Resource Utilization
- **RAM Usage:** 15.2% (49.7 KB / 320 KB) - excellent headroom for future features
- **Flash Usage:** 37.1% (736 KB / 1.98 MB)

---

## Current Implementation

**Phase 1: Zigbee Framework** ✅
- Initializes Zigbee stack as Router device
- Creates 5 endpoints with proper clusters (EP 10, 11, 12, 14)
- Joins Zigbee network automatically
- Multi-mode attribute reporting (automatic + explicit)

**Phase 2: Sensor Integration** ✅
- I2C initialization for BH1750 light sensor (GPIO1/GPIO2)
- 1-Wire implementation for DS18B20 temperature (GPIO5)
- DHT11 protocol for temperature + humidity (GPIO4)
- All sensors reporting real-time data to Home Assistant
- ZCL-compliant logarithmic encoding for illuminance

**Phase 3: Visual Debugging** ✅
- WS2812 RGB LED (GPIO8) with color-coded status indicators
- Real-time feedback for sensor reads and Zigbee transmissions
- Network connection status visualization

**Phase 4: Runtime Configuration** ✅
- Home Assistant switch entity for reporting mode control
- Toggle between automatic and explicit reporting without reflashing
- Indoor/Outdoor location labels in sensor attributes

---

## Pin Connections

| Component | GPIO Pin | Protocol | Notes |
|-----------|----------|----------|-------|
| BH1750 SDA | GPIO1 | I2C | I2C Data (4.7kΩ pull-up) |
| BH1750 SCL | GPIO2 | I2C | I2C Clock (4.7kΩ pull-up) |
| DHT11 Data | GPIO4 | 1-Wire DHT | Indoor temperature + humidity |
| DS18B20 Data | GPIO5 | 1-Wire Dallas | Outdoor temperature (4.7kΩ pull-up) |
| WS2812 LED | GPIO8 | RMT | RGB visual indicators (built-in) |
| Status LED | GPIO15 | GPIO | Pre-wired on Waveshare board |

---

## Troubleshooting

### Device Won't Join Network

**Check:**
1. Coordinator in pairing mode?
2. Serial output shows "Start network steering"?
3. Check Extended PAN ID matches coordinator

**Solution:**
```bash
# Erase NVS and retry
pio run --target erase
pio run --target upload
```

### Build Errors

**Missing ESP-Zigbee-SDK:**
```bash
# PlatformIO will auto-install, but if issues:
pio pkg install --platform espressif32
```

### LED Not Turning On

- LED turns on only after successfully joining network
- Check serial output for "Joined network successfully"
- If stuck in "Network steering", coordinator may not be in pairing mode

### Sensor Data Not Appearing in Home Assistant

**Check:**
1. Device successfully joined network? (Cyan LED flash)
2. Serial output shows sensor readings? (use `pio device monitor`)
3. Try the included `monitor.py` GUI to verify sensor data
4. Wait 30-60 seconds - automatic reporting has minimum intervals

**Solution:**
- Toggle the "Reporting Mode" switch in Home Assistant to enable explicit reporting for immediate updates
- Re-pair the device if it was paired before sensor integration was complete

---

## Key Technical Implementation Details

### 1. Illuminance Logarithmic Encoding (src/main.c:986-998)

Zigbee Cluster Library requires logarithmic encoding for illuminance:
```c
// Linear lux → ZCL MeasuredValue
uint16_t lux_value = (uint16_t)(10000.0f * log10f(lux_float) + 1.0f);
```

This allows accurate representation from 1 to 100,000 lux with 16-bit resolution.

### 2. Runtime-Switchable Reporting Modes (src/main.c:245-306)

Two reporting strategies implemented:
- **Automatic:** `esp_zb_zcl_set_attribute_val(..., true)` - marks changed
- **Explicit:** `esp_zb_zcl_report_attr_cmd_req()` - immediate delivery

Controlled via On/Off cluster on EP14, switchable from Home Assistant UI.

### 3. FreeRTOS Task Coordination (src/main.c:1237-1249)

Critical fix: Sensor tasks must be created BEFORE entering Zigbee main loop:
```c
esp_zb_initialize_zigbee();        // Setup (non-blocking)
xTaskCreate(bh1750_sensor_task, ...);  // Create tasks
xTaskCreate(ds18b20_sensor_task, ...);
xTaskCreate(dht11_sensor_task, ...);
esp_zb_main_loop_iteration();      // Now enter blocking loop
```

Previous implementation blocked before task creation, preventing sensor reads.

### 4. WS2812 LED via RMT Peripheral (src/main.c:142-227)

Uses ESP-IDF's `led_strip` component with RMT hardware timing:
```c
led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 10 * 1000 * 1000,  // 10 MHz for precise timing
};
```

Color-coded visual feedback for debugging without serial monitor.

---

## Next Steps (Future Enhancements)

1. **HLK-LD2450 mmWave Sensor**
   Add EP13 for occupancy detection and presence tracking

2. **OTA Firmware Updates**
   Test Over-The-Air updates via Zigbee network

3. **Production Hardening**
   - Watchdog timer for crash recovery
   - Network reconnection logic
   - Factory reset mechanism (hold button 10s)
   - 24-hour stability testing

---

## Monitoring Tools

### monitor.py - Real-time Sensor GUI

Included Python GUI for debugging and monitoring:
```bash
python3 monitor.py
```

**Features:**
- Auto-detects ESP32-C6 USB port
- Displays all 4 sensor readings in real-time
- Shows Zigbee network diagnostics (connection, channel, addresses)
- Color-coded sensor boxes
- Requires: `pyserial` and `tkinter` (usually pre-installed)

---

**Last Updated:** 2026-01-14 (Production release - all sensors integrated)
