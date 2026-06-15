#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h" // Pro bleskový tep (checkForBeat)
#include "spo2_algorithm.h" // Pro výpočet kyslíku
#include <SoftwareSerial.h>

MAX30105 particleSensor;
SoftwareSerial btSerial(10, 11);

// Proměnné pro tep
long lastBeat = 0; 
int beatAvg = 75; 

// Proměnné pro kyslík
uint16_t irBuffer[25]; 
uint16_t redBuffer[25];
int32_t spo2
int8_t validSPO2;
int32_t dummyHR; 
int8_t dummyVHR;

unsigned long lastOxygenUpdate = 0;

void setup() {
  Serial.begin(115200);
  btSerial.begin(9600);
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Senzor nenalezen!");
    while (1);
  }
  
  // Nastavení senzoru
  particleSensor.setup(60, 4, 2, 100, 411, 4096); 
  Serial.println("System bezi: Tep stale, kyslik kazdych 10s.");
}

void loop() {
  long irValue = particleSensor.getIR();

  // 1. NEUSTÁLÉ SLEDOVÁNÍ TEPU
  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    float beatsPerMinute = 60 / (delta / 1000.0);
    
    if (beatsPerMinute < 160 && beatsPerMinute > 40) {
      beatAvg = (int)beatsPerMinute; 
    }
  }

  // 2. AKTUALIZACE KYSLÍKU KAŽDÝCH 10 SEKUND
  if (millis() - lastOxygenUpdate > 10000) { // Změněno na 10 sekund
    
    // 25 vzorků (to trvá cca 1 sekundu)
    for (byte i = 0 ; i < 25 ; i++) {
      while (particleSensor.available() == false) particleSensor.check();
      redBuffer[i] = (uint16_t)particleSensor.getRed();
      irBuffer[i] = (uint16_t)particleSensor.getIR();
      particleSensor.nextSample();
    }
    
  
    maxim_heart_rate_and_oxygen_saturation(irBuffer, 25, redBuffer, &spo2, &validSPO2, &dummyHR, &dummyVHR);
    
    // ODESLÁNÍ DAT PŘES BLUETOOTH
    btSerial.print(beatAvg);
    btSerial.print(",");
    if (validSPO2 && spo2 > 80 && spo2 <= 100) {
      btSerial.println(spo2);
    }
    
    Serial.print("--- Odeslano do Pythonu: HR "); Serial.print(beatAvg);
    Serial.print(" SPO2 "); Serial.println(validSPO2 ? spo2 : 98);
    
    lastOxygenUpdate = millis();
  }
}
