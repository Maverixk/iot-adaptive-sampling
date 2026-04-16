#include <Arduino.h>
#include "generator.h"
#include "sampler.h"
#include "network.h"

TaskHandle_t DacTaskHandle = NULL;
TaskHandle_t SamplerTaskHandle = NULL;

// #define FREQ_TEST

void setup() {
  Serial.begin(115200);
  delay(3000); // Our Better Serial Plotter hack!
  
  initGenerator();
  initSampler();

  mqttQueue = xQueueCreate(10, sizeof(float));

  // Generator on Core 1
  xTaskCreatePinnedToCore(
    generateSignalTask, "DAC_Task", 2048, NULL, 3, &DacTaskHandle, 1
  );

  #ifdef FREQ_TEST
    xTaskCreatePinnedToCore(
      highSpeedTestTask, "HighSpeedTest", 8192, NULL, 5, NULL, 1
    );
  #else
    // Runs standard sampler tasks
    xTaskCreatePinnedToCore(
      sampleSignalTask, "Sampler_Task", 4096, NULL, 4, NULL, 1
    );
    xTaskCreatePinnedToCore(
      processSignalTask, "FFT_Task", 8192, NULL, 2, &FFTTaskHandle, 0
    );
    xTaskCreatePinnedToCore(
      networkTask, "Network_Task", 4096, NULL, 1, NULL, 0
    );
  #endif
}

void loop() {
  vTaskDelete(NULL); 
}