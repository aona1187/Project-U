// M5Atom + KXR94-2050 dual accelerometer bar controller
// Simple version for single M5Atom reading two KXR94-2050 sensors
// Outputs CSV data over Serial for Unity block breaker game
//
// Hardware setup:
// - Single M5Atom with two KXR94-2050 sensors
// - Sensor 1: バーの左端 (GPIO32, GPIO33, GPIO25)
// - Sensor 2: バーの右端 (GPIO26, GPIO19, GPIO27)
//
// KXR94-2050 Wiring (analog output):
// Sensor 1: X->GPIO32, Y->GPIO33, Z->GPIO25
// Sensor 2: X->GPIO26, Y->GPIO19, Z->GPIO27
// Both: VCC->3.3V, GND->GND

#include <Arduino.h>

// Pin definitions for two KXR94-2050 sensors
const int X_PIN_1 = 32;  // Sensor 1 X-axis

const int Y_PIN_1 = 33;  // Sensor 1 Y-axis  
const int Z_PIN_1 = 25;  // Sensor 1 Z-axis

const int X_PIN_2 = 26;  // Sensor 2 X-axis
const int Y_PIN_2 = 19;  // Sensor 2 Y-axis
const int Z_PIN_2 = 27;  // Sensor 2 Z-axis

// KXR94-2050 specifications
const float SUPPLY_VOLTAGE = 3.3;     // V
const float SENSITIVITY = 0.66;       // V/g (typical for KXR94-2050)
const float ZERO_G_OFFSET = 1.65;     // V (VCC/2)
const int ADC_MAX = 4095;             // 12-bit ADC for ESP32

// Calibration offsets
float offset1[3] = {0, 0, 0};
float offset2[3] = {0, 0, 0};

// Low-pass filter
const float LPF_ALPHA = 0.6;
float filt1[3] = {0, 0, 0};
float filt2[3] = {0, 0, 0};

// Sampling control
unsigned long lastSample = 0;
const unsigned long sampleInterval = 10; // 100 Hz

// Convert ADC reading to acceleration in g
float adcToAccel(int adcValue) {
  float voltage = (adcValue * SUPPLY_VOLTAGE) / ADC_MAX;
  return (voltage - ZERO_G_OFFSET) / SENSITIVITY;
}

// Read 3-axis acceleration from sensor
void readAccel(int xPin, int yPin, int zPin, float accel[3]) {
  accel[0] = adcToAccel(analogRead(xPin));
  accel[1] = adcToAccel(analogRead(yPin));
  accel[2] = adcToAccel(analogRead(zPin));
}

// Calibrate sensor (assumes flat and stationary)
void calibrate(int xPin, int yPin, int zPin, float offsets[3]) {
  const int samples = 200;
  float sum[3] = {0, 0, 0};
  float accel[3];
  
  for (int i = 0; i < samples; ++i) {
    readAccel(xPin, yPin, zPin, accel);
    sum[0] += accel[0];
    sum[1] += accel[1];
    sum[2] += accel[2];
    delay(5);
  }
  
  offsets[0] = sum[0] / samples;
  offsets[1] = sum[1] / samples;
  offsets[2] = sum[2] / samples - 1.0;  // Remove 1g gravity on Z
}

// Apply low-pass filter
void lowPass(float in[3], float out[3], float alpha) {
  for (int i = 0; i < 3; ++i) {
    out[i] = alpha * in[i] + (1 - alpha) * out[i];
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  
  // Configure ADC for 12-bit resolution
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // 0-3.3V range
  
  Serial.println("M5Atom + KXR94-2050 Bar Controller");
  Serial.println("Calibrating sensors (keep still)...");
  
  // Calibrate both sensors
  calibrate(X_PIN_1, Y_PIN_1, Z_PIN_1, offset1);
  calibrate(X_PIN_2, Y_PIN_2, Z_PIN_2, offset2);
  
  // Initialize filters
  for (int i = 0; i < 3; ++i) {
    filt1[i] = -offset1[i];
    filt2[i] = -offset2[i];
  }
  
  Serial.println("Calibration complete!");
  Serial.println("time_ms,ax1,ay1,az1,ax2,ay2,az2");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample < sampleInterval) return;
  lastSample = now;
  
  float accel1[3], accel2[3];
  
  // Read both sensors
  readAccel(X_PIN_1, Y_PIN_1, Z_PIN_1, accel1);
  readAccel(X_PIN_2, Y_PIN_2, Z_PIN_2, accel2);
  
  // Apply calibration offsets
  accel1[0] -= offset1[0];
  accel1[1] -= offset1[1];
  accel1[2] -= offset1[2];
  
  accel2[0] -= offset2[0];
  accel2[1] -= offset2[1];
  accel2[2] -= offset2[2];
  
  // Apply low-pass filter
  lowPass(accel1, filt1, LPF_ALPHA);
  lowPass(accel2, filt2, LPF_ALPHA);
  
  // Output CSV: time_ms,ax1,ay1,az1,ax2,ay2,az2
  Serial.print(now);
  Serial.print(',');
  Serial.print(filt1[0], 4); Serial.print(',');
  Serial.print(filt1[1], 4); Serial.print(',');
  Serial.print(filt1[2], 4); Serial.print(',');
  Serial.print(filt2[0], 4); Serial.print(',');
  Serial.print(filt2[1], 4); Serial.print(',');
  Serial.print(filt2[2], 4);
  Serial.println();
}
