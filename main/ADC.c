// =============================
// 📦 Header Files (Your Toolbox)
// =============================
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

// =============================
// 🏷️ Application Log Tag
// =============================
#define TAG "ADC_TASK"

// =============================
// ⚙️ ADC Configuration
// =============================
#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_6   // GPIO34 (Check your board pinout)

// =============================
// 📋 Function Prototypes
// =============================
static adc_oneshot_unit_handle_t init_adc(void);
static void adc_task(void *param);

// =============================
// ⚙️ ADC Initialization Function
// =============================
static adc_oneshot_unit_handle_t init_adc(void)
{
    ESP_LOGI(TAG, "Initializing ADC...");

    // --- Step 1: Create ADC handle ---
    adc_oneshot_unit_handle_t adc_handle;

    // --- Step 2: Configure ADC unit ---
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    // 🧩 ESP-IDF API Call: Initialize ADC unit
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit! Error code: %d", ret);
        return NULL;
    }

    // --- Step 3: Configure ADC channel ---
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11
    };

    // 🧩 ESP-IDF API Call: Configure channel
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel! Error code: %d", ret);
        return NULL;
    }

    ESP_LOGI(TAG, "ADC initialization complete.");
    return adc_handle;
}

// =============================
// 🧠 ADC Reading Task
// =============================
static void adc_task(void *param)
{
    adc_oneshot_unit_handle_t adc_handle = (adc_oneshot_unit_handle_t)param;
    int raw_value = 0;

    while (1) {
        // 🧩 ESP-IDF API Call: Take one ADC sample
        esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ADC Reading: %d", raw_value);
        } else {
            ESP_LOGE(TAG, "ADC read failed! Error code: %d", ret);
        }

        // Wait 100 ms before next read
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================
// 🚀 Main Application Entry Point
// =============================
void app_main(void)
{
    // Initialize ADC and get handle
    adc_oneshot_unit_handle_t adc_handle = init_adc();
    if (adc_handle == NULL) {
        ESP_LOGE(TAG, "ADC initialization failed. Stopping.");
        return;
    }

    // 🧩 ESP-IDF API Call: Read one sample (just once)
    int raw_value = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Single ADC sample: %d", raw_value);
    } else {
        ESP_LOGE(TAG, "Failed to read ADC sample! Error code: %d", ret);
    }

    // 🧩 ESP-IDF API Call: Create a FreeRTOS task
    xTaskCreate(adc_task, "adc_task", 2048, adc_handle, 5, NULL);
}
