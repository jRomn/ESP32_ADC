// =============================
// Header Files (Your Toolbox)
// =============================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"    // For ADC HW interation
#include "esp_adc/adc_cali.h"       // For voltage calibration


// =============================
// Application Log Tag
// =============================
#define TAG "ADC_BASIC"


// =============================
// ADC Configuration
// =============================
// Define which ADC unit and channel you want to use.
// For example: ADC1 Channel 6 (Take a look to the ESP32-DevKit Pin Layout)
#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_6   // GPIO34
#define BUFFER_SIZE    256             // Circular buffer length
#define ADC_SAMPLE_PERIOD_MS 100       // Sampling period (ms)


// =============================
// Global Handles
// =============================
// Shared between tasks
static adc_oneshot_unit_handle_t adc_handle;  // ADC driver handle
static adc_cali_handle_t adc_cali_handle;     // Calibration handle


// =============================
// Circular Buffer Declaration
// =============================
// Stores the latest ADC samples
static int16_t adc_buffer[BUFFER_SIZE];     // vector of size 256
static volatile size_t buffer_index = 0;    // index pointing to  where the next sample will be written


// =============================
// ADC Unit Initialization + Channel Configuration + Calibration
// =============================
// Function to initialize the ADC unit, configure the channel, and set up calibration.
// Returns a handle to the initialized ADC unit.
adc_oneshot_unit_handle_t init_adc(void)
{

    esp_err_t ret;

    // ==============================
    // 1️⃣ ADC Unit Configuration
    // ==============================

    // STEP 1A : Create a Handle (Done - Alrady defined globally)
    // A handle is like a "pointer" or reference to a software object that represents
    // the ADC hardware inside ESP-IDF. This handle will be used for all future ADC calls.
    // (At this point, adc_handle is just a NULL pointer.)
    // static adc_oneshot_unit_handle_t adc_handle;    // Reference to ADC driver

    // STEP 1B : Define the ADC Unit configuration structure
    // This structure describes global settings for the ADC peripheral.
    // - unit_id: Which ADC hardware block (ADC1 or ADC2)
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,   // Use ADC1 block
    };
    // Nothing is initialized yet. This is just the **desired configuration**.

    // STEP 1C : Initialize ADC unit using the ESP-IDF API
    // Function: adc_oneshot_new_unit(init_config, &adc_handle)
    // What it does:
    // 1. Allocates memory for the ADC driver object
    // 2. Programs ADC hardware registers according to init_config
    // 3. Updates adc_handle to point to this driver object
    ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC Unit initialized successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to initialize ADC unit! Error code: %d", ret);
        return NULL; // Stop if initialization failed
    }
    // Now adc_handle points to a fully initialized ADC driver object
    // but the ADC channel/pin and input scaling are not set yet.

    // ==============================
    // 2️⃣ ADC Channel Configuration
    // ==============================

    // STEP 2B : Define the Channel Configuration structure
    // This is a separate configuration structure that describes how to read from a specific ADC channel (pin).
    // - bitwidth: Resolution of conversion (default 12-bit)
    // - attenuation: How much input voltage the ADC can measure (~3.3V for DB_11)
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // Default 12-bit resolution
        .atten = ADC_ATTEN_DB_11           // ~3.3V full-scale voltage range
    };

    // STEP 2C: [ BLANK]
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC channel configured successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to configure ADC channel! Error code: %d", ret);
        return NULL;
    }

    // ==============================
    // 3️⃣ ADC Calibration Initialization
    // ==============================

    // Calibration is optional but recommended for accurate voltage readings.
    // On ESP32, raw ADC values may vary due to temperature, voltage supply, and manufacturing.
    // The calibration API converts raw readings to mV.

    // Step 3A: Define the Calibration configuration structure
    // This structure describes how the calibration should be performed.
    // - unit_id: Which ADC unit (must match the one used before)
    // - atten: Must match the attenuation used in channel config
    // - bitwidth: Must match the bitwidth used in channel config
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,               // Same ADC unit as before
        .atten = ADC_ATTEN_DB_11,          // Same attenuation as channel config
        .bitwidth = ADC_BITWIDTH_DEFAULT   // Same bitwidth as channel config
    };

    // Step 3B: Initialize Calibration using ESP-IDF API
    // Function: adc_cali_create_scheme_curve_fitting(&cali_cfg, &handle)
    // What it does:
    // - Allocates memory for calibration object
    // - Prepares math to convert raw ADC → voltage (mV)
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle) == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration ready.");
    } else {
        ESP_LOGW(TAG, "ADC calibration not available. Using raw ADC values.");
        adc_cali_handle = NULL;          // Use raw values if calibration fails
    }

    // --- End of setup ---
    ESP_LOGI(TAG, "ADC is now initialized and ready for sampling.");

    // Return the ADC driver handle in case the caller wants it
    return adc_handle;

}

// =============================
// FreeRTOS Task: ADC Sampling
// =============================
void adc_sampling(void *arg)
{
    while (1) {

        int raw = 0;
        uint16_t voltage = 0; // Calibrated voltage in mV

        // --- 1. Read raw ADC value ---
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw); // ESP-IDF API

        // --- 2. Convert raw to calibrated voltage (mV) ---
        if (adc_cali_handle) {
            adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage); // ESP-IDF API
        } else {
            // Fallback if calibration unavailable
            voltage = raw;
        }

        // --- 3. Store calibrated voltage in circular buffer ---
        adc_buffer[buffer_index] = voltage;
        buffer_index = (buffer_index + 1) % BUFFER_SIZE; // Wrap around

        // --- 4. Optional: Print to serial ---
        ESP_LOGI(TAG, "ADC Voltage: %d mV", voltage);

        // --- 5. Delay for next sample (100 ms) ---
        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_PERIOD_MS));
    }
}

// =============================
// FreeRTOS Task: Filtering
// =============================
void adc_filtering(void *arg)
{
    const size_t filter_size = 5;  // Moving average window
    int sum = 0;
    int filtered_value = 0;

    while (1) {
        sum = 0;

        // Compute moving average of last N samples
        for (size_t i = 0; i < filter_size; i++) {
            size_t index = (buffer_index + BUFFER_SIZE - 1 - i) % BUFFER_SIZE;
            sum += adc_buffer[index];
        }
        filtered_value = sum / filter_size;

        // Display filtered value
        ESP_LOGI(TAG, "Filtered ADC Voltage: %d mV", filtered_value);

        // Control filtering frequency (matches sampling)
        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_PERIOD_MS));
    }
}


// =============================
// Main Application Entry Point
// =============================
void app_main(void)
{   
    // --- Start logging ---
    // ESP-IDF functon to print to serial console
    ESP_LOGI(TAG, "Starting ADC Initialization and Calibration...");

    // --- Initialize ADC ---
    // ESP-IDF function to setup ADC Unit, Channel, and Calibration
    // In specific ADC Unit 1 - Channel 6 (GPIO34)
    adc_handle = init_adc();
    if (!adc_handle) {
        ESP_LOGE(TAG, "ADC initialization failed. Exiting.");
        return;
    }

    
    BaseType_t task_status;

    // --- Task for ADC Sampling ---
    task_status = xTaskCreate(adc_sampling, "ADC Sampling", 2048, NULL, 5, NULL);
    if (task_status != pdPASS)
        ESP_LOGE(TAG, "Failed to create ADC sampling task!");
    // --- Task for ADC Filtering ---
    task_status = xTaskCreate(adc_filtering, "ADC Filtering", 2048, NULL, 4, NULL);
    if (task_status == pdPASS) {
        ESP_LOGI(TAG, "ADC task created successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to create ADC task!");
    }

}


