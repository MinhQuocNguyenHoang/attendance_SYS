# IoT Attendance System

## Overview
This is an IoT-based attendance system built using an ESP32 microcontroller.  
The device scans RFID cards to record attendance information (Card ID + Timestamp) and uploads the data in real time to Google Sheets for storage and monitoring.

**Future enhancements:** integrating fingerprint authentication for additional security and flexibility.



## Hardware Used
- MCU: ESP32 (WiFi + Bluetooth)
- RFID Module: RC522 (SPI)
- LCD: 16x02 via GPIO 4-bit (HD44780 driver)
- Power supply + voltage regulator
- Custom PCB designed with Altium Designer



## Software Environment
- Language: Embedded C
- Framework: ESP-IDF v5.4.1
- RTOS: FreeRTOS real-time kernel
- Tools: VS Code, Altium Designer (PCB design)
- Library: abobija_rc522



## Project Structure
```
    attendance_SYS/
    ├─ attendance_SYS_altium/ # PCB design / schematic
    ├─ attendance_system/ # MCU code
    ├─ README.md # This file
    ├─ demo_IOT_attendance_sys.mp4 # Demo video
    ├─ layout.pdf # PCB layout
    └─ schematic.pdf # Schematic
```

## Features
- Scan RFID cards and verify card IDs
- Log attendance time (RTC or NTP sync)
- Upload attendance data to Google Sheets
- Wi-Fi provisioning via BLE


## Wiring / Schematic
- [PCB Layout](./layout.pdf) 
- [Schematic](./schematic.pdf) 


## Getting Started
1. Install ESP-IDF 
2. Setup Google Sheets API: [Google Sheet](https://docs.google.com/spreadsheets/d/ 1bOf_m6btDb6PRxpN6TObC4pNcN5I2Ge17Xd1aixpjCk/edit?usp=sharing) 
3. Connect USB cable to flash and monitor ESP32 

```bash
git clone https://github.com/MinhQuocNguyenHoang/attendance_SYS.git
cd attendance_SYS/attendance_system
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

## Demo
A demonstration video is included:
[Demo Video](./demo_IOT_attendance_sys.mp4)

## Testing
Scan RFID card → Data should instantly appear in Google Sheets 
Test Wi-Fi Provisioning → Device must reconnect automatically

## Roadmap / Future Improvements
Add fingerprint sensor for secure attendance 
Retry & error handling to prevent data loss 
Build a mobile/web dashboard for data visualization 
Improve data security with message encryption 

## Author
Nguyễn Hoàng Minh Quốc<br>
GitHub: @MinhQuocNguyenHoang
