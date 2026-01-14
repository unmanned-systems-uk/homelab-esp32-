/*
 * ESP32-C6 Zigbee Multi-Sensor
 *
 * Device Type: Router/Repeater (always powered)
 * Sensors: BH1750 (light), DS18B20 (outdoor temp), DHT11 (indoor temp/humidity)
 *
 * Zigbee Endpoints:
 * - EP 10: DHT11 Indoor (Temperature + Humidity clusters)
 * - EP 11: DS18B20 Outdoor (Temperature cluster)
 * - EP 12: BH1750 Light (Illuminance cluster)
 * - EP 13: Reserved for HLK-LD2450 (future)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

// ESP-IDF Zigbee includes
#include "esp_zigbee_core.h"
#include "esp_zigbee_cluster.h"

// WS2812 RGB LED
#include "led_strip.h"

// ========================================
// Configuration
// ========================================

// Zigbee Configuration
#define INSTALLCODE_POLICY_ENABLE       false
#define ED_AGING_TIMEOUT                ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE                   3000    // 3000 seconds
#define ESP_ZB_PRIMARY_CHANNEL_MASK     ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK

// Reporting Mode Configuration
// EXPLICIT: Immediate reports every sensor read (high traffic, instant updates)
// AUTOMATIC: Reports based on HA intervals/thresholds (efficient, 30s-15min delay)
#define REPORTING_MODE_EXPLICIT         1
#define REPORTING_MODE_AUTOMATIC        2
#define ZIGBEE_REPORTING_MODE           REPORTING_MODE_EXPLICIT  // Change to REPORTING_MODE_AUTOMATIC for lower traffic

// Device Information
#define ESP_ZB_MANUFACTURER_NAME        "UnmannedSystems"
#define ESP_ZB_MODEL_IDENTIFIER         "ESP32-C6-MultiSensor"
#define ESP_ZB_DEVICE_VERSION           1
#define ESP_ZB_POWER_SOURCE             0x01    // Mains (single phase)

// GPIO Pins (Waveshare ESP32-C6-Zero)
#define LED_BUILTIN                     15      // Simple LED (ON when Zigbee connected)
#define WS2812_GPIO                     8       // RGB LED data pin
#define I2C_MASTER_SDA_IO               1
#define I2C_MASTER_SCL_IO               2
#define DS18B20_GPIO                    5
#define DHT11_GPIO                      4

// WS2812 RGB LED Configuration
#define WS2812_LED_COUNT                1       // Single RGB LED
#define WS2812_RMT_CHANNEL              0       // RMT channel for WS2812

// I2C Configuration (for BH1750)
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          100000  // 100kHz
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

// BH1750 Device Address and Commands
#define BH1750_ADDR                 0x23
#define BH1750_POWER_ON             0x01
#define BH1750_RESET                0x07
#define BH1750_CONTINUOUS_HIGH_RES  0x10

// DS18B20 Commands
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_SKIP_ROM        0xCC

// DHT11 Configuration
#define DHT11_TIMEOUT_US            1000    // Timeout for bit reads

// Temperature Calibration Offsets (°C)
// TODO: Adjust these based on your reference thermometer
// Positive = sensor reads HIGH, subtract to correct
// Negative = sensor reads LOW, add to correct
#define DS18B20_OFFSET_C            -1.0f   // Outdoor sensor calibration
#define DHT11_OFFSET_C              -1.0f   // Indoor sensor calibration

// Sensor update intervals (milliseconds)
#define BH1750_UPDATE_INTERVAL          30000   // 30 seconds
#define DS18B20_UPDATE_INTERVAL         60000   // 60 seconds
#define DHT11_UPDATE_INTERVAL           60000   // 60 seconds

// Zigbee Endpoint IDs
#define EP_DHT11_INDOOR                 10
#define EP_DS18B20_OUTDOOR              11
#define EP_BH1750_LIGHT                 12
#define EP_LD2450_PRESENCE              13      // Reserved for future
#define EP_REPORTING_MODE_SWITCH        14      // Debug: Reporting mode control

static const char *TAG = "ZIGBEE_SENSOR";

// ========================================
// Reporting Mode Control
// ========================================
// Runtime-switchable reporting mode (controlled by HA switch on EP14)
// true = EXPLICIT (instant reports), false = AUTOMATIC (efficient)
static bool use_explicit_reporting = true;

// ========================================
// WS2812 RGB LED Handle
// ========================================
static led_strip_handle_t led_strip = NULL;

// ========================================
// Zigbee Attribute Defaults
// ========================================

// Temperature measurement defaults
#define ZB_TEMP_INVALID                 0x8000  // Invalid temperature
#define ZB_TEMP_MIN                     -5000   // -50.00°C
#define ZB_TEMP_MAX                     12500   // 125.00°C

// Humidity measurement defaults
#define ZB_HUMIDITY_INVALID             0xFFFF  // Invalid humidity
#define ZB_HUMIDITY_MIN                 0       // 0% RH
#define ZB_HUMIDITY_MAX                 10000   // 100.00% RH

// Illuminance measurement defaults
#define ZB_ILLUM_INVALID                0xFFFF  // Invalid illuminance
#define ZB_ILLUM_MIN                    1       // 1 lux
#define ZB_ILLUM_MAX                    0xFFFE  // 65534 lux

// ========================================
// Forward Declarations
// ========================================

static void esp_zb_task(void *pvParameters);
static void bh1750_sensor_task(void *pvParameters);
static void ds18b20_sensor_task(void *pvParameters);
static void dht11_sensor_task(void *pvParameters);

// ========================================
// WS2812 RGB LED Functions
// ========================================

/**
 * Initialize WS2812 RGB LED
 */
static esp_err_t ws2812_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = WS2812_LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip: %s", esp_err_to_name(ret));
        return ret;
    }

    // Clear LED on init
    led_strip_clear(led_strip);
    ESP_LOGI(TAG, "WS2812 RGB LED initialized on GPIO%d", WS2812_GPIO);
    return ESP_OK;
}

/**
 * Set RGB color and optionally flash
 */
static void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b, bool flash, uint32_t duration_ms)
{
    if (led_strip == NULL) return;

    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);

    if (flash && duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        led_strip_clear(led_strip);
    }
}

/**
 * Visual Indicators:
 * - Green flash: Sensor read successful
 * - Red flash: Sensor read failed
 * - Blue flash: Zigbee message sent successfully
 * - Purple: Zigbee searching for network
 * - Yellow: Sensor initializing
 * - White flash: System operational check
 */

static void led_sensor_ok(void) {
    ws2812_set_color(0, 50, 0, true, 100);  // Green flash (100ms)
}

static void led_sensor_error(void) {
    ws2812_set_color(50, 0, 0, true, 200);  // Red flash (200ms)
}

static void led_zigbee_tx(void) {
    ws2812_set_color(0, 0, 50, true, 100);  // Blue flash (100ms)
}

static void led_zigbee_searching(void) {
    ws2812_set_color(40, 0, 20, false, 0);  // Purple/Magenta solid (more red than blue)
}

static void led_sensor_init(void) {
    ws2812_set_color(50, 25, 0, true, 150);  // Yellow/Orange flash (150ms)
}

static void led_system_ok(void) {
    ws2812_set_color(20, 20, 20, true, 100);  // White flash (100ms)
}

static void led_zigbee_connected(void) {
    // Quick cyan flash to indicate connection
    ws2812_set_color(0, 50, 50, true, 200);  // Cyan flash (200ms)
}

static void led_off(void) {
    if (led_strip) led_strip_clear(led_strip);
}

// ========================================
// Zigbee Attribute Reporting Helper Functions
// ========================================

/**
 * Report attribute using AUTOMATIC mode (passive)
 * Marks attribute as changed; reports sent based on HA's configured intervals
 */
static void report_attribute_automatic(uint8_t endpoint, uint16_t cluster_id,
                                       uint16_t attr_id, void *value)
{
    esp_zb_zcl_set_attribute_val(
        endpoint,
        cluster_id,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        attr_id,
        value,
        true  // Mark as changed for automatic reporting
    );
}

/**
 * Report attribute using EXPLICIT mode (active)
 * Immediately sends report command to coordinator
 */
static void report_attribute_explicit(uint8_t endpoint, uint16_t cluster_id,
                                      uint16_t attr_id, void *value)
{
    // First, update the local attribute value
    esp_zb_zcl_set_attribute_val(
        endpoint,
        cluster_id,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        attr_id,
        value,
        false  // Don't auto-report
    );

    // Then explicitly send report to coordinator
    esp_zb_zcl_report_attr_cmd_t report_cmd = {
        .zcl_basic_cmd = {
            .src_endpoint = endpoint,
            .dst_endpoint = 1,  // Coordinator endpoint
            .dst_addr_u = {
                .addr_short = 0x0000,  // Coordinator short address
            },
        },
        .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        .clusterID = cluster_id,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .attributeID = attr_id,
    };
    esp_zb_zcl_report_attr_cmd_req(&report_cmd);
}

/**
 * Report attribute with runtime-switchable mode
 * Mode controlled by Home Assistant switch on EP14
 */
static void report_attribute(uint8_t endpoint, uint16_t cluster_id,
                             uint16_t attr_id, void *value)
{
    if (use_explicit_reporting) {
        report_attribute_explicit(endpoint, cluster_id, attr_id, value);
    } else {
        report_attribute_automatic(endpoint, cluster_id, attr_id, value);
    }
}

// ========================================
// 1-Wire Protocol Implementation
// ========================================

static void onewire_delay_us(uint32_t us)
{
    ets_delay_us(us);
}

static esp_err_t onewire_reset(gpio_num_t pin)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    onewire_delay_us(480);

    gpio_set_direction(pin, GPIO_MODE_INPUT);
    onewire_delay_us(70);

    int present = gpio_get_level(pin);
    onewire_delay_us(410);

    return (present == 0) ? ESP_OK : ESP_FAIL;
}

static void onewire_write_bit(gpio_num_t pin, int bit)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);

    if (bit) {
        onewire_delay_us(10);
        gpio_set_level(pin, 1);
        onewire_delay_us(55);
    } else {
        onewire_delay_us(65);
        gpio_set_level(pin, 1);
        onewire_delay_us(5);
    }
}

static int onewire_read_bit(gpio_num_t pin)
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    onewire_delay_us(3);

    gpio_set_direction(pin, GPIO_MODE_INPUT);
    onewire_delay_us(10);

    int bit = gpio_get_level(pin);
    onewire_delay_us(53);

    return bit;
}

static void onewire_write_byte(gpio_num_t pin, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(pin, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t onewire_read_byte(gpio_num_t pin)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (onewire_read_bit(pin)) {
            byte |= 0x80;
        }
    }
    return byte;
}

// ========================================
// BH1750 Functions
// ========================================

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t bh1750_write_command(uint8_t command)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BH1750_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t bh1750_read_light(float *lux)
{
    uint8_t data[2];

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BH1750_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        uint16_t raw = (data[0] << 8) | data[1];
        *lux = raw / 1.2;
    }

    return ret;
}

static esp_err_t bh1750_init(void)
{
    esp_err_t ret;

    ret = bh1750_write_command(BH1750_POWER_ON);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750: Failed to power on");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = bh1750_write_command(BH1750_RESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750: Failed to reset");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = bh1750_write_command(BH1750_CONTINUOUS_HIGH_RES);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750: Failed to set measurement mode");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(120));

    return ESP_OK;
}

// ========================================
// DS18B20 Functions
// ========================================

static esp_err_t ds18b20_init(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);

    return onewire_reset(pin);
}

static esp_err_t ds18b20_start_conversion(gpio_num_t pin)
{
    esp_err_t ret = onewire_reset(pin);
    if (ret != ESP_OK) {
        return ret;
    }

    onewire_write_byte(pin, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(pin, DS18B20_CMD_CONVERT_T);

    return ESP_OK;
}

static esp_err_t ds18b20_read_temperature(gpio_num_t pin, float *temperature)
{
    esp_err_t ret = onewire_reset(pin);
    if (ret != ESP_OK) {
        return ret;
    }

    onewire_write_byte(pin, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(pin, DS18B20_CMD_READ_SCRATCHPAD);

    uint8_t data[9];
    for (int i = 0; i < 9; i++) {
        data[i] = onewire_read_byte(pin);
    }

    // Calculate temperature from raw data
    int16_t raw = (data[1] << 8) | data[0];
    *temperature = (float)raw / 16.0;

    return ESP_OK;
}

// ========================================
// DHT11 Functions
// ========================================

static esp_err_t dht11_wait_for_level(gpio_num_t pin, int level, uint32_t timeout_us)
{
    uint32_t start = esp_timer_get_time();
    while (gpio_get_level(pin) != level) {
        if ((esp_timer_get_time() - start) > timeout_us) {
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

static esp_err_t dht11_read_bit(gpio_num_t pin, uint8_t *bit)
{
    // Wait for low phase (start of bit)
    if (dht11_wait_for_level(pin, 0, DHT11_TIMEOUT_US) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }

    // Wait for high phase
    if (dht11_wait_for_level(pin, 1, DHT11_TIMEOUT_US) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }

    // Measure high pulse duration
    uint32_t start = esp_timer_get_time();
    if (dht11_wait_for_level(pin, 0, DHT11_TIMEOUT_US) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }
    uint32_t duration = esp_timer_get_time() - start;

    // Bit is 1 if high phase > 40us, otherwise 0
    *bit = (duration > 40) ? 1 : 0;

    return ESP_OK;
}

static esp_err_t dht11_read_data(gpio_num_t pin, float *temperature, float *humidity)
{
    uint8_t data[5] = {0};

    // Send start signal: pull low for 18ms
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(18));

    // Release and wait 20-40us
    gpio_set_level(pin, 1);
    onewire_delay_us(30);

    // Switch to input mode
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    // Wait for DHT11 response: low (80us) then high (80us)
    if (dht11_wait_for_level(pin, 0, 100) != ESP_OK) {
        ESP_LOGE(TAG, "DHT11: No response (timeout waiting for low)");
        return ESP_FAIL;
    }

    if (dht11_wait_for_level(pin, 1, 100) != ESP_OK) {
        ESP_LOGE(TAG, "DHT11: No response (timeout waiting for high)");
        return ESP_FAIL;
    }

    if (dht11_wait_for_level(pin, 0, 100) != ESP_OK) {
        ESP_LOGE(TAG, "DHT11: No response (timeout after high)");
        return ESP_FAIL;
    }

    // Read 40 bits (5 bytes)
    for (int i = 0; i < 40; i++) {
        uint8_t bit;
        if (dht11_read_bit(pin, &bit) != ESP_OK) {
            ESP_LOGE(TAG, "DHT11: Timeout reading bit %d", i);
            return ESP_FAIL;
        }
        data[i / 8] <<= 1;
        data[i / 8] |= bit;
    }

    // Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGE(TAG, "DHT11: Checksum error: calc=0x%02X, recv=0x%02X", checksum, data[4]);
        return ESP_FAIL;
    }

    // DHT11 returns integer values in data[0] (humidity) and data[2] (temperature)
    *humidity = (float)data[0];
    *temperature = (float)data[2];

    return ESP_OK;
}

static esp_err_t dht11_init(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);

    return ESP_OK;
}

// ========================================
// Zigbee Diagnostics
// ========================================

static bool zigbee_connected = false;
static uint8_t zigbee_channel = 0;
static uint16_t zigbee_short_addr = 0xFFFF;
static uint8_t zigbee_pan_id[8] = {0};

static void zigbee_print_diagnostics(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Zigbee Status:");
    if (zigbee_connected) {
        ESP_LOGI(TAG, "  Connected:    YES");
        ESP_LOGI(TAG, "  Channel:      %d", zigbee_channel);
        ESP_LOGI(TAG, "  Short Addr:   0x%04X", zigbee_short_addr);
        ESP_LOGI(TAG, "  PAN ID:       %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                 zigbee_pan_id[7], zigbee_pan_id[6], zigbee_pan_id[5], zigbee_pan_id[4],
                 zigbee_pan_id[3], zigbee_pan_id[2], zigbee_pan_id[1], zigbee_pan_id[0]);
        ESP_LOGI(TAG, "  Endpoints:    10 (DHT11), 11 (DS18B20), 12 (BH1750), 14 (Mode Switch)");
    } else {
        ESP_LOGI(TAG, "  Connected:    NO (searching...)");
    }
    ESP_LOGI(TAG, "  Report Mode:  %s", use_explicit_reporting ? "EXPLICIT (instant)" : "AUTOMATIC (efficient)");
    ESP_LOGI(TAG, "========================================");
}

// ========================================
// Zigbee Stack Event Handler
// ========================================

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode",
                     sig_type == ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START ? "" : "non");
            ESP_LOGI(TAG, "Start network steering");

            // Purple LED indicates searching for network
            led_zigbee_searching();

            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            ESP_LOGE(TAG, "Failed to initialize Zigbee stack (status: %s)",
                     esp_err_to_name(err_status));
            led_sensor_error();  // Red flash for init failure
        }
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "✓ Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0]);

            // Update diagnostic variables
            zigbee_connected = true;
            memcpy(zigbee_pan_id, extended_pan_id, 8);
            zigbee_channel = esp_zb_get_current_channel();
            zigbee_short_addr = esp_zb_get_short_address();

            // Turn LED on to indicate connected
            gpio_set_level(LED_BUILTIN, 1);

            // Cyan flash indicates successful connection
            led_zigbee_connected();

            // Turn off RGB LED after connection (builtin LED stays on)
            vTaskDelay(pdMS_TO_TICKS(300));
            led_off();

            // Print diagnostics
            zigbee_print_diagnostics();
        } else {
            zigbee_connected = false;
            ESP_LOGW(TAG, "Network steering was not successful (status: %s)",
                     esp_err_to_name(err_status));
            ESP_LOGW(TAG, "Ensure coordinator is in pairing mode! Retrying in 3s...");

            // Keep purple/magenta LED on while searching
            led_zigbee_searching();

            // Retry after 3 seconds
            esp_zb_scheduler_alarm((esp_zb_callback_t)esp_zb_bdb_start_top_level_commissioning,
                                   ESP_ZB_BDB_MODE_NETWORK_STEERING, 3000);
        }
        break;

    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s",
                 esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

// Core action handler wrapper for ESP-Zigbee SDK 1.0.9+
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;

    switch (callback_id) {
    case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
    case ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID:
    case ESP_ZB_CORE_CMD_WRITE_ATTR_RESP_CB_ID:
        // These are handled by the stack default handlers
        break;

    case ESP_ZB_CORE_CMD_REPORT_CONFIG_RESP_CB_ID:
        ESP_LOGI(TAG, "📊 Report config response received from coordinator");
        break;

    case ESP_ZB_CORE_CMD_READ_REPORT_CFG_RESP_CB_ID:
        break;

    case ESP_ZB_CORE_REPORT_ATTR_CB_ID:
        ESP_LOGI(TAG, "📡 Attribute report SENT to coordinator");
        break;

    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        {
            esp_zb_zcl_set_attr_value_message_t *attr_msg = (esp_zb_zcl_set_attr_value_message_t *)message;

            // Check if this is the reporting mode switch (EP14, On/Off cluster)
            if (attr_msg->info.dst_endpoint == EP_REPORTING_MODE_SWITCH &&
                attr_msg->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF &&
                attr_msg->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID) {

                // Read the new switch state
                bool *on_off_value = (bool *)attr_msg->attribute.data.value;
                use_explicit_reporting = *on_off_value;

                ESP_LOGI(TAG, "🔄 Reporting mode changed to: %s",
                         use_explicit_reporting ? "EXPLICIT (instant reports)" : "AUTOMATIC (efficient)");

                // Update diagnostics display
                zigbee_print_diagnostics();
            }
        }
        break;

    case ESP_ZB_CORE_CMD_DISC_ATTR_RESP_CB_ID:
    case ESP_ZB_CORE_CMD_CUSTOM_CLUSTER_REQ_CB_ID:
    case ESP_ZB_CORE_CMD_CUSTOM_CLUSTER_RESP_CB_ID:
        // These are handled by the stack default handlers
        break;

    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }

    return ret;
}

// ========================================
// Zigbee Device Configuration
// ========================================

static void esp_zb_create_device_clusters(void)
{
    esp_zb_cluster_list_t *esp_zb_cluster_list;

    // ========================================
    // Endpoint 10: DHT11 Indoor (Temp + Humidity)
    // ========================================

    esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    // Basic cluster with location description
    esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(NULL);
    esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  ESP_ZB_MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  ESP_ZB_MODEL_IDENTIFIER);

    // Add location description for Indoor sensors
    char *indoor_location = "Indoor";
    esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_LOCATION_DESCRIPTION_ID,
                                  indoor_location);

    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, basic_cluster,
                                          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Temperature measurement cluster (indoor)
    esp_zb_temperature_meas_cluster_cfg_t temp_cfg = {
        .measured_value = ZB_TEMP_INVALID,
        .min_value = ZB_TEMP_MIN,
        .max_value = ZB_TEMP_MAX,
    };
    esp_zb_attribute_list_t *temp_cluster = esp_zb_temperature_meas_cluster_create(&temp_cfg);
    esp_zb_cluster_list_add_temperature_meas_cluster(esp_zb_cluster_list, temp_cluster,
                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Humidity measurement cluster
    esp_zb_humidity_meas_cluster_cfg_t humidity_cfg = {
        .measured_value = ZB_HUMIDITY_INVALID,
        .min_value = ZB_HUMIDITY_MIN,
        .max_value = ZB_HUMIDITY_MAX,
    };
    esp_zb_attribute_list_t *humidity_cluster = esp_zb_humidity_meas_cluster_create(&humidity_cfg);
    esp_zb_cluster_list_add_humidity_meas_cluster(esp_zb_cluster_list, humidity_cluster,
                                                   ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Create endpoint 10
    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list,
                          EP_DHT11_INDOOR,
                          ESP_ZB_AF_HA_PROFILE_ID,
                          ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID);

    // ========================================
    // Endpoint 11: DS18B20 Outdoor Temperature
    // ========================================

    esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    // Basic cluster with location description
    esp_zb_attribute_list_t *basic_cluster_outdoor = esp_zb_basic_cluster_create(NULL);
    esp_zb_basic_cluster_add_attr(basic_cluster_outdoor, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  ESP_ZB_MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(basic_cluster_outdoor, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  ESP_ZB_MODEL_IDENTIFIER);

    // Add location description for Outdoor sensor
    char *outdoor_location = "Outdoor";
    esp_zb_basic_cluster_add_attr(basic_cluster_outdoor, ESP_ZB_ZCL_ATTR_BASIC_LOCATION_DESCRIPTION_ID,
                                  outdoor_location);

    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, basic_cluster_outdoor,
                                          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Temperature measurement cluster (outdoor)
    temp_cfg.measured_value = ZB_TEMP_INVALID;
    temp_cluster = esp_zb_temperature_meas_cluster_create(&temp_cfg);
    esp_zb_cluster_list_add_temperature_meas_cluster(esp_zb_cluster_list, temp_cluster,
                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Create endpoint 11
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list,
                          EP_DS18B20_OUTDOOR,
                          ESP_ZB_AF_HA_PROFILE_ID,
                          ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID);

    // ========================================
    // Endpoint 12: BH1750 Illuminance
    // ========================================

    esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    // Illuminance measurement cluster
    esp_zb_illuminance_meas_cluster_cfg_t illum_cfg = {
        .measured_value = ZB_ILLUM_INVALID,
        .min_value = ZB_ILLUM_MIN,
        .max_value = ZB_ILLUM_MAX,
    };
    esp_zb_attribute_list_t *illum_cluster = esp_zb_illuminance_meas_cluster_create(&illum_cfg);
    esp_zb_cluster_list_add_illuminance_meas_cluster(esp_zb_cluster_list, illum_cluster,
                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Create endpoint 12
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list,
                          EP_BH1750_LIGHT,
                          ESP_ZB_AF_HA_PROFILE_ID,
                          ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);

    // ========================================
    // Endpoint 14: Reporting Mode Control Switch
    // ========================================

    esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    // Basic cluster
    esp_zb_attribute_list_t *basic_cluster_switch = esp_zb_basic_cluster_create(NULL);
    esp_zb_basic_cluster_add_attr(basic_cluster_switch, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  ESP_ZB_MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(basic_cluster_switch, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  ESP_ZB_MODEL_IDENTIFIER);
    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, basic_cluster_switch,
                                          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // On/Off cluster for reporting mode control
    // ON = EXPLICIT mode (instant reports), OFF = AUTOMATIC mode (efficient)
    esp_zb_on_off_cluster_cfg_t on_off_cfg = {
        .on_off = use_explicit_reporting,  // Initial state
    };
    esp_zb_attribute_list_t *on_off_cluster = esp_zb_on_off_cluster_create(&on_off_cfg);
    esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list, on_off_cluster,
                                           ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Create endpoint 14 as an On/Off Switch device
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list,
                          EP_REPORTING_MODE_SWITCH,
                          ESP_ZB_AF_HA_PROFILE_ID,
                          ESP_ZB_HA_ON_OFF_SWITCH_DEVICE_ID);

    // Register all endpoints
    esp_zb_device_register(esp_zb_ep_list);
}

static esp_err_t esp_zb_initialize_zigbee(void)
{
    // Initialize Zigbee stack as Router
    esp_zb_cfg_t zb_nwk_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,
        .nwk_cfg = {
            .zczr_cfg = {
                .max_children = 10,
            },
        },
    };
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_create_device_clusters();

    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

    // Register core action handler (ESP-Zigbee SDK 1.0.9+)
    esp_zb_core_action_handler_register(zb_action_handler);

    ESP_ERROR_CHECK(esp_zb_start(false));

    return ESP_OK;
}

// ========================================
// Sensor Tasks - Real Implementations
// ========================================

static void bh1750_sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BH1750 sensor task started");

    // Yellow flash indicates sensor initialization
    led_sensor_init();

    // Initialize I2C bus
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750: I2C initialization failed (%s)", esp_err_to_name(ret));
        led_sensor_error();  // Red flash for I2C init failure
        vTaskDelete(NULL);
        return;
    }

    // Initialize BH1750 sensor
    ret = bh1750_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750: Sensor initialization failed (%s)", esp_err_to_name(ret));
        led_sensor_error();  // Red flash for sensor init failure
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "BH1750: ✓ Initialized successfully");
    led_sensor_ok();  // Green flash for successful initialization

    while (1) {
        float lux_float;
        ret = bh1750_read_light(&lux_float);

        if (ret == ESP_OK) {
            // Convert lux to Zigbee ZCL logarithmic encoding
            // ZCL MeasuredValue = 10000 × log10(lux) + 1
            uint16_t lux_value;
            if (lux_float < 1.0f) {
                // For very low light, use minimum valid value
                lux_value = ZB_ILLUM_MIN;
            } else {
                // Apply logarithmic encoding per ZCL spec
                lux_value = (uint16_t)(10000.0f * log10f(lux_float) + 1.0f);

                // Clamp to valid range
                if (lux_value < ZB_ILLUM_MIN) lux_value = ZB_ILLUM_MIN;
                if (lux_value > ZB_ILLUM_MAX) lux_value = ZB_ILLUM_MAX;
            }

            // Report attribute (mode controlled by HA switch on EP14)
            report_attribute(
                EP_BH1750_LIGHT,
                ESP_ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                ESP_ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID,
                &lux_value
            );

            // Format for monitor.py compatibility
            ESP_LOGI(TAG, "BH1750: Light: %7.1f lux (ZCL: %u)", lux_float, lux_value);

            // Green flash for successful read
            led_sensor_ok();

            // Blue flash indicates Zigbee attribute update sent
            vTaskDelay(pdMS_TO_TICKS(50));
            led_zigbee_tx();
        } else {
            ESP_LOGW(TAG, "BH1750: Read failed (%s)", esp_err_to_name(ret));
            led_sensor_error();  // Red flash for read failure
        }

        vTaskDelay(pdMS_TO_TICKS(BH1750_UPDATE_INTERVAL));
    }
}

static void ds18b20_sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "DS18B20 sensor task started");

    // Yellow flash indicates sensor initialization
    led_sensor_init();

    // Initialize DS18B20 1-Wire interface
    esp_err_t ret = ds18b20_init(DS18B20_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DS18B20: Sensor not detected on GPIO%d", DS18B20_GPIO);
        led_sensor_error();  // Red flash for init failure
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DS18B20: ✓ Initialized successfully");
    led_sensor_ok();  // Green flash for successful initialization

    while (1) {
        // Start temperature conversion
        ret = ds18b20_start_conversion(DS18B20_GPIO);
        if (ret == ESP_OK) {
            // Wait for conversion to complete (750ms for 12-bit resolution)
            vTaskDelay(pdMS_TO_TICKS(750));

            // Read temperature
            float temp_celsius;
            ret = ds18b20_read_temperature(DS18B20_GPIO, &temp_celsius);

            if (ret == ESP_OK) {
                // Apply calibration offset
                temp_celsius += DS18B20_OFFSET_C;

                // Convert to Zigbee format (0.01°C units)
                int16_t temp_value = (int16_t)(temp_celsius * 100);

                // Clamp to valid range
                if (temp_value < ZB_TEMP_MIN) temp_value = ZB_TEMP_MIN;
                if (temp_value > ZB_TEMP_MAX) temp_value = ZB_TEMP_MAX;

                // Report attribute (mode controlled by HA switch on EP14)
                report_attribute(
                    EP_DS18B20_OUTDOOR,
                    ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                    ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                    &temp_value
                );

                // Format for monitor.py compatibility
                float temp_f = (temp_celsius * 9.0/5.0) + 32.0;
                ESP_LOGI(TAG, "DS18B20: Temp:  %6.2f °C  (%.2f °F)  [Outdoor]",
                         temp_celsius, temp_f);

                // Green flash for successful read
                led_sensor_ok();

                // Blue flash indicates Zigbee attribute update sent
                vTaskDelay(pdMS_TO_TICKS(50));
                led_zigbee_tx();
            } else {
                ESP_LOGW(TAG, "DS18B20: Read failed (%s)", esp_err_to_name(ret));
                led_sensor_error();  // Red flash for read failure
            }
        } else {
            ESP_LOGW(TAG, "DS18B20: Conversion start failed (%s)", esp_err_to_name(ret));
            led_sensor_error();  // Red flash for conversion failure
        }

        vTaskDelay(pdMS_TO_TICKS(DS18B20_UPDATE_INTERVAL));
    }
}

static void dht11_sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "DHT11 sensor task started");

    // Yellow flash indicates sensor initialization
    led_sensor_init();

    // Initialize DHT11 GPIO
    esp_err_t ret = dht11_init(DHT11_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DHT11: GPIO initialization failed");
        led_sensor_error();  // Red flash for init failure
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DHT11: ✓ Initialized successfully");
    led_sensor_ok();  // Green flash for successful initialization

    // DHT11 needs time to stabilize after power-on
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        float temp_celsius, humidity_percent;
        ret = dht11_read_data(DHT11_GPIO, &temp_celsius, &humidity_percent);

        if (ret == ESP_OK) {
            // Apply calibration offset to temperature
            temp_celsius += DHT11_OFFSET_C;

            // Convert to Zigbee formats
            int16_t temp_value = (int16_t)(temp_celsius * 100);      // 0.01°C units
            uint16_t humidity_value = (uint16_t)(humidity_percent * 100); // 0.01% units

            // Clamp to valid ranges
            if (temp_value < ZB_TEMP_MIN) temp_value = ZB_TEMP_MIN;
            if (temp_value > ZB_TEMP_MAX) temp_value = ZB_TEMP_MAX;
            if (humidity_value < ZB_HUMIDITY_MIN) humidity_value = ZB_HUMIDITY_MIN;
            if (humidity_value > ZB_HUMIDITY_MAX) humidity_value = ZB_HUMIDITY_MAX;

            // Report temperature (mode controlled by HA switch on EP14)
            report_attribute(
                EP_DHT11_INDOOR,
                ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                &temp_value
            );

            // Report humidity (mode controlled by HA switch on EP14)
            report_attribute(
                EP_DHT11_INDOOR,
                ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
                ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
                &humidity_value
            );

            // Format for monitor.py compatibility
            float temp_f = (temp_celsius * 9.0/5.0) + 32.0;
            ESP_LOGI(TAG, "DHT11: Temp:  %6.1f °C  (%.1f °F)  [Indoor]",
                     temp_celsius, temp_f);
            ESP_LOGI(TAG, "DHT11: Humid: %6.1f %%", humidity_percent);

            // Green flash for successful read
            led_sensor_ok();

            // Blue flash indicates Zigbee attribute update sent
            vTaskDelay(pdMS_TO_TICKS(50));
            led_zigbee_tx();
        } else {
            ESP_LOGW(TAG, "DHT11: Read failed (%s)", esp_err_to_name(ret));
            led_sensor_error();  // Red flash for read failure
        }

        vTaskDelay(pdMS_TO_TICKS(DHT11_UPDATE_INTERVAL));
    }
}

// ========================================
// Zigbee Main Task
// ========================================

static void esp_zb_task(void *pvParameters)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-C6 Zigbee Multi-Sensor");
    ESP_LOGI(TAG, "Device: Router/Repeater");
    ESP_LOGI(TAG, "Sensors: BH1750 + DS18B20 + DHT11");
    ESP_LOGI(TAG, "========================================");

    // Initialize Zigbee stack
    esp_zb_initialize_zigbee();

    // Start sensor tasks
    xTaskCreate(bh1750_sensor_task, "bh1750_task", 4096, NULL, 5, NULL);
    xTaskCreate(ds18b20_sensor_task, "ds18b20_task", 4096, NULL, 5, NULL);
    xTaskCreate(dht11_sensor_task, "dht11_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Zigbee stack started, sensor tasks running");

    // Main Zigbee loop - this is a blocking call that handles Zigbee events
    esp_zb_main_loop_iteration();
}

// ========================================
// Main Application Entry
// ========================================

void app_main(void)
{
    // Initialize NVS (required for Zigbee)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize LED GPIO
    gpio_reset_pin(LED_BUILTIN);
    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_BUILTIN, 0);  // Off until network joined

    // Initialize WS2812 RGB LED
    ret = ws2812_init();
    if (ret == ESP_OK) {
        // System startup - quick white flash
        led_system_ok();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Start Zigbee task
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
