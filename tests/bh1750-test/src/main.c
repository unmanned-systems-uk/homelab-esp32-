#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

// BH1750 I2C Configuration
#define I2C_MASTER_SCL_IO           2       // GPIO2
#define I2C_MASTER_SDA_IO           1       // GPIO1
#define I2C_MASTER_NUM              0       // I2C port 0
#define I2C_MASTER_FREQ_HZ          100000  // 100kHz
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

// BH1750 Device Address and Commands
#define BH1750_ADDR                 0x23    // Default address (ADDR pin to GND)
#define BH1750_POWER_ON             0x01
#define BH1750_RESET                0x07
#define BH1750_CONTINUOUS_HIGH_RES  0x10    // 1 lux resolution, 120ms measurement time

static const char *TAG = "BH1750";

/**
 * Initialize I2C master
 */
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

/**
 * Write command to BH1750
 */
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

/**
 * Read light level from BH1750
 * Returns lux value
 */
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
        *lux = raw / 1.2;  // Convert to lux (formula from datasheet)
    }

    return ret;
}

/**
 * Initialize BH1750 sensor
 */
static esp_err_t bh1750_init(void)
{
    esp_err_t ret;

    // Power on
    ret = bh1750_write_command(BH1750_POWER_ON);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on BH1750");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Reset
    ret = bh1750_write_command(BH1750_RESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset BH1750");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set continuous high-res mode
    ret = bh1750_write_command(BH1750_CONTINUOUS_HIGH_RES);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set measurement mode");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(120));  // Wait for first measurement

    return ESP_OK;
}

/**
 * Scan I2C bus for devices
 */
static void i2c_scan(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  Found device at 0x%02X", addr);
        }
    }

    ESP_LOGI(TAG, "Scan complete");
}

/**
 * Get human-readable light level description
 */
static const char* get_light_description(float lux)
{
    if (lux < 1) return "Pitch Black     ";
    if (lux < 50) return "Very Dim        ";
    if (lux < 200) return "Dim Indoor      ";
    if (lux < 500) return "Normal Indoor   ";
    if (lux < 1000) return "Bright Indoor   ";
    if (lux < 10000) return "Overcast/Shade  ";
    if (lux < 32000) return "Full Daylight   ";
    return "Direct Sunlight ";
}

void app_main(void)
{
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "BH1750 Illuminance Sensor Test");
    ESP_LOGI(TAG, "Waveshare ESP32-C6-Zero");
    ESP_LOGI(TAG, "Framework: ESP-IDF");
    ESP_LOGI(TAG, "================================");

    // Initialize I2C
    ESP_LOGI(TAG, "Initializing I2C master...");
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C master initialized (SDA=GPIO%d, SCL=GPIO%d)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // Scan I2C bus
    i2c_scan();

    // Initialize BH1750
    ESP_LOGI(TAG, "Initializing BH1750...");
    ret = bh1750_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BH1750: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "Troubleshooting:");
        ESP_LOGE(TAG, "  1. Check VCC → 3.3V (NOT 5V!)");
        ESP_LOGE(TAG, "  2. Check GND → GND");
        ESP_LOGE(TAG, "  3. Check SCL → GPIO2");
        ESP_LOGE(TAG, "  4. Check SDA → GPIO1");
        ESP_LOGE(TAG, "  5. Verify 4.7kΩ pull-ups on SDA/SCL");
        return;
    }

    ESP_LOGI(TAG, "✓ BH1750 initialized successfully");
    ESP_LOGI(TAG, "  Address: 0x23");
    ESP_LOGI(TAG, "  Mode: CONTINUOUS_HIGH_RES (1 lux resolution)");
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "Starting measurements (every 2 seconds)...");
    ESP_LOGI(TAG, "");

    // Main loop
    while (1) {
        float lux;
        ret = bh1750_read_light(&lux);

        if (ret == ESP_OK) {
            // Create visual bar graph
            char bar[41] = {0};  // 40 chars + null
            int bar_length = (int)(lux / 50);
            if (bar_length > 40) bar_length = 40;
            for (int i = 0; i < bar_length; i++) {
                bar[i] = '█';
            }

            ESP_LOGI(TAG, "Light: %7.1f lux │ %s │ %s",
                     lux, get_light_description(lux), bar);
        } else {
            ESP_LOGE(TAG, "Failed to read from BH1750: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(2000));  // 2 second delay
    }
}
