ESP32 ADC + Signal Processing + BLE

Description:
This repository contains example firmware for the ESP32 that demonstrates analog signal acquisition, calibration, filtering, and wireless streaming. It focuses on using the ESP-IDF ADC oneshot driver, FreeRTOS tasks, and BLE GATT services for real-time signal monitoring.

Key Features:

ADC Sampling

Reads analog signals from ESP32 ADC channels using the modern adc_oneshot driver.

Supports calibrated voltage readings (millivolts) via ESP-IDF calibration APIs.

Stores samples in a circular buffer for smooth data management.

Signal Filtering

Implements a simple moving average low-pass filter as an example of real-time signal processing.

Prepares the data for further analysis or transmission.

Demonstrates how to create additional FreeRTOS tasks for non-blocking filtering.

BLE Streaming (Future Step)

Plans to send filtered signals over BLE using a custom GATT characteristic.

Allows real-time monitoring from mobile or desktop apps.

Technical Details:

Developed with ESP-IDF (Espressif IoT Development Framework).

Uses FreeRTOS tasks for concurrent sampling and processing.

Demonstrates best practices for ADC initialization, channel configuration, and calibration.

Designed for educational purposes and as a starting point for EEG, sensor, or IoT projects.
