// =============================================================================
// Dr. Water Smart Counter - STEP 3 (FINAL): STANDALONE WEB SERVER
// Target Board: NodeMCU 1.0 (ESP-12E Module)
// Goal: A final, standalone product with a full-featured web interface.
// This version is optimized and uses the stable 3-cartridge hardware setup.
// =============================================================================

// --- LIBRARIES ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// This file contains the HTML, CSS, and JavaScript for the web interface.
// It MUST be in a separate tab named "index.h" in your Arduino IDE.
#include "index.h"

// --- HARDWARE PIN CONFIGURATION (Stable 3-Cartridge Setup) ---
const int FLOW_SENSOR_PIN = D4; // GPIO2
const int WIFI_LED_PIN = D0;    // GPIO16 - Wi-Fi Active Indicator
const int CARTRIDGE_LED_PINS[3] = { D1, D2, D5 }; // GPIO5, 4, 14
const int CARTRIDGE_BUTTON_PINS[3] = { D6, D7, D3 }; // GPIO12, 13, 0

// --- WI-FI & WEB SERVER CONFIGURATION ---
const char* ssid = "Dr.Water SOC";
const char* password = "12345678";
ESP8266WebServer server(80); // Web server runs on port 80

// --- ADMIN CREDENTIALS ---
const String adminUser = "drwtr01";
const String adminPass = "1234";

// --- SENSOR & DATA CONFIGURATION ---
const float PULSES_PER_LITER = 450.0;
const int NUM_CARTRIDGES = 3;

// EEPROM Addresses
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
        totalSystemVolume = 0.0;
        highestSpeedLPM = 0.0;
        for (int i = 0; i < NUM_CARTRIDGES; i++) cartridgeResetAt[i] = 0;
        cartridgeLifespans[0] = (unsigned long)(7000 * PULSES_PER_LITER);
        cartridgeLifespans[1] = (unsigned long)(10000 * PULSES_PER_LITER);
        cartridgeLifespans[2] = (unsigned long)(13000 * PULSES_PER_LITER);
        EEPROM.put(ADDR_MAGIC_NUM, 13371337UL);
        saveDataToEEPROM();
    } else {
        EEPROM.get(ADDR_VOLUME, totalSystemVolume);
        EEPROM.get(ADDR_RESETS, cartridgeResetAt);
        EEPROM.get(ADDR_HIGHEST_SPEED, highestSpeedLPM);
        EEPROM.get(ADDR_LIFESPANS, cartridgeLifespans);
    }
}

// =============================================================================
// WEB SERVER HANDLERS
// =============================================================================
void handleRoot() {
    server.send_P(200, "text/html", HTML_CONTENT);
}

void handleData() {
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
    server.send(200, "application/json", json);
}

void handleCommand() {
    if (!server.hasArg("user") || !server.hasArg("pass") || server.arg("user") != adminUser || server.arg("pass") != adminPass) {
        server.send(401, "text/plain", "AUTH_ERROR: Invalid credentials.");
        return;
    }
    if (server.hasArg("cmd")) {
        String cmd = server.arg("cmd");
        if (cmd == "h") {
            EEPROM.put(ADDR_MAGIC_NUM, 0UL);
            EEPROM.commit();
            server.send(200, "text/plain", "SUCCESS: Hard Reset complete. Device will now reboot.");
            delay(1000);
            ESP.restart();
        } else if (cmd.startsWith("c=")) {
            int cartIndex = cmd.substring(2).toInt() - 1;
            if (cartIndex >= 0 && cartIndex < NUM_CARTRIDGES) {
                cartridgeResetAt[cartIndex] = (unsigned long)(totalSystemVolume * PULSES_PER_LITER);
                saveDataToEEPROM();
                server.send(200, "text/plain", "SUCCESS: Cartridge " + String(cartIndex + 1) + " has been reset.");
            } else {
                 server.send(400, "text/plain", "ERROR: Invalid cartridge number.");
            }
        }
    }
}

void handleSetLimits() {
    if (!server.hasArg("user") || !server.hasArg("pass") || server.arg("user") != adminUser || server.arg("pass") != adminPass) {
        server.send(401, "text/plain", "AUTH_ERROR: Invalid credentials.");
        return;
    }
    bool success = true;
    for (int i = 0; i < NUM_CARTRIDGES; i++) {
        String argName = "limit" + String(i + 1);
        if (server.hasArg(argName)) {
            unsigned long newLimit = server.arg(argName).toInt();
            if (newLimit > 0) {
                cartridgeLifespans[i] = newLimit * PULSES_PER_LITER;
            } else { success = false; }
        } else { success = false; }
    }
    if (success) {
        saveDataToEEPROM();
        server.send(200, "text/plain", "SUCCESS: Cartridge limits updated.");
    } else {
        server.send(400, "text/plain", "ERROR: Invalid or missing limit data.");
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

// =============================================================================
// SETUP & LOOP
// =============================================================================
void setup() {
    // Pin configurations
    pinMode(FLOW_SENSOR_PIN, INPUT);
    for (int pin : CARTRIDGE_LED_PINS) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
    for (int pin : CARTRIDGE_BUTTON_PINS) { pinMode(pin, INPUT_PULLUP); }
    pinMode(WIFI_LED_PIN, OUTPUT);
    digitalWrite(WIFI_LED_PIN, LOW);

    EEPROM.begin(512);
    loadDataFromEEPROM();
    
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), onPulse, RISING);
    
    WiFi.softAP(ssid, password);
    digitalWrite(WIFI_LED_PIN, HIGH); // Turn on LED to indicate Wi-Fi is active

    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.on("/command", HTTP_POST, handleCommand);
    server.on("/setlimits", HTTP_POST, handleSetLimits);
    server.begin();
}

void loop() {
    server.handleClient(); // Handle incoming web requests
    processPulses();
    calculateSpeed();
    updateLEDs();
    checkPhysicalButtons();
}

