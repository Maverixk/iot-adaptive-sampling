#ifndef SAMPLER_H
#define SAMPLER_H

#include <Arduino.h>

// Constants
#define ADC_PIN 6   // Must be a channel of ADC1 as ADC2 might be later used by WiFi
#define SAMPLES 512 // Must be a power of 2 (256, 512, 1024, ...)

// Functions
void initSampler();
void sampleSignalTask(void *pvParameters);

#endif