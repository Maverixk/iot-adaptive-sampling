#include "sampler.h"
#include "network.h"
#include <arduinoFFT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Buffers
uint16_t bufferA[SAMPLES];
uint16_t bufferB[SAMPLES];

// Pointers to quickly swap the active buffer
uint16_t* activeBuffer = bufferA;
uint16_t* processBuffer = bufferB;

// FFT arrays
double vReal[SAMPLES];
double vImag[SAMPLES];

// 'volatile' tells the compiler these change across different FreeRTOS tasks
volatile double currentSamplingFreq = 500.0;
volatile double latestAverage = 0.0;
volatile double latestMaxFreq = 0.0;

// Tracks the CPU time spent calculating FFTs in the current window
volatile uint32_t windowExecutionTimeUs = 0;

QueueHandle_t mqttWifiQueue;
QueueHandle_t loraQueue;

TaskHandle_t FFTTaskHandle = NULL;

void initSampler() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    pinMode(ADC_PIN, INPUT);
}

void highSpeedTestTask(void *pvParameters) {
    const int TEST_SAMPLES = 1000;
    uint16_t buffer[TEST_SAMPLES];
    uint32_t start, end;

    uint32_t sum_duration = 0;
    float sum_freq = 0.0f;

    Serial.printf("\nTesting maximum frequency on 5 iterations of 1000 samples each...\n\n");

    for (int i = 0; i < 5; i++) {
        start = micros();
        
        for (int j = 0; j < TEST_SAMPLES; j++)
            buffer[j] = analogRead(ADC_PIN); 
        
        end = micros();
        uint32_t duration = end - start;
        
        sum_duration += duration;
        sum_freq += (float)TEST_SAMPLES / (duration / 1000000.0);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second then go again
    }

    float avgDuration = (float)sum_duration / 5.0f;
    float avgFreq = sum_freq / 5.0f;
    float avgTimePerSample = avgDuration / TEST_SAMPLES;
  
    Serial.printf("Average elapsed time: %.2f us\n", avgDuration);
    Serial.printf("Average maximum frequency: %.2f Hz\n", avgFreq);
    Serial.printf("Average time for sample: %.2f us\n\n", avgTimePerSample);

    // Finally, task autodeletes itself
    vTaskDelete(NULL);
}

// Sampler task
void sampleSignalTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int index = 0;

    TickType_t lastWindowTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

    uint64_t windowSum = 0;
    uint32_t windowCount = 0;

    for (;;) {
        // Read shared frequency safely
        double currentFreq = currentSamplingFreq;
        TickType_t samplingDelay = pdMS_TO_TICKS(1000.0 / currentFreq);

        uint16_t val = analogRead(ADC_PIN);
        activeBuffer[index] = val;

        #if WIFI == 1 || LORA == 1
            windowSum += val;
            windowCount++;

            TickType_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            if ((currentTime - lastWindowTime) >= 30000) {
                float windowAvg = (float)windowSum / windowCount;

                // Read the volatile variable and reset for the next window
                uint32_t totalExecUs = windowExecutionTimeUs;
                windowExecutionTimeUs = 0;
                float totalExecMs = totalExecUs / 1000.0f;

                Serial.printf("CPU Execution Time (FFT): %.2f ms\n", totalExecMs);
                
                // Send the average to the queues for MQTT
                #if WIFI == 1
                    xQueueSend(mqttWifiQueue, &windowAvg, 0);
                #endif
                #if LORA == 1
                    xQueueSend(loraQueue, &windowAvg, 0);
                #endif
                
                // Reset and go again
                windowSum = 0;
                windowCount = 0;

                lastWindowTime = currentTime;
            }
        #endif
       
        Serial.print(val);             
        Serial.print("\t");                   
        Serial.print(latestAverage);                
        Serial.print("\t");                   
        Serial.println(latestMaxFreq);

        index++;

        // If the buffer is full, change it
        if (index >= SAMPLES) {
            // Swap the memory pointers instantly
            uint16_t* temp = activeBuffer;
            activeBuffer = processBuffer;
            processBuffer = temp;
            index = 0;

            // Wake up the FFT task to process the full buffer
            xTaskNotifyGive(FFTTaskHandle);
        }

        // vTaskDelayUntil perfectly absorbs the slight latency of Serial.print
        vTaskDelayUntil(&xLastWakeTime, samplingDelay);
    }
}

// FFT task
void processSignalTask(void *pvParameters) {
    for (;;) {
        // Sleep and yield CPU until the Sampler fills a buffer
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Tic tac tic tac
        uint32_t startFFT = micros();

        double sum = 0;
        // Copy the raw values into the FFT array
        for (int i = 0; i < SAMPLES; i++) {
            vReal[i] = processBuffer[i];
            vImag[i] = 0.0;
            sum += processBuffer[i];
        }

        // Update shared average for the plotter
        latestAverage = sum / SAMPLES;

        // Compute FFT
        ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, currentSamplingFreq);
        FFT.dcRemoval(); 
        FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(FFT_FORWARD);
        FFT.complexToMagnitude();

        // Find the True Max Frequency (Nyquist limit)
        double magnitudeSum = 0;
        for (int i = 1; i < SAMPLES / 2; i++) 
            magnitudeSum += vReal[i];
        
        double magnitudeAverage = magnitudeSum / (SAMPLES / 2);

        // We define the threshold as 5 times the magnitude of the signals
        double dynamicThreshold = magnitudeAverage * 5.0; 

        double maxFreq = 0.0;
        for (int i = (SAMPLES / 2) - 1; i > 0; i--) {
            if (vReal[i] > dynamicThreshold) {
                maxFreq = (i * currentSamplingFreq) / SAMPLES;
                break;
            }
        }

        // Update shared frequency for the plotter
        if (maxFreq >= 0.1) latestMaxFreq = maxFreq;

        #if ADAPTIVE == 1
            // Adaptive phase
            double optimizedFreq = maxFreq * 2.2;

            if (abs(currentSamplingFreq - optimizedFreq) > 2.0) {
                currentSamplingFreq = optimizedFreq;
            }
        #endif
        
        uint32_t endFFT = micros();
        windowExecutionTimeUs += (endFFT - startFFT);
    }
}