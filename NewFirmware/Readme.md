Dr. Water Smart Cartridge Monitor
Version: 2.9 (Stable Offline Controller with Web Serial Interface)

Lead Developer: Lian Mollick

1. Project Overview
The Dr. Water Smart Cartridge Monitor is a prototype for an intelligent, low-cost device that accurately tracks the lifespan of water filter cartridges based on actual water volume. This system replaces simple timers with a precise, usage-based measurement, providing users with timely alerts for replacement and technicians with a powerful tool for configuration and maintenance.

This version is a fully functional offline controller that uses a Web Serial API-based interface for real-time monitoring and administration.

2. Key Features
Real-Time Flow Tracking: Accurately measures total water volume, current flow rate, and peak flow rate.

Multi-Cartridge Support: Tracks the independent lifespan of multiple filter cartridges (currently configured for 3).

Persistent Memory: All data is saved to non-volatile memory (EEPROM), ensuring no data loss during power outages.

Physical Controls: Supports hardware buttons for technicians to reset cartridge counters in the field.

Advanced Web Interface: A beautiful, macOS-inspired web application that connects via USB to provide real-time data visualization and a full suite of password-protected technician controls.

Dynamic Configuration: Technicians can remotely set new lifespan limits for each cartridge without needing to re-program the device.

3. How to Use (User & Technician Guide)
This guide explains how to operate the device using the Web Serial monitoring tool.

Prerequisites:
The ESP8266 device, fully wired and programmed.

The serial_monitor.html file saved on your computer.

A compatible web browser (Google Chrome or Microsoft Edge).

Step-by-Step Instructions:
Connect the Device: Plug the ESP8266 into your computer via its USB port.

Open the Web Interface: Open the serial_monitor.html file in your web browser.

Establish Connection:

Click the "Connect" button in the top-right corner.

A dialogue box will appear listing available serial ports. Select the port corresponding to your ESP8266 and click "Connect".

The status dot will turn green, and the page will begin streaming live data from the device.


Monitor Data:

The top card displays the Total Volume, Current Flow, and Highest Flow in real-time.

The "Cartridge Status" section shows the usage, remaining life, and current limit for each of the 3 cartridges. The progress bar will change color from blue to yellow (at 90% used) and red (at 100% used).

Technician Controls:

Reset Cartridge Usage: Click the "Reset Usage" button on the specific cartridge card. A dialogue box will appear asking for admin credentials. Upon successful login, the usage for that cartridge will be reset to zero.

Change Cartridge Limit: Click the "Change Limit" button. A dialogue box will appear asking for the new limit. After you save, a second dialogue will ask for admin credentials to confirm the change.

Hard Reset System: Click the red "Hard Reset System" button. This will wipe all data on the device (total volume, limits, etc.) and reset it to its factory defaults after you provide the admin credentials.

4. Technical Details
Hardware Required:
1 x NodeMCU ESP8266 Development Board

1 x YF-S201 Hall Effect Water Flow Sensor

3 x Standard LEDs (e.g., 5mm)

3 x Tactile Push Buttons

3 x 220Ω Resistors (for LEDs)

1 x External 5V Power Supply (for the flow sensor)

Breadboard and Jumper Wires

Wiring Diagram (Stable 3-Cartridge Prototype):
Component

Pin on NodeMCU

Connection Details

Flow Sensor (Signal)

D4



LED 1

D1

Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND

LED 2

D2

Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND

LED 3

D5

Pin → 220Ω Resistor → LED Anode (+), Cathode (-) → GND

Button 1

D6

Pin → Button → GND (Uses internal pull-up)

Button 2

D7

Pin → Button → GND (Uses internal pull-up)

Button 3

D3

Pin → Button → GND (Uses internal pull-up)

5. Version History (Changelog)
v1.0: Initial Proof of Concept

Successfully read pulse data from the YF-S201 sensor.

Calibrated the sensor to convert pulses into an accurate volume measurement (Liters).

v2.0: Stable Offline Controller

Implemented core logic to track total volume and individual cartridge usage.

Added support for physical buttons to reset cartridge counters.

Integrated non-volatile EEPROM storage to save all data through power cycles.

Added status LEDs with warning (blinking) and replacement (solid) states.

Established a stable 3-cartridge pinout that preserves Serial Monitor functionality for debugging.

v2.5 - v3.5: Web Serial Interface & Bug Fixes

Developed a standalone HTML web application to connect to the device using the Web Serial API.

Implemented a JSON-based data protocol for communication between the ESP8266 and the web UI.

Added a full suite of password-protected technician controls to the web UI.

Redesigned the UI to be more intuitive, replacing the command line with user-friendly buttons and dialogue boxes.
