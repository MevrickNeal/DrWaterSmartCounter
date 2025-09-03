# Dr. Water Smart Cartridge Monitor  
**Version:** 2.9 (Stable Offline Controller with Web Serial Interface)  
**Lead Developer:** Lian Mollick  

---

## 1. Project Overview  
The **Dr. Water Smart Cartridge Monitor** is a prototype for an intelligent, low-cost device that accurately tracks the lifespan of water filter cartridges based on actual water volume.  

This system replaces simple timers with a precise, usage-based measurement, providing:  
- **Users** with timely alerts for cartridge replacement.  
- **Technicians** with a powerful tool for configuration and maintenance.  

This version is a **fully functional offline controller** that uses a **Web Serial API-based interface** for real-time monitoring and administration.  

---

## 2. Key Features  

- **Real-Time Flow Tracking**  
  Accurately measures total water volume, current flow rate, and peak flow rate.  

- **Multi-Cartridge Support**  
  Tracks the independent lifespan of multiple filter cartridges (currently configured for **3 cartridges**).  

- **Persistent Memory**  
  All data is stored in non-volatile memory (EEPROM), ensuring no data loss during power outages.  

- **Physical Controls**  
  Supports hardware buttons for technicians to reset cartridge counters in the field.  

- **Advanced Web Interface**  
  A macOS-inspired web application that connects via USB, providing real-time data visualization and a suite of password-protected technician controls.  

- **Dynamic Configuration**  
  Technicians can remotely set new lifespan limits for each cartridge without needing to reprogram the device.  

---

## 3. How to Use (User & Technician Guide)  

### Prerequisites  
- NodeMCU ESP8266 (fully wired and programmed)  
- `serial_monitor.html` file saved on your computer  
- Compatible web browser (Google Chrome or Microsoft Edge)  

### Step-by-Step Instructions  

1. **Connect the Device**  
   Plug the ESP8266 into your computer via its USB port.  

2. **Open the Web Interface**  
   Open `serial_monitor.html` in your web browser.  

3. **Establish Connection**  
   - Click the **“Connect”** button in the top-right corner.  
   - Select the correct COM port for your ESP8266.  
   - Click **“Connect”** again.  
   - The status dot will turn **green** and live data will begin streaming.  

4. **Monitor Data**  
   - **Top Card:** Displays **Total Volume, Current Flow, and Highest Flow** in real-time.  
   - **Cartridge Status Section:** Shows usage, remaining life, and limit for each cartridge.  
     - Progress bar color codes:  
       - **Blue:** Normal  
       - **Yellow:** 90% used  
       - **Red:** 100% used  

5. **Technician Controls**  
   - **Reset Cartridge Usage**: Click *Reset Usage* → Enter admin credentials → Resets counter to zero.  
   - **Change Cartridge Limit**: Click *Change Limit* → Enter new limit → Confirm with admin credentials.  
   - **Hard Reset System**: Click the red button → Confirms with admin credentials → Wipes all data & restores defaults.  

---

## 4. Technical Details  

### Hardware Required  
- 1 × NodeMCU ESP8266 Development Board  
- 1 × YF-S201 Hall Effect Water Flow Sensor  
- 3 × Standard LEDs (e.g., 5mm)  
- 3 × Tactile Push Buttons  
- 3 × 220Ω Resistors (for LEDs)  
- 1 × External 5V Power Supply (for the flow sensor)  
- Breadboard & Jumper Wires  

### Wiring Diagram (Stable 3-Cartridge Prototype)  

| Component         | Pin on NodeMCU | Connection Details                                                |
|-------------------|----------------|-------------------------------------------------------------------|
| Flow Sensor       | D4             | Signal                                                            |
| LED 1             | D1             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| LED 2             | D2             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| LED 3             | D5             | Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND            |
| Button 1          | D6             | Pin → Button → GND (Uses internal pull-up)                        |
| Button 2          | D7             | Pin → Button → GND (Uses internal pull-up)                        |
| Button 3          | D3             | Pin → Button → GND (Uses internal pull-up)                        |

---

## 5. Version History (Changelog)  

- **v1.0: Initial Proof of Concept**  
  - Successfully read pulse data from YF-S201 sensor.  
  - Calibrated pulses into accurate volume measurement (Liters).  

- **v2.0: Stable Offline Controller**  
  - Implemented core logic for tracking total volume & cartridge usage.  
  - Added button support to reset counters.  
  - Integrated EEPROM storage for persistence.  
  - Status LEDs added with **warning (blinking)** and **replacement (solid)** states.  
  - Stable 3-cartridge pinout designed while preserving Serial Monitor debugging.  

- **v2.5 – v3.5: Web Serial Interface & Bug Fixes**  
  - Built standalone HTML web app using Web Serial API.  
  - JSON-based communication protocol between ESP8266 and UI.  
  - Full technician control suite (password-protected).  
  - Redesigned UI with intuitive buttons and dialogs (no command line).  

---
