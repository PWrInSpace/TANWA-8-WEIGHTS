# TANWA SLAVE TEMPLATE

TANWA WEIGHTS slave node template:

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
This project is a firmware written in C for ESP32 microcontrollers (like the ESP32-S3). It is designed to be easily adaptable to different hardware and includes advanced tests to check system health before operation.

**Core technologies:**

*C Programming Language:* Used for high performance and direct control over the system's memory.

*ESP-IDF Framework:* The official development tool for ESP32 chips, allowing full control over the hardware without slowing it down.

*FreeRTOS:* A real-time operating system that manages multiple tasks at once, ensuring important jobs (like saving flight data) always run smoothly and on time.

**Peripherals & Storage:**

*High-Speed SD Card Connection:* Uses the chip's built-in hardware to quickly save large amounts of system data.

*File System (FAT):* Allows the system to read and write standard .txt files directly to the SD card.

*Voltage Measurement (ADC):* Accurately reads electrical signals on the data lines for system testing.

*Sensor Calibration:* Uses factory settings saved inside the chip to make sure all voltage readings are highly accurate.

*Advanced Pin Control (GPIO):* Manages the physical connection pins on the chip, controlling how electrical signals and internal resistors behave.

**Diagnostics & System Health:**

*Precise Timing:* Uses the processor's internal clock to measure electrical signals with extreme, single-cycle accuracy.

*Circuit Health Checks:* Custom tests that check for short circuits, signal interference, and overall wire health on the circuit board.

*Flexible Hardware Design (HAL):* Keeps the main code separate from specific hardware, making it easy to swap out physical parts (like LEDs) and test the system.

**Standards & Extra Features:**

*Standard C Libraries:* Uses standard, reliable methods for handling time and data.

*System Logging:* Records system events and categorizes them (Errors, Warnings, Info, Debug) to make it easier to analyze performance and fix issues later.

## USAGE AND CHARACTERISTICS:
* Saving data onto an SD card
* Communicating with TANWA-COM through CAN and USB
* Weighting the system
* Component initialization and diagnostics


## CAN TASK USAGE:

**TBD**

##
**CAUTION!** task responsible for message management (can task) shouldn't be busy with relatively long operations, any funtionality should be managed in different task (main task). Can task shall only listen for messages and send back answers.
