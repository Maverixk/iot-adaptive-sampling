#include "sampler.h"
#include <arduinoFFT.h>

// FFT arrays
double vReal[SAMPLES];
double vImag[SAMPLES];
uint16_t rawWave[SAMPLES];

// Initial frequency (Oversampling)
double currentSamplingFreq = 200.0; 

void initSampler() {
    // 12-bit resolution to match the DAC (values in range 0-4095)
    analogReadResolution(12);
    pinMode(ADC_PIN, INPUT);
}

void sampleSignalTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        double sum = 0;
        // How many ticks (from ms) should I wait between each sample
        TickType_t samplingDelay = pdMS_TO_TICKS(1000.0 / currentSamplingFreq);
        
        // Sampling phase
        for (int i = 0; i < SAMPLES; i++) {
            uint16_t val = analogRead(ADC_PIN);

            vReal[i] = val;
            rawWave[i] = val;   // Save the value for the plotter
            vImag[i] = 0.0;
            
            sum += vReal[i];    // We sum for the aggregation phase
            
            vTaskDelayUntil(&xLastWakeTime, samplingDelay);
        }

        // Aggregation phase (average)
        // Note: we compute the average NOW, as the FFT will overwrite the vReal array!
        double average = sum / SAMPLES;

        // FFT computation
        // Instantiating the FFT object here so it will always use the updated currentSamplingFreq
        ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, currentSamplingFreq);
        
        // Removing the DC component (average offset) to avoid that 0 Hz peak hides our signals
        FFT.dcRemoval(); 
        FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(FFT_FORWARD);
        FFT.complexToMagnitude();

        double dominantFreq = FFT.majorPeak();

        // Plotting phase
        for (int i = 0; i < SAMPLES; i++) {
            // We print the intact rawWave, NOT the destroyed vReal!
            Serial.print(rawWave[i]);             
            Serial.print("\t");                   
            Serial.print(average);                
            Serial.print("\t");                   
            Serial.println(dominantFreq);         
        }

        // Adapting phase
        // Shannon's Theorem: the sampling frequency must be at least twice the maximum frequency.
        // (We go slightly higher than 2x just to prevent any kind of noise/jitter)
        double optimizedFreq = dominantFreq * 2.2;

        // We add 2 safety margins to avoid disasters or wrong reads
        if (optimizedFreq < 15.0) optimizedFreq = 15.0; // Min limit
        if (optimizedFreq > 200.0) optimizedFreq = 200.0; // Max limit (original oversampling)

        // If the optimized frequency is way smaller than the current sampling frequency, we adapt!
        if (abs(currentSamplingFreq - optimizedFreq) > 2.0) {
            currentSamplingFreq = optimizedFreq;
        }
    }
}