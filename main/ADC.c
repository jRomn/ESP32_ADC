// =============================
// 🧰 Header Files (Your Toolbox)
// =============================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

// =============================
// 📌 Application Log Tag
// =============================
#define TAG "ADC_BASIC"

// =============================
// ⚙️ ADC Configuration
// =============================
// Define which ADC unit and channel you want to use.
// For example: ADC1 Channel 6 (Take a look to the ESP32-DevKit Pin Layout)
#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_6   // GPIO34

// =============================
// 🚀 Main Application Entry Point
// =============================
// app_main() runs as the first FreeRTOS task after boot.
void app_main(void)
{
    // --- 0. Start logging ---
    ESP_LOGI(TAG, "Starting ADC basic example...");

    // --- 1. Create a handle (like a pointer) to later link to a real ADC driver object ---
    adc_oneshot_unit_handle_t adc_handle;

    // --- 2. Define the initialization configuration for the ADC driver object ---
    // This describes how the ADC should be set up: which unit, resolution, clock, attenuation, etc.
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,   // Which ADC hardware block (ADC1 or ADC2)
    };
    // Nothing has been initialized yet — this is just the desired setup description.

    // --- 3. Create and initialize the ADC driver using the previous configuration ---
    // * Allocates the driver's internal structure (in heap)
    // * Initializes it according to your config
    // * Returns a handle (pointer) to that internal driver object
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC unit initialized successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to initialize ADC unit! Error code: %d", ret);
        return; // Stop if initialization failed
    }


    // --- 5. Configure a single ADC channel ---
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // Default 12-bit resolution
        .atten = ADC_ATTEN_DB_11           // ~3.3V full-scale voltage range
    };

    // Apply the configuration to ADC1 Channel 6 (GPIO34)
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC channel configured successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to configure ADC channel! Error code: %d", ret);
        return;
    }

    // --- 6. End of setup (no reading yet) ---
    ESP_LOGI(TAG, "ADC is now initialized and ready for sampling.");
}

