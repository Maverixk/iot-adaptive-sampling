#include "generator.h"
#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

uint16_t amplitudes[6] = {2, 4, 5, 8, 10, 12};
uint16_t frequencies[6] = {3, 5, 7, 11, 13, 17};

// Lookup Table (LUT) parameter
uint16_t waveLUT[SAMPLES_PER_CYCLE];

// Function meant for pre-computing sine values
void generateLookupTable() {

  int a1, a2, f1, f2;
  
  #if MODE == 0
    int idx_a1 = esp_random() % 6;
    int idx_f1 = esp_random() % 6;
    int idx_a2 = esp_random() % 6;
    int idx_f2 = esp_random() % 6;

    a1 = amplitudes[idx_a1];
    f1 = frequencies[idx_f1];
    a2 = amplitudes[idx_a2];
    f2 = frequencies[idx_f2];
  #elif MODE == 1
    a1 = 4;
    a2 = 5;
    f1 = 5;
    f2 = 7;
  #elif MODE == 2
    a1 = 12;
    a2 = 2;
    f1 = 3;
    f2 = 17;
  #elif MODE == 3
    a1 = 2;
    a2 = 10;
    f1 = 3;
    f2 = 13;
  #else 
    Serial.printf("Wrong signal generation mode! Defaulting to MODE 1\n");
    a1 = 4; a2 = 5; f1 = 5; f2 = 7;
  #endif

  Serial.printf("Signal 1 -> %d*(2*pi*%d*t)\n", a1, f1);
  Serial.printf("Signal 2 -> %d*(2*pi*%d*t)\n", a2, f2);

  for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
    // Compute fake 't' for a full cycle
    float t = (float)i / SAMPLES_PER_CYCLE; 
    
    // Assignment formula (subject to changes)
    // s(t) = 2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)
    float signal = a1 * sin(2 * PI * f1 * t) + a2 * sin(2 * PI * f2 * t);
    
    // Normalization (e.g. from -6 to +6) on 0-4095 as my DAC has a 12-bit structure
    // Note: you must adapt the min and max values based on the maximum amplitude of your formula
    float min_val = -(abs(a1)+abs(a2));
    float max_val = abs(a1)+abs(a2);
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