// Dr. Water - Intelligent Cartridge Monitoring System Firmware
// Version 1.5 - Arduino Uno with Multi-Cartridge Tracking for Web App

#include <EEPROM.h>

// --- Configuration Constants ---

// Flow Sensor is on Pin 2 (must be an interrupt pin)
const int flowSensorPin = 2;

// Calibration: Pulses from the sensor per liter of water.
const int PULSES_PER_LITER = 450;

// --- Cartridge Configuration ---
const int NUM_CARTRIDGES = 4;
const unsigned long CARTRIDGE_LIFESPANS[NUM_CARTRIDGES] = {
  700,    // Cartridge 1
  1000,   // Cartridge 2
  1200,   // Cartridge 3
  15000   // Cartridge 4
};

// --- Global Variables ---
unsigned long totalSystemVolume = 0;
// Array to store the 'totalSystemVolume' when each cartridge was last reset
unsigned long cartridgeResetAt[NUM_CARTRIDGES] = {0, 0, 0, 0};
volatile unsigned long pulseCount = 0;
unsigned long lastSerialSend = 0;

// --- EEPROM Address Management ---
const int ADDR_VOLUME = 0; // Address for totalSystemVolume
const int ADDR_RESETS = 4; // Starting address for the reset array (1 unsigned long = 4 bytes)

// --- Interrupt Service Routine (ISR) ---
void onPulse() {
  pulseCount++;
}

// --- Setup Function ---
void setup() {
  Serial.begin(9600);
  Serial.println("Arduino Ready. Loading data...");

  // Load the last saved data from permanent EEPROM memory.
  EEPROM.get(ADDR_VOLUME, totalSystemVolume);
  EEPROM.get(ADDR_RESETS, cartridgeResetAt);
  
  Serial.print("Initial volume loaded: ");
  Serial.println(totalSystemVolume);

  // Configure the flow sensor pin.
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), onPulse, FALLING);

  Serial.println("System Ready. Monitoring water flow...");
}

// --- Main Loop ---
void loop() {
  processPulses();

  // Send the system status to the serial port every 1 second.
  if (millis() - lastSerialSend > 1000) {
    sendSerialData();
    lastSerialSend = millis();
  }
}

// --- Core Functions ---
void processPulses() {
  if (pulseCount >= PULSES_PER_LITER) {
    noInterrupts();
    pulseCount -= PULSES_PER_LITER;
    interrupts();

    totalSystemVolume++;
    // Save the new volume permanently.
    EEPROM.put(ADDR_VOLUME, totalSystemVolume);
  }
}

void sendSerialData() {
  // We send data as a comma-separated string:
  // totalVolume,cart1_status,cart2_status,cart3_status,cart4_status
  // Status: 0=OK, 1=Warning, 2=Replace
  
  Serial.print(totalSystemVolume);

  for (int i = 0; i < NUM_CARTRIDGES; i++) {
    Serial.print(",");
    unsigned long currentUsage = totalSystemVolume - cartridgeResetAt[i];
    unsigned long lifespan = CARTRIDGE_LIFESPANS[i];
    int status = 0; // Default to OK

    if (currentUsage >= lifespan) {
      status = 2; // Replace
    } else if (currentUsage >= lifespan * 0.9) {
      status = 1; // Warning
    }
    Serial.print(status);
  }
  Serial.println(); // Send a newline to mark the end of the message
}

