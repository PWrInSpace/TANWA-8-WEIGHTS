idf_component_register(  SRC_DIRS "."
                         INCLUDE_DIRS "."
                         REQUIRES config can esp_timer cmock fatfs nvs_flash spi_flash spiffs esp_adc)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format" "-Wall" "-Werror")