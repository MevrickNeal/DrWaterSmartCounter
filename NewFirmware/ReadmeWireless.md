# Dr. Water Smart Cartridge Monitor – Standalone Wi-Fi Version  
**Version:** 3.0 (Final)  
**Lead Developer:** Lian Mollick  

---

## 1. Project Overview  
This is the **final, standalone release** of the Dr. Water Smart Cartridge Monitor.  
The device has evolved from a USB-tethered prototype into a **self-contained smart product**.  

It now operates as its own **Wi-Fi access point** and hosts an **onboard web server**, enabling completely wireless monitoring and administration via any modern smartphone or computer.  

The core functionality remains the same:  
- Accurately track the lifespan of water filter cartridges based on actual water volume.  
- Provide precise replacement alerts to users and technician-level controls for field maintenance.  

---

## 2. Key Features  

- **Standalone Wi-Fi Access Point**  
  Creates its own secure Wi-Fi network (`Dr.Water SOC`) with no internet dependency.  

- **Onboard Web Server**  
  Hosts a **mobile-friendly web interface** accessible from any connected device.  

- **Real-Time Wireless Monitoring**  
  Streams live data (total volume, flow rates, cartridge usage) directly to browsers.  

- **Full Wireless Administration**  
  All technician controls (reset, change limit, hard reset) available via password-protected web UI.  

- **Persistent Memory**  
  Usage data and settings stored in EEPROM to survive power loss.  

- **Physical Redundancy**  
  Hardware buttons and status LEDs remain for on-site diagnostics and fallback control.  

---

## 3. How to Use (User & Technician Guide)  

### Step-by-Step Instructions  

1. **Power On the Device**  
   - Connect the ESP8266 to a standard USB power source.  
   - Wait for boot-up.  

2. **Wait for Wi-Fi Indicator**  
   - The **Wi-Fi status LED (D0)** will light up, showing the Access Point is active.  

3. **Connect Your Phone/Laptop**  
   - Open Wi-Fi settings → find and connect to: **`Dr.Water SOC`**  
   - Enter the password: **`12345678`**  
   - *(Optional: Generate a Wi-Fi QR code for instant technician access.)*  

4. **Access the Control Panel**  
   - Open a browser (Chrome, Safari, Edge).  
   - Enter **`192.168.4.1`** in the address bar.  
   - *(Optional: QR code can be generated for this URL.)*  

5. **Use the Web Interface**  
   - The macOS-inspired dashboard loads automatically.  
   - Displays **live water flow data** and **cartridge usage**.  
   - Technician Controls (password-protected):  
     - **Reset Usage** → Reset cartridge counter to 0.  
     - **Change Limit** → Define new lifespan limits.  
     - **Hard Reset** → Factory reset all data/settings.  

---

## 4. Technical Details  

### Hardware (Stable 3-Cartridge Prototype)  
- 1 × NodeMCU ESP8266 Development Board  
- 1 × YF-S201 Hall Effect Water Flow Sensor  
- 4 × Standard LEDs (3 for cartridges, 1 for Wi-Fi status)  
- 3 × Tactile Push Buttons  
- 4 × 220Ω Resistors (for LEDs)  
- 1 × External 5V Power Supply (for flow sensor)  
- Breadboard & Jumper Wires  

### Pin Diagram  

| Component         | Pin on NodeMCU | Connection Details                                                |
|-------------------|----------------|-------------------------------------------------------------------|
| Flow Sensor       | D4             | Signal                                                            |
| Wi-Fi Status LED  | D0             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| LED 1 (Cartridge) | D1             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| LED 2 (Cartridge) | D2             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| LED 3 (Cartridge) | D5             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| Button 1          | D6             | Pin → Button → GND (Uses internal pull-up)                        |
| Button 2          | D7             | Pin → Button → GND (Uses internal pull-up)                        |
| Button 3          | D3             | Pin → Button → GND (Uses internal pull-up)                        |

---

## 5. Version History (Final Changelog)  

### Firmware (`esp8266_firmware.ino`)  
- **v1.0: Proof of Concept**  
  - Calibrated YF-S201 sensor and measured water volume.  

- **v2.0: Stable Offline Controller**  
  - Added EEPROM persistence, LEDs, buttons, and 3-cartridge logic.  

- **v2.5: Admin Command Interface**  
  - Added password-protected serial command handler.  

- **v2.9: Interactive Serial Handler**  
  - Re-engineered the serial handler for full interactivity and reliability.  

- **v3.0 (Final): Standalone Web Server**  
  - Migrated to Wi-Fi Access Point mode.  
  - Hosted full-featured onboard web server.  
  - Removed legacy serial command logic.  

### Web Interface (`index.h`)  
- **v1.0: Initial Data Display**  
  - Basic live data via Web Serial API.  

- **v2.0: Multi-Cartridge Dashboard**  
  - UI upgrade to support multiple cartridges.  

- **v3.5: Interactive Serial UI**  
  - Solved race conditions and overwrite bugs with dialog-driven system.  

- **v3.6 (Final): Wi-Fi Web Application**  
  - Migrated UI to onboard Wi-Fi web server.  
  - Fully wireless monitoring + technician controls.  

---
