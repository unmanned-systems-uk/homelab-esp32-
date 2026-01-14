# Production Hardening Requirements - ESP32-C6 Zigbee Multi-Sensor

**Document Version:** 1.0
**Date:** 2026-01-14
**Project:** ESP32-C6 Zigbee Multi-Sensor
**Phase:** 6 - Production Hardening
**Status:** Planning

---

## Executive Summary

This document defines the requirements for hardening the ESP32-C6 Zigbee multi-sensor device for 24/7 production deployment. The goal is to ensure reliable, unattended operation with automatic recovery from common failure scenarios.

**Priority:** ⭐ CRITICAL - Must complete before production deployment

**Scope:**
1. Watchdog Timer Implementation
2. Network Reconnection Logic
3. Factory Reset Mechanism
4. Error Handling & Resilience

**Success Criteria:**
- 99.9% uptime (< 8.6 minutes downtime per day)
- Automatic recovery from crashes without manual intervention
- Graceful handling of network disconnections
- User-accessible factory reset for troubleshooting

---

## 1. Watchdog Timer Implementation

### 1.1 Purpose
Automatically restart the ESP32-C6 if the firmware becomes unresponsive due to crashes, deadlocks, or infinite loops.

### 1.2 Functional Requirements

**FR-WDT-001:** Hardware watchdog timer shall be enabled on system boot
**FR-WDT-002:** Watchdog timeout shall be configurable (default: 30 seconds)
**FR-WDT-003:** Main task loop shall reset watchdog every iteration
**FR-WDT-004:** All FreeRTOS tasks with blocking operations shall reset watchdog periodically
**FR-WDT-005:** Watchdog reset events shall be logged to NVS (non-volatile storage)
**FR-WDT-006:** Reset reason shall be readable after boot (watchdog vs manual vs power-on)

### 1.3 Technical Specifications

**Hardware Watchdog (MWDT0):**
```c
// ESP-IDF Configuration
CONFIG_ESP_TASK_WDT=y                    // Enable task watchdog
CONFIG_ESP_TASK_WDT_TIMEOUT_S=30         // 30 second timeout
CONFIG_ESP_TASK_WDT_PANIC=y              // Panic on timeout (triggers restart)
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=n  // Don't monitor idle task
```

**Software Implementation:**
```c
#include "esp_task_wdt.h"

// Initialize watchdog (in app_main)
esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,           // 30 seconds
    .idle_core_mask = 0,           // Don't monitor idle tasks
    .trigger_panic = true          // Restart on timeout
};
esp_task_wdt_init(&wdt_config);
esp_task_wdt_add(NULL);            // Add current task to watchdog

// Reset watchdog in main loop
void sensor_task(void *pvParameters) {
    while (1) {
        // Read sensors
        read_sensors();

        // Reset watchdog
        esp_task_wdt_reset();

        // Report to Zigbee
        report_zigbee_data();

        vTaskDelay(pdMS_TO_TICKS(10000));  // 10 second delay
    }
}
```

**Watchdog Reset Counter:**
```c
// Store watchdog reset count in NVS
nvs_handle_t nvs_handle;
uint32_t wdt_reset_count = 0;

void log_watchdog_reset(void) {
    esp_reset_reason_t reset_reason = esp_reset_reason();

    if (reset_reason == ESP_RST_TASK_WDT ||
        reset_reason == ESP_RST_WDT) {
        // Increment watchdog reset counter
        nvs_open("storage", NVS_READWRITE, &nvs_handle);
        nvs_get_u32(nvs_handle, "wdt_count", &wdt_reset_count);
        wdt_reset_count++;
        nvs_set_u32(nvs_handle, "wdt_count", wdt_reset_count);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);

        ESP_LOGW(TAG, "Watchdog reset detected! Count: %lu", wdt_reset_count);
    }
}
```

### 1.4 Testing Requirements

**TEST-WDT-001:** Simulate infinite loop - verify device restarts within 35s
**TEST-WDT-002:** Simulate I2C bus hang - verify watchdog triggers
**TEST-WDT-003:** Verify watchdog reset count increments correctly
**TEST-WDT-004:** Verify normal operation does NOT trigger watchdog
**TEST-WDT-005:** Verify LED flash pattern on watchdog reset (3x red flash)

### 1.5 Success Metrics

- [ ] Watchdog timer triggers within 30-35 seconds of firmware hang
- [ ] Watchdog reset count logged to NVS
- [ ] Device fully operational after watchdog reset
- [ ] No false positives during 24-hour normal operation

---

## 2. Network Reconnection Logic

### 2.1 Purpose
Automatically rejoin the Zigbee network if connection is lost due to coordinator failure, power outage, or interference.

### 2.2 Functional Requirements

**FR-NET-001:** Detect Zigbee network disconnection within 60 seconds
**FR-NET-002:** Attempt automatic reconnection using exponential backoff
**FR-NET-003:** Log connection uptime and disconnection events
**FR-NET-004:** Provide visual feedback during reconnection attempts (LED)
**FR-NET-005:** Track network quality metrics (LQI, RSSI) for diagnostics
**FR-NET-006:** Continue sensor operation during network outage (cache data if possible)

### 2.3 Technical Specifications

**Zigbee Network State Detection:**
```c
// Monitor Zigbee network state
typedef enum {
    ZB_STATE_DISCONNECTED,
    ZB_STATE_JOINING,
    ZB_STATE_CONNECTED,
    ZB_STATE_RECONNECTING
} zigbee_state_t;

zigbee_state_t zigbee_state = ZB_STATE_DISCONNECTED;

// Zigbee event handler
void zigbee_event_handler(esp_zb_core_action_callback_id_t callback_id,
                           const void *message) {
    switch (callback_id) {
        case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
            // Handle default response
            break;

        case ESP_ZB_CORE_NWK_NLME_STATUS_INDICATION_CB_ID:
            esp_zb_nwk_nlme_status_indication_t *status =
                (esp_zb_nwk_nlme_status_indication_t *)message;

            if (status->status == ESP_ZB_NWK_STATUS_NETWORK_LEFT) {
                ESP_LOGW(TAG, "Network connection lost!");
                zigbee_state = ZB_STATE_DISCONNECTED;
                start_reconnection_timer();
            }
            break;

        case ESP_ZB_CORE_START_ZIGBEE_STACK_CB_ID:
            ESP_LOGI(TAG, "Zigbee stack started successfully");
            zigbee_state = ZB_STATE_JOINING;
            break;
    }
}
```

**Exponential Backoff Reconnection:**
```c
// Reconnection with exponential backoff
typedef struct {
    uint32_t retry_count;
    uint32_t backoff_delay_ms;
    uint32_t max_backoff_ms;
    TimerHandle_t reconnect_timer;
} reconnect_state_t;

reconnect_state_t reconnect_state = {
    .retry_count = 0,
    .backoff_delay_ms = 1000,      // Start at 1 second
    .max_backoff_ms = 60000,       // Max 60 seconds
    .reconnect_timer = NULL
};

void attempt_reconnection(void) {
    ESP_LOGI(TAG, "Attempting reconnection (attempt %lu)...",
             reconnect_state.retry_count + 1);

    // Update LED to show reconnecting state (yellow blink)
    set_led_state(LED_STATE_RECONNECTING);

    // Attempt to rejoin Zigbee network
    esp_zb_start(false);  // Rejoin network (don't erase data)

    reconnect_state.retry_count++;

    // Calculate next backoff delay (exponential: 1s, 2s, 4s, 8s, 16s, 32s, 60s)
    reconnect_state.backoff_delay_ms *= 2;
    if (reconnect_state.backoff_delay_ms > reconnect_state.max_backoff_ms) {
        reconnect_state.backoff_delay_ms = reconnect_state.max_backoff_ms;
    }
}

void on_network_connected(void) {
    ESP_LOGI(TAG, "Network reconnected! Attempts: %lu",
             reconnect_state.retry_count);

    // Reset reconnection state
    reconnect_state.retry_count = 0;
    reconnect_state.backoff_delay_ms = 1000;

    // Stop reconnection timer
    if (reconnect_state.reconnect_timer) {
        xTimerStop(reconnect_state.reconnect_timer, 0);
    }

    // Update LED to show connected state (green)
    set_led_state(LED_STATE_CONNECTED);

    // Log uptime statistics
    log_connection_uptime();
}
```

**Connection Uptime Tracking:**
```c
typedef struct {
    uint32_t total_uptime_sec;        // Total connected time
    uint32_t current_session_start;   // Current connection start time
    uint32_t disconnect_count;        // Number of disconnections
    uint32_t longest_uptime_sec;      // Longest continuous connection
} connection_stats_t;

connection_stats_t conn_stats = {0};

void log_connection_uptime(void) {
    uint32_t current_time = esp_log_timestamp() / 1000;  // Convert to seconds

    if (conn_stats.current_session_start > 0) {
        uint32_t session_duration = current_time - conn_stats.current_session_start;
        conn_stats.total_uptime_sec += session_duration;

        if (session_duration > conn_stats.longest_uptime_sec) {
            conn_stats.longest_uptime_sec = session_duration;
        }

        ESP_LOGI(TAG, "Session uptime: %lu seconds", session_duration);
        ESP_LOGI(TAG, "Total uptime: %lu seconds", conn_stats.total_uptime_sec);
        ESP_LOGI(TAG, "Disconnections: %lu", conn_stats.disconnect_count);
    }

    // Start new session
    conn_stats.current_session_start = current_time;
}

void on_network_disconnected(void) {
    conn_stats.disconnect_count++;
    log_connection_uptime();

    // Reset session start (will be set again on reconnect)
    conn_stats.current_session_start = 0;
}
```

**Network Quality Monitoring:**
```c
typedef struct {
    uint8_t lqi;           // Link Quality Indicator (0-255)
    int8_t rssi;           // Received Signal Strength Indicator (dBm)
    uint16_t short_addr;   // Device short address
    uint16_t parent_addr;  // Parent router short address
} network_quality_t;

network_quality_t network_quality = {0};

void update_network_quality(void) {
    // Get LQI and RSSI from Zigbee stack
    esp_zb_nwk_get_lqi(&network_quality.lqi);
    esp_zb_nwk_get_rssi(&network_quality.rssi);

    ESP_LOGD(TAG, "Network quality - LQI: %u, RSSI: %d dBm",
             network_quality.lqi, network_quality.rssi);

    // Warn if network quality is poor
    if (network_quality.lqi < 100) {
        ESP_LOGW(TAG, "Poor network quality! LQI: %u", network_quality.lqi);
    }
}
```

### 2.4 LED Visual Feedback States

| State | Color | Pattern | Meaning |
|-------|-------|---------|---------|
| Disconnected | Magenta/Purple | Solid | Not connected to network |
| Joining | Yellow | Slow blink (1Hz) | Searching for network |
| Reconnecting | Orange | Fast blink (2Hz) | Attempting to rejoin |
| Connected | Green | Solid (or brief flash) | Successfully connected |
| Data TX | Blue | Brief flash | Sending Zigbee data |

### 2.5 Testing Requirements

**TEST-NET-001:** Disable coordinator - verify reconnection within 5 minutes
**TEST-NET-002:** Verify exponential backoff delays (1s, 2s, 4s, 8s, 16s, 32s, 60s)
**TEST-NET-003:** Verify LED state changes during reconnection
**TEST-NET-004:** Power cycle coordinator 10 times - verify automatic recovery
**TEST-NET-005:** Verify uptime statistics are accurate
**TEST-NET-006:** Test LQI/RSSI monitoring and logging

### 2.6 Success Metrics

- [ ] Automatic reconnection within 5 minutes of network loss
- [ ] Exponential backoff implemented correctly
- [ ] Connection uptime tracked and logged
- [ ] LED feedback provides clear visual status
- [ ] No data loss during brief disconnections (<60s)

---

## 3. Factory Reset Mechanism

### 3.1 Purpose
Allow users to reset the device to factory defaults for troubleshooting or redeployment without reflashing firmware.

### 3.2 Functional Requirements

**FR-RST-001:** Factory reset triggered by holding GPIO0 (boot button) for 10 seconds
**FR-RST-002:** LED shall provide visual feedback during reset sequence
**FR-RST-003:** Zigbee network credentials shall be erased
**FR-RST-004:** WiFi credentials shall be erased (if stored)
**FR-RST-005:** Sensor calibration settings shall be preserved (optional)
**FR-RST-006:** Watchdog reset counter shall be preserved for diagnostics
**FR-RST-007:** Device shall automatically restart after reset
**FR-RST-008:** Factory reset shall be logged to NVS for audit trail

### 3.3 Technical Specifications

**GPIO0 Button Detection:**
```c
#define FACTORY_RESET_GPIO      GPIO_NUM_0    // Boot button
#define FACTORY_RESET_HOLD_TIME 10000         // 10 seconds in milliseconds

// Button monitoring task
void factory_reset_monitor_task(void *pvParameters) {
    uint32_t button_pressed_time = 0;
    bool button_pressed = false;

    // Configure GPIO as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FACTORY_RESET_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    while (1) {
        int button_state = gpio_get_level(FACTORY_RESET_GPIO);

        if (button_state == 0) {  // Button pressed (active low)
            if (!button_pressed) {
                button_pressed = true;
                button_pressed_time = esp_log_timestamp();
                ESP_LOGI(TAG, "Factory reset button pressed...");

                // Start LED feedback (red slow blink)
                set_led_state(LED_STATE_FACTORY_RESET_PENDING);
            }

            // Check if held for 10 seconds
            uint32_t hold_duration = esp_log_timestamp() - button_pressed_time;
            if (hold_duration >= FACTORY_RESET_HOLD_TIME) {
                ESP_LOGW(TAG, "Factory reset triggered!");
                perform_factory_reset();
                break;  // Task will exit, device will restart
            }
        } else {
            // Button released before 10 seconds
            if (button_pressed) {
                button_pressed = false;
                ESP_LOGI(TAG, "Factory reset cancelled");

                // Restore normal LED state
                set_led_state(LED_STATE_CONNECTED);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }
}
```

**Factory Reset Implementation:**
```c
typedef struct {
    uint32_t reset_count;
    uint32_t last_reset_timestamp;
} factory_reset_log_t;

void perform_factory_reset(void) {
    ESP_LOGW(TAG, "=== FACTORY RESET IN PROGRESS ===");

    // Log factory reset event
    nvs_handle_t nvs_handle;
    factory_reset_log_t reset_log = {0};

    nvs_open("storage", NVS_READWRITE, &nvs_handle);

    // Read existing reset count
    nvs_get_u32(nvs_handle, "factory_reset_count", &reset_log.reset_count);
    reset_log.reset_count++;
    reset_log.last_reset_timestamp = esp_log_timestamp() / 1000;

    // Store updated count
    nvs_set_u32(nvs_handle, "factory_reset_count", reset_log.reset_count);
    nvs_set_u32(nvs_handle, "factory_reset_time", reset_log.last_reset_timestamp);
    nvs_commit(nvs_handle);

    ESP_LOGI(TAG, "Factory reset count: %lu", reset_log.reset_count);

    // LED feedback: Fast red flashing (5 times)
    for (int i = 0; i < 5; i++) {
        set_led_color(255, 0, 0);  // Red
        vTaskDelay(pdMS_TO_TICKS(200));
        set_led_color(0, 0, 0);    // Off
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // Erase Zigbee network data
    ESP_LOGI(TAG, "Erasing Zigbee credentials...");
    esp_zb_factory_reset();  // ESP-Zigbee SDK function

    // Erase WiFi credentials (if applicable)
    #ifdef CONFIG_WIFI_ENABLED
    ESP_LOGI(TAG, "Erasing WiFi credentials...");
    nvs_erase_key(nvs_handle, "wifi_ssid");
    nvs_erase_key(nvs_handle, "wifi_password");
    nvs_commit(nvs_handle);
    #endif

    // Optional: Preserve sensor calibration
    // Do NOT erase keys: "temp_offset", "humidity_offset"

    nvs_close(nvs_handle);

    // Final LED feedback: Solid green (reset complete)
    set_led_color(0, 255, 0);  // Green
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGW(TAG, "Factory reset complete. Restarting...");

    // Restart device
    esp_restart();
}
```

**Factory Reset Counter Readout:**
```c
void print_factory_reset_stats(void) {
    nvs_handle_t nvs_handle;
    uint32_t reset_count = 0;
    uint32_t last_reset_time = 0;

    nvs_open("storage", NVS_READONLY, &nvs_handle);
    nvs_get_u32(nvs_handle, "factory_reset_count", &reset_count);
    nvs_get_u32(nvs_handle, "factory_reset_time", &last_reset_time);
    nvs_close(nvs_handle);

    if (reset_count > 0) {
        ESP_LOGI(TAG, "Device has been factory reset %lu times", reset_count);
        ESP_LOGI(TAG, "Last reset: %lu seconds ago",
                 (esp_log_timestamp() / 1000) - last_reset_time);
    } else {
        ESP_LOGI(TAG, "Device has never been factory reset");
    }
}
```

### 3.4 LED Visual Feedback Sequence

| Phase | Duration | LED State | Description |
|-------|----------|-----------|-------------|
| Button Held | 0-10s | Red slow blink (1Hz) | Waiting for 10 second hold |
| Reset Start | 0.5s | Red fast flash (5x) | Factory reset confirmed |
| Processing | 2-5s | Red solid | Erasing data |
| Complete | 2s | Green solid | Reset complete, restarting |

### 3.5 Testing Requirements

**TEST-RST-001:** Hold button for 5s, release - verify reset cancelled
**TEST-RST-002:** Hold button for 10s - verify factory reset executes
**TEST-RST-003:** Verify Zigbee credentials erased (device in pairing mode)
**TEST-RST-004:** Verify factory reset counter increments
**TEST-RST-005:** Verify sensor calibration preserved (optional test)
**TEST-RST-006:** Verify LED feedback sequence
**TEST-RST-007:** Verify device boots and joins network after reset

### 3.6 Success Metrics

- [ ] Factory reset triggers after exactly 10 seconds of button hold
- [ ] All network credentials erased
- [ ] Reset counter logged correctly
- [ ] LED feedback sequence clear and intuitive
- [ ] Device fully operational after reset

---

## 4. Error Handling & Resilience

### 4.1 Purpose
Ensure the device continues operating even when individual sensors or subsystems fail.

### 4.2 Functional Requirements

**FR-ERR-001:** Sensor failures shall not crash the entire device
**FR-ERR-002:** I2C bus errors shall trigger automatic bus recovery
**FR-ERR-003:** 1-Wire bus errors shall trigger automatic retry
**FR-ERR-004:** Failed sensors shall be retried periodically (every 5 minutes)
**FR-ERR-005:** Sensor failures shall be logged and reported to diagnostics
**FR-ERR-006:** Memory allocation failures shall be logged (heap monitoring)
**FR-ERR-007:** Zigbee transmission failures shall trigger retry (max 3 attempts)

### 4.3 Technical Specifications

**Graceful Sensor Failure Handling:**
```c
typedef enum {
    SENSOR_STATUS_OK,
    SENSOR_STATUS_INIT_FAILED,
    SENSOR_STATUS_READ_FAILED,
    SENSOR_STATUS_TIMEOUT,
    SENSOR_STATUS_DISABLED
} sensor_status_t;

typedef struct {
    sensor_status_t status;
    uint32_t failure_count;
    uint32_t last_success_timestamp;
    uint32_t last_retry_timestamp;
} sensor_state_t;

sensor_state_t bh1750_state = {.status = SENSOR_STATUS_OK};
sensor_state_t ds18b20_state = {.status = SENSOR_STATUS_OK};
sensor_state_t dht11_state = {.status = SENSOR_STATUS_OK};

// Sensor read with error handling
bool read_bh1750_with_retry(float *lux_value) {
    esp_err_t err = read_bh1750_raw(lux_value);

    if (err != ESP_OK) {
        bh1750_state.status = SENSOR_STATUS_READ_FAILED;
        bh1750_state.failure_count++;

        ESP_LOGW(TAG, "BH1750 read failed (count: %lu): %s",
                 bh1750_state.failure_count, esp_err_to_name(err));

        // Trigger I2C bus recovery
        if (bh1750_state.failure_count % 3 == 0) {
            ESP_LOGI(TAG, "Attempting I2C bus recovery...");
            recover_i2c_bus();
        }

        return false;
    }

    // Success - reset failure count
    bh1750_state.status = SENSOR_STATUS_OK;
    bh1750_state.failure_count = 0;
    bh1750_state.last_success_timestamp = esp_log_timestamp() / 1000;

    return true;
}

// Sensor task with error resilience
void sensor_task(void *pvParameters) {
    float lux = 0.0f;
    float temp_outdoor = 0.0f;
    float temp_indoor = 0.0f;
    float humidity = 0.0f;

    while (1) {
        // Read BH1750 (continue even if failed)
        if (read_bh1750_with_retry(&lux)) {
            report_illuminance_to_zigbee(lux);
        }

        // Read DS18B20 (continue even if failed)
        if (read_ds18b20_with_retry(&temp_outdoor)) {
            report_outdoor_temp_to_zigbee(temp_outdoor);
        }

        // Read DHT11 (continue even if failed)
        if (read_dht11_with_retry(&temp_indoor, &humidity)) {
            report_indoor_climate_to_zigbee(temp_indoor, humidity);
        }

        // Reset watchdog
        esp_task_wdt_reset();

        vTaskDelay(pdMS_TO_TICKS(10000));  // 10 second loop
    }
}
```

**I2C Bus Recovery:**
```c
void recover_i2c_bus(void) {
    ESP_LOGW(TAG, "Performing I2C bus recovery...");

    // Delete I2C driver
    i2c_driver_delete(I2C_NUM_0);

    // Wait for bus to settle
    vTaskDelay(pdMS_TO_TICKS(100));

    // Reinitialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_1,
        .scl_io_num = GPIO_NUM_2,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,  // 100 kHz
    };

    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
    if (err == ESP_OK) {
        err = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus recovery successful");
    } else {
        ESP_LOGE(TAG, "I2C bus recovery failed: %s", esp_err_to_name(err));
    }
}
```

**1-Wire Bus Recovery:**
```c
bool read_ds18b20_with_retry(float *temperature) {
    const int MAX_RETRIES = 3;
    esp_err_t err = ESP_FAIL;

    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        err = read_ds18b20_raw(temperature);

        if (err == ESP_OK) {
            ds18b20_state.status = SENSOR_STATUS_OK;
            ds18b20_state.failure_count = 0;
            ds18b20_state.last_success_timestamp = esp_log_timestamp() / 1000;
            return true;
        }

        ESP_LOGW(TAG, "DS18B20 read failed (retry %d/%d): %s",
                 retry + 1, MAX_RETRIES, esp_err_to_name(err));

        // Wait before retry (exponential backoff: 100ms, 200ms, 400ms)
        vTaskDelay(pdMS_TO_TICKS(100 * (1 << retry)));
    }

    // All retries failed
    ds18b20_state.status = SENSOR_STATUS_READ_FAILED;
    ds18b20_state.failure_count++;

    ESP_LOGE(TAG, "DS18B20 read failed after %d retries (total failures: %lu)",
             MAX_RETRIES, ds18b20_state.failure_count);

    return false;
}
```

**Periodic Sensor Retry (Failed Sensors):**
```c
void sensor_recovery_task(void *pvParameters) {
    const uint32_t RETRY_INTERVAL_SEC = 300;  // 5 minutes

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL_SEC * 1000));

        uint32_t current_time = esp_log_timestamp() / 1000;

        // Check if BH1750 needs retry
        if (bh1750_state.status != SENSOR_STATUS_OK) {
            ESP_LOGI(TAG, "Retrying failed BH1750 sensor...");
            float lux;
            if (read_bh1750_with_retry(&lux)) {
                ESP_LOGI(TAG, "BH1750 sensor recovered!");
            }
        }

        // Check if DS18B20 needs retry
        if (ds18b20_state.status != SENSOR_STATUS_OK) {
            ESP_LOGI(TAG, "Retrying failed DS18B20 sensor...");
            float temp;
            if (read_ds18b20_with_retry(&temp)) {
                ESP_LOGI(TAG, "DS18B20 sensor recovered!");
            }
        }

        // Check if DHT11 needs retry
        if (dht11_state.status != SENSOR_STATUS_OK) {
            ESP_LOGI(TAG, "Retrying failed DHT11 sensor...");
            float temp, humidity;
            if (read_dht11_with_retry(&temp, &humidity)) {
                ESP_LOGI(TAG, "DHT11 sensor recovered!");
            }
        }
    }
}
```

**Memory Leak Detection:**
```c
void heap_monitor_task(void *pvParameters) {
    const uint32_t MONITOR_INTERVAL_SEC = 60;  // Monitor every 60 seconds
    size_t min_free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_SEC * 1000));

        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

        ESP_LOGI(TAG, "Heap - Free: %u bytes, Largest block: %u bytes, Min ever: %u bytes",
                 free_heap, largest_block, min_free_heap);

        // Update minimum free heap
        if (free_heap < min_free_heap) {
            min_free_heap = free_heap;
        }

        // Warn if heap is getting low (<50KB)
        if (free_heap < 50000) {
            ESP_LOGW(TAG, "WARNING: Low heap memory! Free: %u bytes", free_heap);
        }

        // Critical warning if heap < 20KB
        if (free_heap < 20000) {
            ESP_LOGE(TAG, "CRITICAL: Very low heap! Free: %u bytes", free_heap);
            // Could trigger diagnostic dump here
        }
    }
}
```

**Zigbee Transmission Retry:**
```c
bool send_zigbee_attribute_with_retry(uint8_t endpoint, uint16_t cluster_id,
                                       uint16_t attr_id, void *value, size_t value_len) {
    const int MAX_TX_RETRIES = 3;
    esp_err_t err = ESP_FAIL;

    for (int retry = 0; retry < MAX_TX_RETRIES; retry++) {
        err = esp_zb_zcl_set_attribute_val(endpoint, cluster_id,
                                            ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                            attr_id, value, false);

        if (err == ESP_OK) {
            ESP_LOGD(TAG, "Zigbee TX success (EP%u, cluster 0x%04X)",
                     endpoint, cluster_id);
            return true;
        }

        ESP_LOGW(TAG, "Zigbee TX failed (retry %d/%d): %s",
                 retry + 1, MAX_TX_RETRIES, esp_err_to_name(err));

        vTaskDelay(pdMS_TO_TICKS(500));  // Wait 500ms before retry
    }

    ESP_LOGE(TAG, "Zigbee TX failed after %d retries", MAX_TX_RETRIES);
    return false;
}
```

### 4.4 Testing Requirements

**TEST-ERR-001:** Disconnect BH1750 - verify device continues with other sensors
**TEST-ERR-002:** Trigger I2C bus hang - verify automatic recovery
**TEST-ERR-003:** Disconnect DS18B20 - verify retry mechanism works
**TEST-ERR-004:** Verify failed sensors retry every 5 minutes
**TEST-ERR-005:** Monitor heap usage over 24 hours - verify no leaks
**TEST-ERR-006:** Simulate Zigbee coordinator offline - verify retry logic
**TEST-ERR-007:** Verify all sensors can recover after temporary failure

### 4.5 Success Metrics

- [ ] Device remains operational with 0/3, 1/3, 2/3 sensors working
- [ ] I2C bus recovery successful after induced hang
- [ ] Failed sensors automatically retry and recover
- [ ] Heap usage remains stable over 24+ hours (no leaks)
- [ ] Zigbee transmission retry logic prevents data loss

---

## 5. Testing & Validation

### 5.1 Unit Testing

Each component shall have dedicated unit tests:

**Watchdog Timer Tests:**
- Test watchdog timeout configuration
- Test watchdog reset in normal operation
- Test watchdog trigger on infinite loop
- Test reset counter persistence

**Network Reconnection Tests:**
- Test disconnection detection
- Test exponential backoff timing
- Test uptime statistics accuracy
- Test LQI/RSSI monitoring

**Factory Reset Tests:**
- Test button hold duration detection
- Test credential erasure
- Test reset counter
- Test LED feedback sequence

**Error Handling Tests:**
- Test sensor failure scenarios
- Test I2C/1-Wire bus recovery
- Test periodic retry mechanism
- Test heap monitoring

### 5.2 Integration Testing

**24-Hour Stability Test:**
- Run device continuously for 24 hours
- Monitor for crashes, resets, memory leaks
- Log all errors and warnings
- Verify uptime > 99.9% (< 86 seconds downtime)

**Network Stress Test:**
- Disconnect/reconnect coordinator 100 times
- Verify automatic recovery every time
- Measure average reconnection time (<5 minutes)

**Sensor Failure Test:**
- Disconnect sensors during operation
- Verify graceful handling
- Reconnect sensors - verify recovery
- Test all combinations (0/3, 1/3, 2/3 sensors)

**Power Cycling Test:**
- Power cycle device 100 times
- Verify clean boot every time
- Verify no watchdog resets on normal boot
- Verify network rejoin after each boot

### 5.3 Field Testing

**Production Environment Test (1 week):**
- Deploy device in real environment
- Monitor remotely via Home Assistant
- Track uptime, disconnections, sensor failures
- Collect logs for analysis

---

## 6. Success Metrics

### 6.1 Reliability Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Uptime | >99.9% | Log uptime over 7 days |
| Crash recovery | <35s | Watchdog timeout test |
| Network reconnection | <5min | Coordinator power cycle test |
| Sensor failure recovery | 100% | Periodic retry verification |
| Factory reset reliability | 100% | 10 consecutive reset tests |

### 6.2 Performance Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Heap usage stability | <5% variance | 24-hour heap monitoring |
| Watchdog false positives | 0 | 24-hour normal operation |
| I2C bus recovery success | >95% | Induced I2C hang tests |
| Zigbee TX success rate | >99% | Transmission logging |

---

## 7. Implementation Timeline

### Week 1: Watchdog & Error Handling
- **Day 1-2:** Implement watchdog timer and reset logging
- **Day 3-4:** Implement sensor error handling and bus recovery
- **Day 5:** Unit testing watchdog and error handling
- **Day 6-7:** Integration testing, bug fixes

### Week 2: Network Reconnection & Factory Reset
- **Day 1-2:** Implement network disconnection detection and reconnection logic
- **Day 3-4:** Implement factory reset mechanism and LED feedback
- **Day 5:** Unit testing reconnection and factory reset
- **Day 6-7:** Integration testing, 24-hour stability test

### Week 3: Final Testing & Documentation
- **Day 1-3:** Field testing in production environment
- **Day 4-5:** Bug fixes and optimization
- **Day 6-7:** Update documentation, code review

**Total Estimated Time:** 3 weeks

---

## 8. References

### 8.1 ESP-IDF Documentation
- [Task Watchdog Timer](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c6/api-reference/system/wdts.html)
- [Non-Volatile Storage](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c6/api-reference/storage/nvs_flash.html)
- [Reset Reason](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c6/api-reference/system/system.html#reset-reason)
- [Heap Memory Debugging](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c6/api-reference/system/heap_debug.html)

### 8.2 ESP-Zigbee SDK Documentation
- [ESP-Zigbee SDK API Reference](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/)
- [Zigbee Network Management](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/api-reference/network/index.html)
- [Factory Reset](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/api-reference/esp_zigbee_core.html#_CPPv421esp_zb_factory_resetv)

### 8.3 Internal Documentation
- [ESP32 Zigbee Development Plan](esp32-zigbee-development-plan.md)
- [Waveshare ESP32-C6-Zero Notes](hardware/waveshare-esp32-c6-zero-notes.md)
- [Zigbee Multi-Sensor README](../zigbee-multi-sensor/README.md)

---

## 9. Approval & Sign-Off

**Document Author:** HomeAssistant-Agent
**Review Required:** Anthony (Project Owner)
**Approval Status:** ⏳ Pending Review

**Change Log:**
| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-14 | HomeAssistant-Agent | Initial requirements document |

---

**End of Document**
