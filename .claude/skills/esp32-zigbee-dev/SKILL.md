# ESP32 Zigbee Development Skill

Expert guidance for ESP32-C6 Zigbee development using PlatformIO + ESP-IDF + ESP-Zigbee-SDK.

## Skill Purpose

Provide comprehensive knowledge for developing ESP32-C6 Zigbee devices with:
- PlatformIO build system
- ESP-IDF framework (native C)
- ESP-Zigbee-SDK 1.0.9+ APIs
- Multi-sensor integration
- Home Assistant ZHA integration

---

## Critical Knowledge

### 1. PlatformIO Project Structure (CRITICAL!)

**PlatformIO + ESP-IDF projects MUST use `src/` directory**

```
zigbee-project/
├── src/                      ← MUST be src/ for PlatformIO
│   ├── CMakeLists.txt
│   ├── idf_component.yml    ← Component dependencies
│   └── main.c
├── CMakeLists.txt            ← Root CMake
├── platformio.ini
└── sdkconfig.defaults        ← Zigbee configuration
```

**Why this matters:**
- Native ESP-IDF projects use `main/` directory
- **PlatformIO will NOT recognize projects with `main/` directory**
- If you use `main/`, PlatformIO initialization fails silently
- This is a PlatformIO compatibility requirement, not an ESP-IDF requirement

### 2. Component Dependencies

**ESP-Zigbee-SDK requires TWO libraries:**

File: `src/idf_component.yml`
```yaml
dependencies:
  espressif/esp-zigbee-lib:
    version: "~1.0.9"      # High-level Zigbee API
  espressif/esp-zboss-lib:
    version: "~1.0.0"      # Low-level ZBOSS stack (required!)
  espressif/led_strip:     # Optional: WS2812 LED support
    version: "^2.5.5"
```

File: `src/CMakeLists.txt`
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp-zigbee-lib esp-zboss-lib driver nvs_flash
)
```

**Common Error:**
```
fatal error: zb_vendor.h: No such file or directory
```
**Solution:** Add `esp-zboss-lib` to dependencies

### 3. Zigbee Configuration

File: `sdkconfig.defaults` (project root)
```ini
# Zigbee Stack
CONFIG_ZB_ENABLED=y
CONFIG_ZB_ZCZR=y                    # Router mode
CONFIG_ZB_RADIO_MODE_NATIVE=y       # Native radio (not RCP)
CONFIG_ZB_CHANNEL=11                # Zigbee channel (11-26)

# IEEE 802.15.4 Radio
CONFIG_IEEE802154_ENABLED=y

# NVS (required for Zigbee credentials)
CONFIG_NVS_ENCRYPTION=n

# Logging (optional)
CONFIG_ZB_LOG_LEVEL_DEBUG=y
```

### 4. ESP-Zigbee-SDK 1.0.9 API (Breaking Changes!)

ESP-Zigbee-SDK 1.0.9 has **breaking API changes** from earlier versions. Online examples may be outdated.

#### Endpoint Configuration

**❌ OLD API (doesn't exist in 1.0.9):**
```c
esp_zb_endpoint_config_t ep_config = {
    .endpoint = 10,
    .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
    .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
    .app_device_version = 0,
};
esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_config);
```

**✅ NEW API (1.0.9):**
```c
esp_zb_ep_list_add_ep(
    ep_list,
    cluster_list,
    10,  // endpoint
    ESP_ZB_AF_HA_PROFILE_ID,
    ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID
);
```

#### Router Configuration

**❌ OLD API:**
```c
esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
```

**✅ NEW API (1.0.9):**
```c
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

#### Thread Safety (Simplified in 1.0.9)

**❌ OLD API:**
```c
esp_zb_lock_acquire(portMAX_DELAY);
esp_zb_zcl_set_attribute_val(...);
esp_zb_lock_release();
```

**✅ NEW API (1.0.9):**
```c
esp_zb_zcl_set_attribute_val(...);  // Thread safety is internal
```

#### Device Type Constants

**❌ OLD (doesn't exist):**
```c
ESP_ZB_HA_LIGHT_SENSOR_DEVICE_ID
```

**✅ NEW (1.0.9):**
```c
ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID  // Generic sensor type
```

### 5. FreeRTOS Task Coordination (Critical!)

**Sensor tasks MUST be created BEFORE entering Zigbee main loop:**

**❌ WRONG (will block sensor tasks):**
```c
esp_zb_start(false);  // This blocks forever!
xTaskCreate(sensor_task, ...);  // Never reaches here
```

**✅ CORRECT:**
```c
// 1. Initialize Zigbee (non-blocking setup)
esp_zb_initialize_zigbee();

// 2. Create sensor tasks
xTaskCreate(bh1750_sensor_task, ...);
xTaskCreate(ds18b20_sensor_task, ...);

// 3. NOW enter blocking loop
esp_zb_main_loop_iteration();
```

---

## Common Build Errors

### Error: "Failed to resolve component 'esp_zigbee_core'"

**Cause:** Missing component dependencies or wrong directory structure

**Solutions:**
1. Check `src/idf_component.yml` exists (not root)
2. Check `src/CMakeLists.txt` has REQUIRES clause
3. Verify using `src/` directory (not `main/`)
4. Run `pio pkg install` to download components

### Error: "zb_vendor.h: No such file or directory"

**Cause:** Missing `esp-zboss-lib` dependency

**Solution:** Add to `src/idf_component.yml`:
```yaml
dependencies:
  espressif/esp-zboss-lib:
    version: "~1.0.0"
```

### Error: PlatformIO doesn't recognize project

**Cause:** Using `main/` directory instead of `src/`

**Solution:** Rename `main/` to `src/` for PlatformIO projects

### Error: "undefined reference to `esp_zb_*`"

**Cause:** Missing library in CMakeLists.txt

**Solution:** Add to `src/CMakeLists.txt`:
```cmake
REQUIRES esp-zigbee-lib esp-zboss-lib
```

---

## Zigbee Network Integration

### Home Assistant ZHA Pairing

**1. Enable pairing mode:**
- Settings → Devices & Services → ZHA
- Click "Add Device"
- Coordinator enters pairing mode for 60 seconds

**2. Flash and boot ESP32-C6:**
```bash
pio run --target upload
```

**3. Device will automatically:**
- Initialize Zigbee stack
- Search for networks
- Join coordinator's network
- Register endpoints and clusters

**4. Verify in Home Assistant:**
New entities appear based on endpoint configuration:
- Temperature sensors → `sensor.device_temperature`
- Humidity sensors → `sensor.device_humidity`
- Illuminance sensors → `sensor.device_illuminance`

### Troubleshooting Network Join

**Device won't join network:**

Check serial output for:
```
I (xxx) ESP_ZB: Start network steering
I (xxx) ESP_ZB: Joined network successfully
```

**Solutions:**
1. Verify coordinator in pairing mode
2. Check Extended PAN ID matches
3. Erase NVS and retry:
   ```bash
   pio run --target erase
   pio run --target upload
   ```

---

## Technical Implementation Patterns

### Illuminance Logarithmic Encoding

Zigbee Cluster Library requires logarithmic encoding for illuminance:

```c
// Convert linear lux to ZCL MeasuredValue
float lux_float = 123.45;  // Linear lux
uint16_t lux_zcl = (uint16_t)(10000.0f * log10f(lux_float) + 1.0f);

// Set attribute
esp_zb_zcl_set_attribute_val(
    endpoint,
    ESP_ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID,
    &lux_zcl,
    false
);
```

**Why logarithmic?**
- Allows 1 to 100,000 lux range with 16-bit resolution
- ZCL specification requirement
- Home Assistant automatically decodes

### Attribute Reporting Modes

**Automatic Reporting (Production):**
```c
esp_zb_zcl_set_attribute_val(..., true);  // Mark as changed
// Respects Home Assistant's configured intervals (30s-15min)
```

**Explicit Reporting (Debug):**
```c
esp_zb_zcl_report_attr_cmd_req_t cmd_req = {
    .zcl_basic_cmd = {...},
    .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
    .clusterID = cluster_id,
    .attributeID = attr_id,
    .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
};
esp_zb_zcl_report_attr_cmd_req(&cmd_req);
// Sends immediate report, bypasses intervals
```

### Location Descriptions (Indoor/Outdoor)

Add human-readable labels to Basic cluster:

```c
char location_desc[16] = "Indoor";
esp_zb_zcl_set_attribute_val(
    endpoint,
    ESP_ZB_ZCL_CLUSTER_ID_BASIC,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_BASIC_LOCATION_DESCRIPTION_ID,
    location_desc,
    false
);
```

Shows as "Indoor" or "Outdoor" in Home Assistant device info.

---

## Multi-Sensor Integration

### I2C Sensors (BH1750)

```c
// I2C initialization
i2c_config_t i2c_config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = GPIO_NUM_1,
    .scl_io_num = GPIO_NUM_2,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000,
};
i2c_param_config(I2C_NUM_0, &i2c_config);
i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
```

### 1-Wire Dallas (DS18B20)

```c
// OneWire driver
onewire_bus_handle_t bus;
onewire_bus_config_t bus_config = {
    .bus_gpio_num = GPIO_NUM_5,
};
onewire_bus_rmt_config_t rmt_config = {
    .max_rx_bytes = 10,
};
onewire_new_bus_rmt(&bus_config, &rmt_config, &bus);

// DS18B20 device
ds18b20_device_handle_t ds18b20;
onewire_device_iter_handle_t iter = NULL;
onewire_device_t next_device;
onewire_new_device_iter(bus, &iter);
ESP_ERROR_CHECK(onewire_device_iter_get_next(iter, &next_device));
ds18b20_new_device(&next_device, &ds18b20);
```

### DHT11 (Temperature + Humidity)

DHT11 requires precise timing. Use existing ESP-IDF DHT driver or implement bit-bang protocol.

---

## Version Requirements

### Minimum Versions
- ESP-IDF ≥ 5.1.0 (ESP32-C6 Zigbee support)
- ESP-Zigbee-SDK ≥ 1.0.0
- PlatformIO platform `espressif32` ≥ 6.0.0

### Verify Installed Version
```bash
cat ~/.platformio/packages/framework-espidf/version.txt
```

### Tested Configuration
- ESP-IDF: 5.5.0 ✅
- ESP-Zigbee-SDK: 1.0.9 ✅
- PlatformIO: espressif32@6.5.0+ ✅

---

## Hardware Reference

### Waveshare ESP32-C6-Zero

| Feature | Value |
|---------|-------|
| MCU | ESP32-C6 (RISC-V 160MHz) |
| Zigbee | Native 802.15.4 radio |
| Flash | 4MB |
| RAM | 512KB |
| USB | Built-in USB-JTAG (no CH340) |
| LED | WS2812 RGB on GPIO8 |
| Status LED | GPIO15 |

### Common GPIO Assignments

| Function | GPIO | Notes |
|----------|------|-------|
| I2C SDA | GPIO1 | 4.7kΩ pull-up |
| I2C SCL | GPIO2 | 4.7kΩ pull-up |
| DHT11 Data | GPIO4 | Built-in pull-up on module |
| DS18B20 Data | GPIO5 | Requires external 4.7kΩ pull-up |
| WS2812 LED | GPIO8 | Built-in on Waveshare board |
| Status LED | GPIO15 | Pre-wired on Waveshare board |

---

## Quick Reference Commands

### Build and Upload
```bash
cd /path/to/project
pio run                    # Build
pio run --target upload    # Upload
pio device monitor         # Serial monitor
```

### Clean Build
```bash
pio run --target clean
pio run --target fullclean  # Remove all build artifacts
```

### Erase NVS (Factory Reset)
```bash
pio run --target erase
```

### Monitor with Auto-reconnect
```bash
python3 monitor.py  # If project has custom monitor script
```

---

## Resources

### Official Documentation
- [ESP-Zigbee-SDK GitHub](https://github.com/espressif/esp-zigbee-sdk)
- [ESP-Zigbee-SDK API Reference](https://docs.espressif.com/projects/esp-zigbee-sdk/)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/)
- [Zigbee Cluster Library Spec](https://zigbeealliance.org/developer_resources/zigbee-cluster-library/)

### Home Assistant Integration
- [ZHA Integration](https://www.home-assistant.io/integrations/zha/)
- [ZHA Device Support](https://zigzag.blakadder.com/)

### Hardware
- [Waveshare ESP32-C6-Zero Wiki](https://www.waveshare.com/wiki/ESP32-C6-Zero)
- [ESP32-C6 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf)

---

## Key Takeaways

1. **PlatformIO requires `src/` directory** (not `main/`)
2. **ESP-Zigbee-SDK needs TWO libraries** (esp-zigbee-lib + esp-zboss-lib)
3. **API version matters** - 1.0.9 has breaking changes
4. **Task timing is critical** - Create sensor tasks BEFORE Zigbee main loop
5. **Configuration files required** - sdkconfig.defaults and idf_component.yml
6. **Always check documentation version** - Online examples may be outdated

---

**Skill Version:** 1.0.0
**Last Updated:** 2026-01-14
**Tested With:** ESP-IDF 5.5.0, ESP-Zigbee-SDK 1.0.9
**Hardware:** Waveshare ESP32-C6-Zero
