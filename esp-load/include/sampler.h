#ifndef SAMPLER_H
#define SAMPLER_H

#include <Arduino.h>

#define ADC_PIN 6
#define ADAPTIVE 1            // 0 = oversampling, 1 = adaptive sampling
#define SAMPLES 512

#define INJECT_NOISE 0        // 0 = clean signal, 1 = add Gaussian noise + spikes
#define FILTER_TYPE 0       // 0 = none, 1 = Z-Score, 2 = Hampel
#define FILTER_WINDOW_SIZE 20 // Window size for the sliding filters

// Expose the FreeRTOS queue so the sampler can put aggregates on it
extern QueueHandle_t mqttWifiQueue;
extern QueueHandle_t loraQueue;

// Expose the handle so sampler.cpp can use it
extern TaskHandle_t FFTTaskHandle;

void initSampler();
void highSpeedTestTask(void *pvParameters);
void sampleSignalTask(void *pvParameters);
void processSignalTask(void *pvParameters);

#endif