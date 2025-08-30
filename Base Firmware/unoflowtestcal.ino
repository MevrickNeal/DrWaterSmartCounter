// Dr. Water - Intelligent Cartridge Monitoring System Firmware
// Version 2.2 - Technician Password Reset Feature

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

// --- Security Credentials ---
const String TECH_USER_ID = "drwtr01";
const String TECH_PASSWORD = "1234";

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
  Serial.println("\n\nDr. Water - Advanced Monitor Initializing...");

  int initFlag;
  EEPROM.get(ADDR_INIT_FLAG, initFlag);

  if (initFlag != INIT_FLAG_VALUE) {
    initializeEEPROM();
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
  Serial.println("Enter 'h' for Hard Reset or 'c=N' (e.g., c=1) to reset a cartridge.");
  lastUpdateTime = millis();
}

// --- Main Loop ---
void loop() {
  // Check for serial commands
  if (Serial.available() > 0) {
    processSerialCommand();
  }
  
  // Update report every second
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
    float litersPassed = (float)pulses / PULSES_PER_LITER;
    totalSystemVolume += litersPassed;
    EEPROM.put(ADDR_VOLUME, totalSystemVolume);
  }
}

void printSerialReport() {
  Serial.println("--- Dr. Water Live System Status ---");
  Serial.print("Total Volume Passed: ");
  Serial.print(totalSystemVolume, 2);
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

// --- New Functions for Technician Reset ---

void processSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command == "h") {
    handleHardReset();
  } else if (command.startsWith("c=")) {
    int cartridgeNum = command.substring(2).toInt();
    handleCartridgeReset(cartridgeNum);
  }
}

bool checkPassword() {
  Serial.println("Technician Authentication Required.");
  Serial.print("Enter User ID: ");
  String userID = readSerialLine();
  Serial.print("Enter Password: ");
  String password = readSerialLine();

  if (userID == TECH_USER_ID && password == TECH_PASSWORD) {
    Serial.println("Authentication successful.");
    return true;
  } else {
    Serial.println("Authentication failed. Aborting.");
    return false;
  }
}

String readSerialLine() {
  while (Serial.available() == 0) {
    // Wait for user input
  }
  String line = Serial.readStringUntil('\n');
  line.trim();
  return line;
}

void handleHardReset() {
  if (checkPassword()) {
    Serial.println("Performing Hard Reset. All data will be wiped.");
    initializeEEPROM();
    Serial.println("Hard Reset complete. System will now restart.");
    delay(1000);
    // Force a software reset
    asm volatile ("jmp 0");
  }
}

void handleCartridgeReset(int cartridgeNum) {
  if (cartridgeNum < 1 || cartridgeNum > NUM_CARTRIDGES) {
    Serial.println("Invalid cartridge number.");
    return;
  }
  
  if (checkPassword()) {
    int index = cartridgeNum - 1;
    cartridgeResetAt[index] = totalSystemVolume;
    EEPROM.put(ADDR_RESETS, cartridgeResetAt); // Save the entire updated array
    Serial.print("Cartridge #");
    Serial.print(cartridgeNum);
    Serial.println(" has been reset.");
  }
}

void initializeEEPROM() {
  Serial.println("First run detected or Hard Reset initiated. Initializing EEPROM...");
  totalSystemVolume = 0.0;
  for (int i = 0; i < NUM_CARTRIDGES; i++) {
    cartridgeResetAt[i] = 0.0;
  }
  highestSpeedLPM = 0.0;

  EEPROM.put(ADDR_VOLUME, totalSystemVolume);
  EEPROM.put(ADDR_RESETS, cartridgeResetAt);
  EEPROM.put(ADDR_HIGHEST_SPEED, highestSpeedLPM);
  EEPROM.put(ADDR_INIT_FLAG, INIT_FLAG_VALUE);
  Serial.println("Initialization complete.");
}

