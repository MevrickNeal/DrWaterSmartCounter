// =============================================================================
// Dr. Water Smart Counter - STEP 2 (FINAL): OFFLINE ADMIN CONTROLLER
// Target Board: NodeMCU 1.0 (ESP-12E Module)
// Goal: A fully functional offline controller with a password-protected
// serial command interface for configuration and resets.
// VERSION 2.9 UPDATE: Reverted to a patient, prompt-based serial handler
//                     to support the new interactive web UI.
// =============================================================================

// --- LIBRARIES ---
#include <EEPROM.h>

// --- HARDWARE PIN CONFIGURATION (Stable 3-Cartridge Setup) ---
const int FLOW_SENSOR_PIN = D4; // GPIO2 - DO NOT CHANGE
const int CARTRIDGE_LED_PINS[3] = { D1, D2, D5 }; // GPIO5, 4, 14
const int CARTRIDGE_BUTTON_PINS[3] = { D6, D7, D3 }; // GPIO12, 13, 0
// WIRING NOTE: Buttons should be wired from the pin to GND. We use internal pull-ups.

// --- ADMIN CREDENTIALS ---
const String adminUser = "drwtr01";
const String adminPass = "1234";

// --- SENSOR & DATA CONFIGURATION ---
const float PULSES_PER_LITER = 450.0;
const int NUM_CARTRIDGES = 3;

// EEPROM Addresses - CORRECTED DYNAMIC MEMORY MAP
const int ADDR_MAGIC_NUM = 0;
const int ADDR_VOLUME = ADDR_MAGIC_NUM + sizeof(unsigned long);
const int ADDR_RESETS = ADDR_VOLUME + sizeof(float);
const int ADDR_HIGHEST_SPEED = ADDR_RESETS + sizeof(unsigned long[NUM_CARTRIDGES]);
const int ADDR_LIFESPANS = ADDR_HIGHEST_SPEED + sizeof(float);

// Global variables
float totalSystemVolume = 0.0;
float currentSpeedLPM = 0.0;
float highestSpeedLPM = 0.0;
unsigned long cartridgeResetAt[NUM_CARTRIDGES];
unsigned long cartridgeLifespans[NUM_CARTRIDGES];

// Timing & interrupt variables
unsigned long lastSerialSendTime = 0;
unsigned long lastSpeedCalcTime = 0;
unsigned long lastPulseTime = 0;
volatile unsigned long pulseCount = 0;
volatile unsigned long pulsesForSpeed = 0;

// =============================================================================
// INTERRUPT SERVICE ROUTINE
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
    EEPROM.put(ADDR_LIFESPANS, cartridgeLifespans);
    EEPROM.commit();
}

void loadDataFromEEPROM() {
    unsigned long magicNumber;
    EEPROM.get(ADDR_MAGIC_NUM, magicNumber);

    if (magicNumber != 13371337UL) {
        Serial.println("First run. Initializing EEPROM with default values.");
        totalSystemVolume = 0.0;
        highestSpeedLPM = 0.0;
        for (int i = 0; i < NUM_CARTRIDGES; i++) {
            cartridgeResetAt[i] = 0;
        }
        cartridgeLifespans[0] = (unsigned long)(7000 * PULSES_PER_LITER);
        cartridgeLifespans[1] = (unsigned long)(10000 * PULSES_PER_LITER);
        cartridgeLifespans[2] = (unsigned long)(13000 * PULSES_PER_LITER);
        
        EEPROM.put(ADDR_MAGIC_NUM, 13371337UL);
        saveDataToEEPROM();
    } else {
        Serial.println("Loading data from EEPROM.");
        EEPROM.get(ADDR_VOLUME, totalSystemVolume);
        EEPROM.get(ADDR_RESETS, cartridgeResetAt);
        EEPROM.get(ADDR_HIGHEST_SPEED, highestSpeedLPM);
        EEPROM.get(ADDR_LIFESPANS, cartridgeLifespans);
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
        saveDataToEEPROM();
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
        if (now - lastPulseTime > 2000) { currentSpeedLPM = 0.0; }
        if (currentSpeedLPM > highestSpeedLPM) { highestSpeedLPM = currentSpeedLPM; }
        lastSpeedCalcTime = now;
    }
}

void updateLEDs() {
    for (int i = 0; i < NUM_CARTRIDGES; i++) {
        float usedPulses = (totalSystemVolume * PULSES_PER_LITER) - cartridgeResetAt[i];
        if (usedPulses < 0) usedPulses = 0;
        float percentageUsed = (cartridgeLifespans[i] > 0) ? ((float)usedPulses / cartridgeLifespans[i]) * 100.0 : 0;
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
    if (millis() - lastSerialSendTime >= 1000) {
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

// Reverted to patient, prompt-based handler
void handleSerialCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command.length() == 0) return;

        Serial.println("Enter User ID:");
        String user = "";
        while (Serial.available() == 0) { delay(100); }
        user = Serial.readStringUntil('\n');
        user.trim();

        Serial.println("Enter Password:");
        String pass = "";
        while (Serial.available() == 0) { delay(100); }
        pass = Serial.readStringUntil('\n');
        pass.trim();

        if (user != adminUser || pass != adminPass) {
            Serial.println("AUTH_ERROR: Invalid credentials.");
            return;
        }
        
        if (command == "h") {
            EEPROM.put(ADDR_MAGIC_NUM, 0UL);
            EEPROM.commit();
            Serial.println("SUCCESS: Hard Reset complete. Please reboot.");
            delay(1000);
            ESP.restart();
        } else if (command.startsWith("c=") && command.indexOf("=") == 1) {
            int cartIndex = command.substring(2).toInt() - 1;
            if (cartIndex >= 0 && cartIndex < NUM_CARTRIDGES) {
                cartridgeResetAt[cartIndex] = (unsigned long)(totalSystemVolume * PULSES_PER_LITER);
                saveDataToEEPROM();
                Serial.println("SUCCESS: Cartridge " + String(cartIndex + 1) + " reset.");
            } else {
                Serial.println("ERROR: Invalid cartridge number.");
            }
        } else if (command.startsWith("c") && command.indexOf("=") > 1) {
            int eqPos = command.indexOf("=");
            int cartIndex = command.substring(1, eqPos).toInt() - 1;
            long newLimit = command.substring(eqPos + 1).toInt();
            if (cartIndex >= 0 && cartIndex < NUM_CARTRIDGES && newLimit > 0) {
                cartridgeLifespans[cartIndex] = (unsigned long)(newLimit * PULSES_PER_LITER);
                saveDataToEEPROM();
                Serial.println("SUCCESS: Cartridge " + String(cartIndex + 1) + " limit set to " + String(newLimit) + " L.");
            } else {
                Serial.println("ERROR: Invalid command format or value.");
            }
        } else {
            Serial.println("ERROR: Unknown command.");
        }
    }
}

// =============================================================================
// SETUP & LOOP
// =============================================================================
void setup() {
    Serial.begin(9600);
    // Pin configurations
    pinMode(FLOW_SENSOR_PIN, INPUT);
    for (int pin : CARTRIDGE_LED_PINS) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
    for (int pin : CARTRIDGE_BUTTON_PINS) { pinMode(pin, INPUT_PULLUP); }

    startupBlink();
    
    EEPROM.begin(512);
    loadDataFromEEPROM();
    
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), onPulse, RISING);
    
    Serial.println("System Ready. Sending JSON data and listening for commands...");
}

void loop() {
    processPulses();
    calculateSpeed();
    updateLEDs();
    checkPhysicalButtons();
    sendSerialData();
    handleSerialCommands();
}

