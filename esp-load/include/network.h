#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

// Physical pins of Heltec V3 for SX1262 module
#define LORA_NSS    8
#define LORA_DIO1   14
#define LORA_NRST   12
#define LORA_BUSY   13

void wifiTask(void *pvParameters);
void loraTask(void *pvParameters);

#endif