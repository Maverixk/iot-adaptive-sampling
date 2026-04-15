#include "generator.h"
#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

// Lookup Table (LUT) parameter
uint16_t waveLUT[SAMPLES_PER_CYCLE];

// Function meant for pre-computing sine values
void generateLookupTable() {
  for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
    // Compute fake 't' for a full cycle
    float t = (float)i / SAMPLES_PER_CYCLE; 
    
    // Assignment formula (subject to changes)
    // s(t) = 2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)
    float signal = 2.0 * sin(2 * PI * 3 * t) + 4.0 * sin(2 * PI * 5 * t);
    
    // Normalization (e.g. from -6 to +6) on 0-4095 as my DAC has a 12-bit structure
    // Note: you must adapt the min and max values based on the maximum amplitude of your formula
    float min_val = -6.0;
    float max_val = 6.0;
    float normalized = ((signal - min_val) / (max_val - min_val)) * 4095.0;
    
    waveLUT[i] = (uint16_t)normalized;
  }
}

void initGenerator() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);
    dac.begin(0x60);
    generateLookupTable();
}

void generateSignalTask(void *pvParameters) {
    int index = 0;
    // To keep a constant frequency we use vTaskDelayUntil
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(2); // Update every 2ms (500Hz update rate)

    for (;;) {
        // Write the value on the DAC ("false" means: do NOT save in EEPROM, save time!)
        dac.setVoltage(waveLUT[index], false);
    
        index++;
        if (index >= SAMPLES_PER_CYCLE) index = 0;

        // Wait until next "tick" keeping the frequency strictly constant
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}