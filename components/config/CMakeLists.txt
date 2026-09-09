idf_component_register(  SRC_DIRS "."
                         INCLUDE_DIRS "."
                         REQUIRES driver mcu_config hardware can cli app nvs)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format" "-Wall" "-Werror")

