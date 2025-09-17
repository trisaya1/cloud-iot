# IoT project

## Overview

In this project, I integrated an embedded system with cloud infrastructure using the MQTT protocol. I set up a virtual server on AWS, configured an MQTT broker, stored sensor data in a PostgreSQL database, and displayed data on a simple webpage using Flask. This project taught me how complete IoT workflows are implemented: from data transmission to cloud storage and visualisation


The **Blinky** project is a simple example that can be used to verify the
basic tool setup.

It is compliant to the Cortex Microcontroller Software Interface Standard (CMSIS)
and uses the CMSIS-RTOS2 API interface for RTOS functionality. The CMSIS-RTOS2 API
is available with various real-time operating systems, for example RTX5 or FreeRTOS.

## Operation

- At start
  - outputs "Blinky Example" to UART that is connected to ST-Link (baudrate 115200bps)
  - blinks vioLED0 in 1 sec interval.
- The vioBUTTON0 changes the blink frequency and start/stops vioLED1.

### CMSIS-Driver Virtual I/O mapping

| CMSIS-Driver VIO      | Board component
|:----------------------|:--------------------------------------
| vioBUTTON0            | USER button (B2)
| vioLED0               | LED green   (LD2)
