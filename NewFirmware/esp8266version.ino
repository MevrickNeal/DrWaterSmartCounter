// =============================================================================
// Dr. Water Smart Counter - STEP 2 (FINAL): WEB SERIAL CONTROLLER
// Target Board: NodeMCU 1.0 (ESP-12E Module)
// Goal: A clean, optimized offline controller that sends pure JSON data over
// serial for the Web Serial API monitoring tool.
// VERSION 2.4 UPDATE: Removed all debug code for a clean data stream.
// =============================================================================

// --- LIBRARIES ---
#include <EEPROM.h>

// --- HARDWARE PIN CONFIGURATION (Stable 3-Cartridge Setup) ---
const int FLOW_SENSOR_PIN = D4; // GPIO2 - DO NOT CHANGE
const int CARTRIDGE_LED_PINS[3] = { D1, D2, D5 }; // GPIO5, 4, 14
const int CARTRIDGE_BUTTON_PINS[3] = { D6, D7, D3 }; // GPIO12, 13, 0
// WIRING NOTE: Buttons should be wired from the pin to GND. We use internal pull-ups.
// A 10kOhm pull-up resistor from D3 to 3V3 is still highly recommended for stability.

// --- SENSOR & DATA CONFIGURATION ---
const float PULSES_PER_LITER = 450.0;
const int NUM_CARTRIDGES = 3;

// EEPROM Addresses for permanent storage
const int ADDR_MAGIC_NUM = 0;
const int ADDR_VOLUME = 4;
const int ADDR_RESETS = 12;
const int ADDR_HIGHEST_SPEED = ADDR_RESETS + sizeof(unsigned long[NUM_CARTRIDGES]);

// Global variables for sensor readings
float totalSystemVolume = 0.0;
float currentSpeedLPM = 0.0;
float highestSpeedLPM = 0.0;
unsigned long cartridgeResetAt[NUM_CARTRIDGES] = {0, 0, 0};
const unsigned long cartridgeLifespans[NUM_CARTRIDGES] = {
    (unsigned long)(700 * PULSES_PER_LITER),
    (unsigned long)(1000 * PULSES_PER_LITER),
    (unsigned long)(1200 * PULSES_PER_LITER)
};

// Timing variables & interrupt counter
unsigned long lastSaveTime = 0;
unsigned long lastSerialSendTime = 0;
unsigned long lastSpeedCalcTime = 0;
unsigned long lastPulseTime = 0;
volatile unsigned long pulseCount = 0;
volatile unsigned long pulsesForSpeed = 0;

// =============================================================================
// INTERRUPT SERVICE ROUTINE (For Flow Sensor)
// =============================================================================
void ICACHE_RAM_ATTR onPulse() {
    pulseCount++;
    pulsesForSpeed++;
    lastPulseTime = millis();
}

// =============================================================================
// EEPROM & DATA FUNCTIONS
// =============================================================================
void saveDataToEEPROM() {
    EEPROM.put(ADDR_VOLUME, totalSystemVolume);
    EEPROM.put(ADDR_RESETS, cartridgeResetAt);
    EEPROM.put(ADDR_HIGHEST_SPEED, highestSpeedLPM);
    EEPROM.commit();
    lastSaveTime = millis();
}

void loadDataFromEEPROM() {
    unsigned long magicNumber;
    EEPROM.get(ADDR_MAGIC_NUM, magicNumber);

    if (magicNumber != 13371337UL) {
        Serial.println("First run detected. Initializing EEPROM.");
        totalSystemVolume = 0.0;
        highestSpeedLPM = 0.0;
        for (int i = 0; i < NUM_CARTRIDGES; i++) {
            cartridgeResetAt[i] = 0;
        }
        EEPROM.put(ADDR_MAGIC_NUM, 13371337UL);
        saveDataToEEPROM();
    } else {
        Serial.println("Loading data from EEPROM.");
        EEPROM.get(ADDR_VOLUME, totalSystemVolume);
        EEPROM.get(ADDR_RESETS, cartridgeResetAt);
        EEPROM.get(ADDR_HIGHEST_SPEED, highestSpeedLPM);
    }
}

// =============================================================================
// CORE LOGIC FUNCTIONS
// =============================================================================
void processPulses() {
    if (pulseCount > 0) {
        noInterrupts();
        unsigned long pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        totalSystemVolume += (float)pulses / PULSES_PER_LITER;

        if (millis() - lastSaveTime > 5000) {
            saveDataToEEPROM();
        }
    }
}

void calculateSpeed() {
    unsigned long now = millis();
    if (now - lastSpeedCalcTime >= 1000) {
        noInterrupts();
        unsigned long currentPulses = pulsesForSpeed;
        pulsesForSpeed = 0;
        interrupts();

        currentSpeedLPM = ((float)currentPulses / PULSES_PER_LITER) * 60.0;

        if (now - lastPulseTime > 2000 && currentSpeedLPM > 0) {
            currentSpeedLPM = 0.0;
        }

        if (currentSpeedLPM > highestSpeedLPM) {
            highestSpeedLPM = currentSpeedLPM;
        }
        lastSpeedCalcTime = now;
    }
}


void updateLEDs() {
    for (int i = 0; i < NUM_CARTRIDGES; i++) {
        float usedPulses = (totalSystemVolume * PULSES_PER_LITER) - cartridgeResetAt[i];
        if (usedPulses < 0) usedPulses = 0;
        float percentageUsed = ((float)usedPulses / cartridgeLifespans[i]) * 100.0;
        if (percentageUsed >= 100.0) {
            digitalWrite(CARTRIDGE_LED_PINS[i], HIGH);
        } else if (percentageUsed >= 90.0) {
            digitalWrite(CARTRIDGE_LED_PINS[i], (millis() / 500) % 2);
        } else {
            digitalWrite(CARTRIDGE_LED_PINS[i], LOW);
        }
    }
}

void checkPhysicalButtons() {
    for (int i = 0; i < NUM_CARTRIDGES; i++) {
        if (digitalRead(CARTRIDGE_BUTTON_PINS[i]) == LOW) {
            delay(50);
            if (digitalRead(CARTRIDGE_BUTTON_PINS[i]) == LOW) {
                cartridgeResetAt[i] = (unsigned long)(totalSystemVolume * PULSES_PER_LITER);
                saveDataToEEPROM();
                while(digitalRead(CARTRIDGE_BUTTON_PINS[i]) == LOW) { delay(10); }
            }
        }
    }
}

void startupBlink() {
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < NUM_CARTRIDGES; i++) { digitalWrite(CARTRIDGE_LED_PINS[i], HIGH); }
        delay(150);
        for (int i = 0; i < NUM_CARTRIDGES; i++) { digitalWrite(CARTRIDGE_LED_PINS[i], LOW); }
        delay(150);
    }
}

void sendSerialData() {
    if (millis() - lastSerialSendTime >= 1000) { // Send every second
        String json = "{";
        json += "\"totalVolume\":" + String(totalSystemVolume, 2) + ",";
        json += "\"currentSpeed\":" + String(currentSpeedLPM, 2) + ",";
        json += "\"highestSpeed\":" + String(highestSpeedLPM, 2) + ",";
        json += "\"cartridges\":[";

        for (int i = 0; i < NUM_CARTRIDGES; i++) {
            float usedLiters = totalSystemVolume - ((float)cartridgeResetAt[i] / PULSES_PER_LITER);
            if (usedLiters < 0) usedLiters = 0;
            float limitLiters = (float)cartridgeLifespans[i] / PULSES_PER_LITER;
            float remainingLiters = limitLiters - usedLiters;
            if (remainingLiters < 0) remainingLiters = 0;

            json += "{\"used\":" + String(usedLiters, 2) + ",";
            json += "\"remaining\":" + String(remainingLiters, 2) + ",";
            json += "\"limit\":" + String(limitLiters) + "}";
            if (i < NUM_CARTRIDGES - 1) json += ",";
        }

        json += "]}";
        Serial.println(json);
        lastSerialSendTime = millis();
    }
}


// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(9600);
    
    // Pin configurations first
    pinMode(FLOW_SENSOR_PIN, INPUT);
    for (int pin : CARTRIDGE_LED_PINS) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
    for (int pin : CARTRIDGE_BUTTON_PINS) { pinMode(pin, INPUT_PULLUP); }

    startupBlink();
    
    EEPROM.begin(512);
    loadDataFromEEPROM();
    
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), onPulse, RISING);
    
    Serial.println("System Ready. Sending JSON data...");
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
    processPulses();
    calculateSpeed();
    updateLEDs();
    checkPhysicalButtons();
    sendSerialData();
}

