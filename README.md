# iot-adaptive-sampling

## Abstract
The following repository contains all the material developed for the individual assignment of the Internet-of-Things Algorithms and Services course, held by Sapienza University of Rome during A.Y. 2025/2026. 
The assignment consists in developing an IoT system capable of generating a continous signal, establishing the correct sampling frequency and sending the aggregate results both on a edge server via WiFi and on a cloud server via LoRaWAN.

## Material used
- 2x ESP32 V3 (from here on called `LOAD` and `MONITOR`)
- 1x INA219
- 1x MCP4725 DAC
- 1x 3.3V LiPo battery
- 1x 400-pin breadboard
- DuPont wires

## Maximum frequency
In order to identify the maximum sampling frequency of my device (`LOAD` ESP32), I chose to implement a function ```void highSpeedTestTask(void *pvParameters)``` whose goal is to perform analog reads (via ADC) on 1000 samples for 5 consecutive times without any kind of delay. At the end of this process, I averaged the results which are:
- **Average elapsed time** = 60.253 ms
- **Average maximum frequency** = 16.6 kHz
- **Average time for sample** = 60.25 us

These results were crucial to better picture my hardware limitations, while they also allowed me to understand how far I could go with the oversampling frequency in the `sampler.cpp` code.