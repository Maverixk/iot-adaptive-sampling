#ifndef SAMPLER_H
#define SAMPLER_H

#include <Arduino.h>

#define ADC_PIN 6 
#define SAMPLES 512

// Expose the FreeRTOS queue so the sampler can put aggregates on it
extern QueueHandle_t mqttQueue;

// Expose the handle so sampler.cpp can use it
extern TaskHandle_t FFTTaskHandle;

void initSampler();
void highSpeedTestTask(void *pvParameters);
void sampleSignalTask(void *pvParameters);
void processSignalTask(void *pvParameters);

#endif