#ifndef GENERATOR_H
#define GENERATOR_H

#include <Arduino.h>

// Constants
#define SDA_PIN 4
#define SCL_PIN 5 
#define SAMPLES_PER_CYCLE 500 

// 0 = random, 1 = signal1, 2 = signal2, 3 = signal3
#define MODE 3

// Functions
void initGenerator();
void generateSignalTask(void *pvParameters);

#endif