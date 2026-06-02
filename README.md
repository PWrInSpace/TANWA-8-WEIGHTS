# TANWA SLAVE TEMPLATE

TANWA - filling, weighting, pressurizing and igniting system slave node template:

## POINTS TO CHANGE:

* console config - edit commands in ***console_config.c*** file
* TWAI (CAN) bus filter - edit **TWAI_ACCEPTANCE_FILTER** and **TWAI_ACCEPTANCE_MASK** in ***Kconfig.projbuild*** file 
* add hardware libraries, and hardware initialization in ***board_config.c*** and ***board_config.h*** files
* add CAN message parsing in ***can_commands.h***, ***can_config.c*** and ***can_config.h***
* add main loop in ***app_task.c*** (e.g. measurements)
* likely, add macros for twai struct creation
* add configurations in existing files or add files specific for functionality (e.g. built-in adc configuration, spi configuration, etc...)

## USAGE CHECKLIST:
1. Make sure SPS is set to 1000 (by default) in file ads1256.c for DEV1 (variable ads1256_config_dev1)
2. Make sure that in file ads1256.c variable ads1256_channels_dev1[1] is set to: {0x9D, 0xF6, 0xFF} {0x79, 0xBA, 0x49}; (calibration for dynamometer)
3. Make sure that in file ads1256_task.c variables buffer_readc_A and buffer_readc_B are set to BUFFER_READC_SAMPLES = 5000
4. Make sure that DYNAMOMETER is calibrated on tanwa's power supply and has measured zero_offset and multiplier (ask Bartek if you don't know)
5. Make sure that the SD card is installed!!
(Polish version in main.c file)


## TECHNOLOGIES AND TOOLS USED:
The project was developed in C as embedded systems firmware dedicated to the ESP32 microcontroller family (e.g., ESP32-S3). The architecture emphasizes Hardware Abstraction Layer (HAL) isolation and advanced pre-flight signal bus diagnostics.

**Core technologies:**
* Programming Language: C (C99 / C11 standards) – focused on high performance and low-level memory management.
* Framework / SDK: ESP-IDF (Espressif IoT Development Framework) – the official, native runtime environment, providing maximum control over the microcontroller with zero system overhead.
* FreeRTOS (Real-Time Operating System): The real-time multitasking kernel embedded within ESP-IDF. It ensures deterministic execution, task scheduling, prioritization of critical I/O operations (telemetry logging), and safe thread blocking using underlying RTOS delays (usleep).

**Peripherals & Storage:**
* SDMMC Bus (4-bit Mode): Utilizes the native hardware controller (driver/sdmmc_host.h, sdmmc_cmd.h) for high-speed, parallel logging of dense telemetry data streams.
* Virtual File System (VFS FAT): Integrates the SD card with a FAT file system (esp_vfs_fat.h), enabling standard C library file operations on .txt files.
* ADC Oneshot Driver (esp_adc/adc_oneshot.h): Leverages the Analog-to-Digital Converter for precise voltage measurements on data lines for diagnostic purposes.
* Software ADC Calibration: Implements Curve Fitting and Line Fitting calibration schemes using factory-burned calibration data stored in the chip's eFuse memory.
* GPIO Driver: Advanced I/O line control, including open-drain mode configuration (GPIO_MODE_INPUT_OUTPUT_OD) and dynamic management of internal pull-up resistors.

**Low-Level Diagnostics & Signal Integrity:**
* CPU Cycle Profiling (esp_cpu.h): Uses processor registers and esp_cpu_get_cycle_count() to profile electrical parameters with single-clock-cycle precision.
* Signal Integrity Testing: Features custom routines to verify signal rise times (PIN recovery time) and detect short circuits or parasitic crosstalk (cross-talk) between PCB traces.
* Hardware Abstraction Layer (HAL): Implements a function-pointer-based driver architecture (e.g., for the LED driver), allowing easy hardware swapping and seamless unit testing/mocking.

**Standards & Auxiliary Mechanisms:**
* Standard C Library & POSIX: Relies on standard, safe I/O and time operations (stdio.h, sys/stat.h, unistd.h).
* Logging Component (ESP_LOG): Provides structured system event logging categorized by tags and verbosity levels (Error, Warning, Info, Debug) to facilitate post-flight analysis.

## USAGE AND CHARACTERISTICS:
* Saving data onto an SD card
* Communicating with other PCBs through CAN and USB
* Filling, weighting, pressurizing and igniting the system
* Component initialization and diagnostics


## CAN TASK USAGE:

**TBD**

##
**CAUTION!** task responsible for message management (can task) shouldn't be busy with relatively long operations, any funtionality should be managed in different task (main task). Can task shall only listen for messages and send back answers.
