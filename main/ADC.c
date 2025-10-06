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
// Circular Buffer Declaration
// =============================
// Stores the latest ADC samples
static int16_t adc_buffer[BUFFER_SIZE];     // vector of size 256
static volatile size_t buffer_index = 0;    // index pointing to  where the next sample will be written

// =============================
// ADC + Calibration Handles
// =============================
static adc_oneshot_unit_handle_t adc_handle = NULL;   // Reference to ADC driver
static adc_cali_handle_t adc_cali_handle = NULL;      // Reference to ADC calibration object

// =============================
// Function: Initialize ADC + Calibration
// =============================
adc_oneshot_unit_handle_t init_adc(void)
{
    // --- 0. Start logging ---
    ESP_LOGI(TAG, "Starting ADC basic example...");


    // --- 1. ADC Unit Configuration ---

    // STEP 1 : Create a Handle 
    // — a reference (pointer) that will later point to the definition of a ADC Hardware Block (structure).
    // adc_oneshot_unit_handle_t adc_handle;

    // STEP 2 : Define the ADC Hardware Block (structure) configuration
    // — global settings that apply to the entire ADC peripheral: which unit, resolution, clock, attenuation, etc.
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,   // Which ADC hardware block (ADC1 or ADC2)
    };
    // Nothing has been initialized yet — this is just the desired setup description.

    // STEP 3 : Initializa and Create the ADC Unit structure within the ESP-IDF (make it active):
    // - Allocates memory for the ADC driver object (internally, on the heap)
    // - Configures the ADC hardware registers based on your init_config.
    // - Updates your adc_handle pointer to point to that new driver object.
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC unit initialized successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to initialize ADC unit! Error code: %d", ret);
        return; // Stop if initialization failed
    }
    // Now the "adc_handle" a real, fully initialized ADC driver managed by the ESP-IDF.
    // but it doesn’t know which physical pin you want to read from yet, 
    // or how to interpret signals coming into that pin.

    // --- 2. ADC Channel Configuration ---

    // STEP 4 : Define a Channel Configuration 
    // - This is a separate configuration structure that describes how to read from a specific ADC channel (pin).
    // - How many bits to convert(bitwidth)
    // - How much input voltage range you expect (attenuation)
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // Default 12-bit resolution
        .atten = ADC_ATTEN_DB_11           // ~3.3V full-scale voltage range
    };

    // STEP 5: Apply the ""Channel Configuration" to an specific ADC Channel
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC channel configured successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to configure ADC channel! Error code: %d", ret);
        return;
    }

    // ==============================
    // 3️⃣ ADC Calibration Initialization
    // ==============================

    // Calibration is optional but recommended for accurate voltage readings.
    // On ESP32, raw ADC values may vary due to temperature, voltage supply, and manufacturing.
    // The calibration API converts raw readings to mV.

    adc_cali_handle_t handle = NULL; // Temporary handle for calibration object

    // Step 3a: Define calibration configuration
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,               // Same ADC unit as before
        .atten = ADC_ATTEN_DB_11,          // Same attenuation as channel config
        .bitwidth = ADC_BITWIDTH_DEFAULT   // Same bitwidth as channel config
    };

    // Step 3b: Create calibration object using ESP-IDF API
    // Function: adc_cali_create_scheme_curve_fitting(&cali_cfg, &handle)
    // What it does:
    // - Allocates memory for calibration object
    // - Prepares math to convert raw ADC → voltage (mV)
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &handle) == ESP_OK) {
        adc_cali_handle = handle;        // Save the calibration handle globally
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
void adc_task(void *arg)
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
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================
// Main Application Entry Point
// =============================
void app_main(void)
{
    ESP_LOGI(TAG, "Starting ADC periodic sampling example...");

    // --- Initialize ADC ---
    adc_oneshot_unit_handle_t adc_handle = init_adc();
    if (!adc_handle) {
        ESP_LOGE(TAG, "ADC initialization failed. Exiting.");
        return;
    }

    // --- Create ADC Task ---
    // xTaskCreate(TaskFunction, Name, StackDepth, Parameters, Priority, TaskHandle)
    BaseType_t task_status = xTaskCreate(
        adc_task,          // Task function (runs in its own context)
        "ADC Task",        // Task name (for debugging)
        2048,              // Stack size in words (not bytes!)
        (void *)adc_handle,// Parameter passed to the task (our ADC handle)
        5,                 // Task priority (higher = more important)
        NULL               // Optional task handle (can be NULL if not needed)
    );

    if (task_status == pdPASS) {
        ESP_LOGI(TAG, "ADC task created successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to create ADC task!");
    }
}

