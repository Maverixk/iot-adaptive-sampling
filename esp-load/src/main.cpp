#include <Arduino.h>
#include "generator.h"
#include "sampler.h"

// Task handles
TaskHandle_t DacTaskHandle = NULL;
TaskHandle_t SamplerTaskHandle = NULL;

void setup() {
  Serial.begin(115200);

  delay(3000);  // Otherwise Better Serial Plotter will cry
  
  // Hardware initialization
  initGenerator();
  initSampler();

  // Spawn the FreeRTOS Tasks
  // DAC on Core 1 (Physical output)
  xTaskCreatePinnedToCore(
    generateSignalTask, "DAC_Task", 2048, NULL, 3, &DacTaskHandle, 1
  );

  // Sampler on Core 0 or 1 depending on my preference
  xTaskCreatePinnedToCore(
    sampleSignalTask, "Sampler_Task", 4096, NULL, 2, &SamplerTaskHandle, 1
  );
}

void loop() {
  vTaskDelete(NULL); // Arduino default loop task deleted
}