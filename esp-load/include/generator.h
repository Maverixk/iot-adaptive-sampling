#ifndef GENERATOR_H
#define GENERATOR_H

#include <Arduino.h>

// Constants
#define SDA_PIN 4
#define SCL_PIN 5 
#define SAMPLES_PER_CYCLE 500 

// Functions
void initGenerator();
void generateSignalTask(void *pvParameters);

#endif