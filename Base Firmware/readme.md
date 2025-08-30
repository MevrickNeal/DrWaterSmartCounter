Dr. Water - Intelligent Cartridge Monitoring System
Overview
This repository contains the Arduino firmware for an intelligent water purifier monitoring system. The system accurately tracks water usage to determine the remaining lifespan of up to seven individual filter cartridges. It provides real-time status updates via the Serial Monitor and includes a password-protected interface for technician maintenance, ensuring data integrity and reliable service.

All critical data, including total water volume and individual cartridge reset points, is persistently stored in the Arduino's EEPROM, making the system resilient to power failures.

Features
Real-Time Flow Tracking: Measures both the current flow rate (Liters/min) and the total cumulative volume of water processed (Liters).

Multi-Cartridge Monitoring: Independently tracks the lifecycle of 7 separate filter cartridges, each with a configurable lifespan.

Persistent Memory: All data is saved to EEPROM, ensuring no data loss during power outages or system resets.

Advanced Metrics: Records and retains the highest flow rate ever achieved by the system.

Smart Alerts: Provides three levels of status for each cartridge: OK, Warning (at 90% usage), and REPLACE.

First-Run Initialization: Automatically detects if the device is being run for the first time and initializes all counters to zero, preventing issues from random EEPROM data.

Technician Service Menu: A secure, password-protected command interface accessible via the Serial Monitor for system maintenance.

Hardware Requirements
Arduino Uno (or a compatible board)

YF-S201 Hall Effect Water Flow Sensor

5V DC Power Supply

Setup and Installation
Hardware Connection:

Connect the Red wire (VCC) of the flow sensor to the 5V pin on the Arduino.

Connect the Black wire (GND) of the flow sensor to a GND pin on the Arduino.

Connect the Yellow wire (Signal) of the flow sensor to Digital Pin 2 on the Arduino. This pin is required as it supports interrupts.

Firmware Upload:

Open the water_purifier_firmware.ino file in the Arduino IDE.

Go to Tools > Board and select "Arduino Uno".

Go to Tools > Port and select the correct COM port for your board.

Click the Upload button.

Monitoring:

Once the firmware is uploaded, open the Serial Monitor (Tools > Serial Monitor).

Set the baud rate to 9600. The system will begin printing its live status report every second.

Technician Commands
To access the service menu, enter commands into the Serial Monitor and press Enter. You will be prompted for authentication.

Default User ID: drwtr01

Default Password: 1234

Commands
Hard Reset (h):

Wipes all data from the EEPROM and resets the system to its factory state. This includes the total volume counter, all cartridge reset points, and the highest recorded speed.

Usage: Type h and press Enter. Follow the authentication prompts.

Cartridge Reset (c=N):
