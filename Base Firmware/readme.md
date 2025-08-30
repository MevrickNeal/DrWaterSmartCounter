Dr. Water - Intelligent Cartridge Monitoring System
Version: 2.2

Platform: Arduino Uno

Author: Lian Mollick

Date: August 31, 2025

1. Project Synopsis
This project provides the firmware for an intelligent monitoring system designed for Dr. Water multi-stage purifiers. The system solves a critical customer pain point by accurately tracking the real-time usage of each individual filter cartridge and providing clear, predictive alerts for replacement.

Instead of relying on simple timers, this firmware uses a water flow sensor to measure the exact volume of water processed, ensuring that cartridges are replaced based on actual usage. The system also includes a secure, password-protected interface for technicians to perform maintenance, such as resetting individual cartridge counters or performing a full system reset.

2. Key Features
Precision Volume Tracking: Measures the total cumulative volume of filtered water with floating-point accuracy.

Real-time Flow Metrics: Monitors and displays both the current flow rate (L/min) and the highest-ever recorded rate.

Multi-Cartridge Lifespan Management: Independently tracks the usage and remaining life for up to seven different filter cartridges, each with a unique lifespan.

Predictive Alerts: Provides two-stage status indicators for each cartridge:

Warning: When a cartridge reaches 90% of its lifespan.

REPLACE: When a cartridge has reached its full rated capacity.

Persistent Data Storage: All system data (total volume, reset points, max speed) is saved to the Arduino's EEPROM, making it immune to power loss or system reboots.

Secure Technician Interface: A password-protected command system accessible via the Serial Monitor for authorized maintenance.

Cartridge Reset (c=N): Resets the counter for a specific cartridge after replacement without affecting others.

Hard Reset (h): Wipes all data and restores the system to its initial factory state.

3. Hardware Requirements
To build the prototype, the following components are required:

Microcontroller: Arduino Uno

Sensor: YF-S201 Hall Effect Water Flow Sensor

Power Supply: 5V DC Power Adapter (1A recommended)

Connectivity: USB-A to USB-B cable for programming and serial monitoring

4. Firmware Installation & Setup
Open the Firmware: Launch the Arduino IDE and open the water_purifier_firmware.ino file.

Select Board: Navigate to Tools > Board > Arduino AVR Boards and select "Arduino Uno".

Select Port: Go to Tools > Port and choose the COM port corresponding to your connected Arduino.

Upload: Click the "Upload" button (→) to flash the firmware onto the Arduino Uno.

5. System Operation & Technician Commands
Standard Monitoring
After uploading the firmware, open the Serial Monitor (Tools > Serial Monitor).

Set the baud rate to 9600.

The system will boot up and, every second, print a detailed live status report of the entire system and each cartridge.

Technician Maintenance Commands
To perform maintenance, type one of the following commands into the input field of the Serial Monitor and press Enter.

Command

Action

h

Initiate Hard Reset: Wipes all system data.

c=N

Reset Cartridge: Resets the counter for a specific cartridge N (e.g., c=1 for Cartridge 1, c=5 for Cartridge 5).

Upon entering a command, the system will prompt for authentication.

User ID: drwtr01

Password: 1234

Authentication is required for both hard resets and individual cartridge resets. An incorrect login will abort the operation.
