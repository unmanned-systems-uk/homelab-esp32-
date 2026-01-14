#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

// ========================================
// BH1750 I2C Configuration
// ========================================
#define I2C_MASTER_SCL_IO           2       // GPIO2
#define I2C_MASTER_SDA_IO           1       // GPIO1
#define I2C_MASTER_NUM              0       // I2C port 0
#define I2C_MASTER_FREQ_HZ          100000  // 100kHz
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

// BH1750 Device Address and Commands
#define BH1750_ADDR                 0x23
#define BH1750_POWER_ON             0x01
#define BH1750_RESET                0x07
#define BH1750_CONTINUOUS_HIGH_RES  0x10

// ========================================
// DS18B20 1-Wire Configuration
// ========================================
#define DS18B20_GPIO                5       // GPIO5

// DS18B20 Commands
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_SKIP_ROM        0xCC

static const char *TAG_BH1750 = "BH1750";
static const char *TAG_DS18B20 = "DS18B20";

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
        ESP_LOGE(TAG_BH1750, "Failed to power on");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = bh1750_write_command(BH1750_RESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "Failed to reset");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = bh1750_write_command(BH1750_CONTINUOUS_HIGH_RES);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "Failed to set measurement mode");
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
// Helper Functions
// ========================================

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

// ========================================
// Main Application
// ========================================

void app_main(void)
{
    ESP_LOGI("MAIN", "========================================");
    ESP_LOGI("MAIN", "Multi-Sensor Test: BH1750 + DS18B20");
    ESP_LOGI("MAIN", "Waveshare ESP32-C6-Zero");
    ESP_LOGI("MAIN", "========================================");

    // Initialize I2C for BH1750
    ESP_LOGI(TAG_BH1750, "Initializing I2C (SDA=GPIO%d, SCL=GPIO%d)...",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "I2C init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG_BH1750, "I2C initialized successfully");
    }

    // Initialize BH1750
    bool bh1750_ok = false;
    ret = bh1750_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_BH1750, "✓ BH1750 ready (Address: 0x23)");
        bh1750_ok = true;
    } else {
        ESP_LOGE(TAG_BH1750, "✗ BH1750 init failed: %s", esp_err_to_name(ret));
    }

    // Initialize DS18B20
    ESP_LOGI(TAG_DS18B20, "Initializing 1-Wire (GPIO%d)...", DS18B20_GPIO);
    bool ds18b20_ok = false;
    ret = ds18b20_init(DS18B20_GPIO);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_DS18B20, "✓ DS18B20 detected on 1-Wire bus");
        ds18b20_ok = true;
    } else {
        ESP_LOGE(TAG_DS18B20, "✗ DS18B20 not found");
        ESP_LOGE(TAG_DS18B20, "Check wiring: VCC→3.3V, GND→GND, DATA→GPIO5");
        ESP_LOGE(TAG_DS18B20, "Verify 4.7kΩ pull-up resistor on DATA line");
    }

    ESP_LOGI("MAIN", "========================================");
    ESP_LOGI("MAIN", "Starting measurements (every 2 seconds)...");
    ESP_LOGI("MAIN", "");

    // Main loop
    while (1) {
        // Start DS18B20 conversion first (takes 750ms)
        if (ds18b20_ok) {
            ds18b20_start_conversion(DS18B20_GPIO);
        }

        // Read BH1750 (fast)
        if (bh1750_ok) {
            float lux;
            ret = bh1750_read_light(&lux);
            if (ret == ESP_OK) {
                // Create visual bar
                char bar[21] = {0};
                int bar_length = (int)(lux / 50);
                if (bar_length > 20) bar_length = 20;
                for (int i = 0; i < bar_length; i++) {
                    bar[i] = '#';
                }

                ESP_LOGI(TAG_BH1750, "Light: %7.1f lux | %s | %-20s",
                         lux, get_light_description(lux), bar);
            }
        }

        // Wait for DS18B20 conversion to complete
        if (ds18b20_ok) {
            vTaskDelay(pdMS_TO_TICKS(750));

            float temperature;
            ret = ds18b20_read_temperature(DS18B20_GPIO, &temperature);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG_DS18B20, "Temp:  %6.2f °C  (%.2f °F)",
                         temperature, (temperature * 9.0/5.0) + 32.0);
            } else {
                ESP_LOGE(TAG_DS18B20, "Failed to read temperature");
            }
        }

        ESP_LOGI("MAIN", "---");
        vTaskDelay(pdMS_TO_TICKS(1250));  // Total ~2 seconds per cycle
    }
}
