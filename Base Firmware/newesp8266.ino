/**
 * Dr. Water - Intelligent Cartridge Monitoring System
 * Version: 3.0 (ESP8266 Standalone Web Server)
 * * This firmware turns an ESP8266 into a standalone monitoring device.
 * It creates its own Wi-Fi Access Point and serves a web interface
 * for real-time monitoring and control.
 */

// --- LIBRARIES ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// --- HARDWARE CONFIGURATION ---
const int FLOW_SENSOR_PIN = 2;  // D4 on NodeMCU (Must be an interrupt pin)

// Pins for the 7 cartridge status LEDs
const int CARTRIDGE_LED_PINS[7] = {16, 5, 4, 0, 14, 12, 13}; // D0, D1, D2, D3, D5, D6, D7

// Pins for the 7 manual reset push buttons
const int CARTRIDGE_BUTTON_PINS[7] = {10, 9, 1, 3, 8, 7, 6}; // SD3, SD2, TX, RX, SD1, HMISO, HMOSI (Note: Use with care, some are special pins)

// Pin for the Wi-Fi Access Point status LED
const int WIFI_STATUS_LED_PIN = 15; // D8

// --- WIFI & WEB SERVER CONFIGURATION ---
const char* ssid = "DrWater_Monitor";
const char* password = "drwater123";
ESP8266WebServer server(80); // Web server runs on port 80

// --- SYSTEM LOGIC & MEMORY ---
#include "index.h" // The web page HTML is stored in this separate tab/file

// EEPROM Addresses for permanent storage
#define ADDR_MAGIC_NUM 0
#define ADDR_VOLUME 4
#define ADDR_RESETS 8
#define ADDR_HIGHEST_SPEED 100

// A magic number to check if EEPROM has been initialized
const uint32_t MAGIC_NUMBER = 0xDEADBEEF;

// Cartridge lifespans in Liters
const float CARTRIDGE_LIFESPANS[7] = {700.0, 1000.0, 1200.0, 15000.0, 3000.0, 6000.0, 10000.0};

// --- GLOBAL VARIABLES ---
volatile unsigned long pulseCount = 0;
float totalSystemVolume = 0.0;
float cartridgeResetAt[7] = {0.0};
float highestSpeedLPM = 0.0;
float currentSpeedLPM = 0.0;

unsigned long lastSerialPrintTime = 0;
unsigned long lastFlowTime = 0;

// --- INTERRUPT SERVICE ROUTINE (ISR) ---
// This function runs every time a pulse is detected from the flow sensor.
// It must be fast! ICACHE_RAM_ATTR ensures it runs from RAM for speed.
void ICACHE_RAM_ATTR onPulse() {
    pulseCount++;
}

// --- CORE LOGIC FUNCTIONS ---

void saveDataToEEPROM() {
    EEPROM.put(ADDR_VOLUME, totalSystemVolume);
    EEPROM.put(ADDR_RESETS, cartridgeResetAt);
    EEPROM.put(ADDR_HIGHEST_SPEED, highestSpeedLPM);
    EEPROM.commit(); // Saves changes to flash
}

void hardResetSystem() {
    totalSystemVolume = 0.0;
    for (int i = 0; i < 7; i++) {
        cartridgeResetAt[i] = 0.0;
    }
    highestSpeedLPM = 0.0;
    // Overwrite the magic number to force re-initialization on next boot
    EEPROM.put(ADDR_MAGIC_NUM, (uint32_t)0); 
    EEPROM.commit();
    saveDataToEEPROM(); // Save the zeroed values
}

void resetCartridge(int cartridgeIndex) {
    if (cartridgeIndex >= 0 && cartridgeIndex < 7) {
        cartridgeResetAt[cartridgeIndex] = totalSystemVolume;
        saveDataToEEPROM();
    }
}

void processTotalVolume() {
    if (pulseCount > 0) {
        // Prevent interrupts from changing pulseCount while we calculate
        noInterrupts();
        unsigned long pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        float flowAmount = (float)pulses / 450.0; // 450 pulses per liter
        totalSystemVolume += flowAmount;

        unsigned long now = millis();
        unsigned long timeDiff = now - lastFlowTime;
        if (timeDiff > 0) {
            currentSpeedLPM = (flowAmount / timeDiff) * 60000.0;
            if (currentSpeedLPM > highestSpeedLPM) {
                highestSpeedLPM = currentSpeedLPM;
            }
        }
        lastFlowTime = now;
        saveDataToEEPROM();
    } else {
        // If no flow for 1 second, reset current speed to 0
        if(millis() - lastFlowTime > 1000) {
            currentSpeedLPM = 0.0;
        }
    }
}

// --- NEW ESP8266-SPECIFIC FUNCTIONS ---

void updateCartridgeLEDs() {
    for (int i = 0; i < 7; i++) {
        float used = totalSystemVolume - cartridgeResetAt[i];
        if (used < 0) used = 0;
        
        if (used >= CARTRIDGE_LIFESPANS[i]) {
            digitalWrite(CARTRIDGE_LED_PINS[i], HIGH); // Solid ON for REPLACE
        } else if (used >= CARTRIDGE_LIFESPANS[i] * 0.9) {
            // Blinking for WARNING
            digitalWrite(CARTRIDGE_LED_PINS[i], (millis() / 500) % 2);
        } else {
            digitalWrite(CARTRIDGE_LED_PINS[i], LOW); // OFF for OK
        }
    }
}

void checkManualResetButtons() {
    for (int i = 0; i < 7; i++) {
        if (digitalRead(CARTRIDGE_BUTTON_PINS[i]) == LOW) {
            delay(50); // Simple debounce
            if (digitalRead(CARTRIDGE_BUTTON_PINS[i]) == LOW) {
                Serial.printf("Manual reset for Cartridge #%d triggered.\n", i + 1);
                resetCartridge(i);
                delay(500); // Prevent multiple resets from one long press
            }
        }
    }
}

// --- WEB SERVER HANDLERS ---

void handleRoot() {
    // Serve the main web page stored in PROGMEM
    server.send(200, "text/html", HTML_CONTENT);
}

void handleGetData() {
    // This function builds the JSON data string that the web app will request
    String json = "{";
    json += "\"totalVolume\":" + String(totalSystemVolume, 2) + ",";
    json += "\"currentSpeed\":" + String(currentSpeedLPM, 2) + ",";
    json += "\"highestSpeed\":" + String(highestSpeedLPM, 2) + ",";
    json += "\"cartridges\":[";
    for (int i = 0; i < 7; i++) {
        float used = totalSystemVolume - cartridgeResetAt[i];
        if (used < 0) used = 0;
        float remaining = CARTRIDGE_LIFESPANS[i] - used;
        if (remaining < 0) remaining = 0;
        String status = "OK";
        if (used >= CARTRIDGE_LIFESPANS[i]) status = "REPLACE";
        else if (used >= CARTRIDGE_LIFESPANS[i] * 0.9) status = "WARNING";

        json += "{";
        json += "\"status\":\"" + status + "\",";
        json += "\"used\":" + String(used, 2) + ",";
        json += "\"remaining\":" + String(remaining, 2);
        json += "}";
        if (i < 6) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void handleReset() {
    String userId = server.arg("user");
    String pass = server.arg("pass");
    String cmd = server.arg("cmd");

    if (userId == "drwtr01" && pass == "1234") {
        if (cmd.startsWith("c=")) {
            int cartNum = cmd.substring(2).toInt();
            if (cartNum >= 1 && cartNum <= 7) {
                resetCartridge(cartNum - 1);
                server.send(200, "text/plain", "Cartridge " + String(cartNum) + " reset.");
            } else {
                server.send(400, "text/plain", "Invalid Cartridge Number.");
            }
        } else if (cmd == "h") {
            hardResetSystem();
            server.send(200, "text/plain", "System Hard Reset Successful.");
        } else {
            server.send(400, "text/plain", "Invalid Command.");
        }
    } else {
        server.send(401, "text/plain", "Authentication Failed.");
    }
}


// --- SETUP ---
void setup() {
    Serial.begin(9600);
    EEPROM.begin(512); // Initialize EEPROM

    // --- Pin Modes ---
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(WIFI_STATUS_LED_PIN, OUTPUT);
    for (int i = 0; i < 7; i++) {
        pinMode(CARTRIDGE_LED_PINS[i], OUTPUT);
        pinMode(CARTRIDGE_BUTTON_PINS[i], INPUT_PULLUP);
    }
    digitalWrite(WIFI_STATUS_LED_PIN, LOW); // Turn off Wi-Fi LED initially

    // --- First Run Check ---
    uint32_t storedMagic;
    EEPROM.get(ADDR_MAGIC_NUM, storedMagic);
    if (storedMagic != MAGIC_NUMBER) {
        Serial.println("First run detected. Initializing EEPROM.");
        hardResetSystem(); // Initialize all values to 0.0
        EEPROM.put(ADDR_MAGIC_NUM, MAGIC_NUMBER);
        EEPROM.commit();
    } else {
        Serial.println("Loading data from EEPROM.");
        EEPROM.get(ADDR_VOLUME, totalSystemVolume);
        EEPROM.get(ADDR_RESETS, cartridgeResetAt);
        EEPROM.get(ADDR_HIGHEST_SPEED, highestSpeedLPM);
    }

    // --- Flow Sensor Interrupt ---
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), onPulse, FALLING);
    
    // --- Wi-Fi and Web Server Setup ---
    Serial.println("\nStarting Wi-Fi Access Point...");
    WiFi.softAP(ssid, password);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    digitalWrite(WIFI_STATUS_LED_PIN, HIGH); // Turn on Wi-Fi LED

    // Define web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleGetData);
    server.on("/reset", HTTP_POST, handleReset); // Use POST for commands
    server.begin();
    Serial.println("HTTP server started.");
}

// --- LOOP ---
void loop() {
    server.handleClient(); // Process incoming web requests
    processTotalVolume();
    updateCartridgeLEDs();
    checkManualResetButtons();

    // Print status to serial monitor every 2 seconds for debugging
    if (millis() - lastSerialPrintTime > 2000) {
        Serial.printf("Total Volume: %.2f L, Current Speed: %.2f LPM\n", totalSystemVolume, currentSpeedLPM);
        lastSerialPrintTime = millis();
    }
}
