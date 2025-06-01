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

## CAN TASK USAGE:

**TBD**

##
**CAUTION!** task responsible for message management (can task) shouldn't be busy with relatively long operations, any funtionality should be managed in different task (main task). Can task shall only listen for messages and send back answers.
