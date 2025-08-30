// Dr. Water - Intelligent Cartridge Monitoring System Firmware
// Version 2.1 - First-Run Init & Floating-Point Volume

#include <EEPROM.h>

// --- Configuration Constants ---

// Flow Sensor is on Pin 2 (must be an interrupt pin)
const int flowSensorPin = 2;

// Calibration: Pulses from the sensor per liter of water.
const int PULSES_PER_LITER = 450;
// Calibration for speed: Based on datasheet, F(Hz) = 7.5 * Q(L/min)
const float FLOW_RATE_COEFFICIENT = 7.5; 

// --- Cartridge Configuration ---
const int NUM_CARTRIDGES = 7;
// Lifespans are now floats to match the new volume tracking
const float CARTRIDGE_LIFESPANS[NUM_CARTRIDGES] = {
  700.0,    // Cartridge 1
  1000.0,   // Cartridge 2
  1200.0,   // Cartridge 3
  1500.0,   // Cartridge 4
  18000.0,  // Cartridge 5
  20000.0,  // Cartridge 6
  23000.0   // Cartridge 7
};

// --- Global Variables ---
float totalSystemVolume = 0.0;
float cartridgeResetAt[NUM_CARTRIDGES] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

volatile unsigned long pulseCount = 0; // For total volume
volatile unsigned int pulseCountForSpeed = 0; // For current speed calculation

float currentSpeedLPM = 0.0; // Liters Per Minute
float highestSpeedLPM = 0.0;

unsigned long lastUpdateTime = 0; // For timing the 1-second updates

// --- EEPROM Address Management ---
const int ADDR_VOLUME = 0;                     // Address for totalSystemVolume (4 bytes)
const int ADDR_RESETS = 4;                     // Start address for reset array (7 * 4 = 28 bytes)
const int ADDR_HIGHEST_SPEED = 32;             // Address for highestSpeedLPM (4 bytes)
const int ADDR_INIT_FLAG = 36;                 // Address to check if EEPROM is initialized

const int INIT_FLAG_VALUE = 123; // A "magic number" to verify initialization

// --- Interrupt Service Routine (ISR) ---
void onPulse() {
  pulseCount++;
  pulseCountForSpeed++;
}

// --- Setup Function ---
void setup() {
  Serial.begin(9600);
  Serial.println("Dr. Water - Advanced Monitor Initializing...");

  int initFlag;
  EEPROM.get(ADDR_INIT_FLAG, initFlag);

  if (initFlag != INIT_FLAG_VALUE) {
    // This is the first time the device has run. Initialize everything to 0.
    Serial.println("First run detected. Initializing EEPROM...");
    totalSystemVolume = 0.0;
    for (int i = 0; i < NUM_CARTRIDGES; i++) {
      cartridgeResetAt[i] = 0.0;
    }
    highestSpeedLPM = 0.0;

    // Save the clean, zeroed values to EEPROM
    EEPROM.put(ADDR_VOLUME, totalSystemVolume);
    EEPROM.put(ADDR_RESETS, cartridgeResetAt);
    EEPROM.put(ADDR_HIGHEST_SPEED, highestSpeedLPM);

    // Set the flag so this block doesn't run again
    EEPROM.put(ADDR_INIT_FLAG, INIT_FLAG_VALUE);
    Serial.println("Initialization complete.");
  } else {
    // Not the first run, load existing data as usual.
    Serial.println("Loading existing data from EEPROM.");
    EEPROM.get(ADDR_VOLUME, totalSystemVolume);
    EEPROM.get(ADDR_RESETS, cartridgeResetAt);
    EEPROM.get(ADDR_HIGHEST_SPEED, highestSpeedLPM);
  }
  
  Serial.print("Initial Total Volume: ");
  Serial.println(totalSystemVolume, 2); // Print with 2 decimal places

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), onPulse, FALLING);

  Serial.println("System Ready. Monitoring flow...");
  lastUpdateTime = millis();
}

// --- Main Loop ---
void loop() {
  if (millis() - lastUpdateTime >= 1000) {
    calculateSpeed();
    processTotalVolume();
    printSerialReport();
    lastUpdateTime = millis();
  }
}

// --- Core Functions ---
void calculateSpeed() {
  noInterrupts();
  unsigned int pulses = pulseCountForSpeed;
  pulseCountForSpeed = 0;
  interrupts();
  
  currentSpeedLPM = (float)pulses / FLOW_RATE_COEFFICIENT;

  if (currentSpeedLPM > highestSpeedLPM) {
    highestSpeedLPM = currentSpeedLPM;
    EEPROM.put(ADDR_HIGHEST_SPEED, highestSpeedLPM);
  }
}

void processTotalVolume() {
  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0; // Reset after reading
  interrupts();

  if (pulses > 0) {
    // Calculate the fractional liters that have passed
    float litersPassed = (float)pulses / PULSES_PER_LITER;
    totalSystemVolume += litersPassed;
    
    // Save the new total volume permanently
    EEPROM.put(ADDR_VOLUME, totalSystemVolume);
  }
}

void printSerialReport() {
  Serial.println("--- Dr. Water Live System Status ---");
  
  Serial.print("Total Volume Passed: ");
  Serial.print(totalSystemVolume, 2); // Print with 2 decimal places
  Serial.println(" L");

  Serial.print("Current Flow Rate:   ");
  Serial.print(currentSpeedLPM, 2);
  Serial.println(" L/min");

  Serial.print("Highest Recorded:    ");
  Serial.print(highestSpeedLPM, 2);
  Serial.println(" L/min");

  Serial.println("\n--- Cartridge Status ---");

  for (int i = 0; i < NUM_CARTRIDGES; i++) {
    float currentUsage = totalSystemVolume - cartridgeResetAt[i];
    float lifespan = CARTRIDGE_LIFESPANS[i];
    float litersLeft = lifespan - currentUsage;
    if (litersLeft < 0) litersLeft = 0;

    String status = "OK";
    if (currentUsage >= lifespan) {
      status = "REPLACE";
    } else if (currentUsage >= lifespan * 0.9) {
      status = "Warning";
    }
    
    Serial.print("Cartridge #");
    Serial.print(i + 1);
    Serial.print(": [Status: ");
    Serial.print(status);
    Serial.print("] - Used: ");
    Serial.print(currentUsage, 2);
    Serial.print("L, Remaining: ");
    Serial.print(litersLeft, 2);
    Serial.println("L");
  }
  Serial.println("-------------------------------------\n");
}

