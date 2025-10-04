idf_component_register(
    SRCS "ADC.c" 
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver freertos esp_adc 
)
