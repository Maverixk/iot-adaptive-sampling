#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "config.h"

Adafruit_INA219 ina219;

void setup(void) {
  Serial.begin(115200);
  
  // Prevent BSP from crying
  delay(3000); 

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip. Check wiring!");
    while (1) { delay(10); }
  }

  // High precision measurements enabled as my battery has < 16V and ESP32 consumes < 400mA
  ina219.setCalibration_16V_400mA();

  // Stampiamo un Header. Better Serial Plotter lo leggerà per dare un nome automatico ai grafici!
  Serial.println("Current_mA\tPower_mW");
}

void loop(void) {
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  // Stampa i valori separati da tabulazione (\t) per il plotter
  Serial.print(current_mA);
  Serial.print("\t");
  Serial.println(power_mW);

  // 50ms of delay means 20Hz measurements. 
  delay(50);
}