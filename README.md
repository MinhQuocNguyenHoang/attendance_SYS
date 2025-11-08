1. Overview
    This is an IoT-based attendance system built using an ESP32 microcontroller.
    The device scans RFID cards to record attendance information (Card ID + Timestamp) and uploads the data in real time to Google Sheets for storage and monitoring.

    Future enhancements include integrating fingerprint authentication for additional security and flexibility.

2. Hardware Used
    MCU: ESP32 (WiFi + Bluetooth)
    Module RFID: RC522 RFID Module (giao tiếp SPI) 
    Module LCD: LCD 16x02 (giao tiếp bằng GPIO 4 bit qua driver HD44780)
    Nguồn cấp + mạch ổn áp
    Custom PCB thiết kế bằng Altium Designer

3. Software Environment
    Ngôn ngữ: Embedded C
    Framework: ESP‑IDF (version v5.4.1)
    RTOS: FreeRTOS real-time kernel
    Công cụ phát triển: VS Code, Altium Designer (cho phần PCB)
    Thư viện: abobija_rc522

4. Project Structure
    attendance_SYS/
    ┣ attendance_SYS_altium/        # Thiết kế PCB / bản vẽ mạch
    ┣ attendance_system/            # Code chính MCU
    ┣ README.md                     # File này
    ┣ demo_IOT_attendance_sys.mp4   # Video demo
    ┣ layout.pdf                    # Bố cục phần cứng
    ┗ schematic.pdf                  # Sơ đồ mạch

5. Features
    Scan RFID cards and verify card IDs
    Log attendance time (Real-time Clock or NTP sync)
    Upload attendance data to Google Sheets
    Wi-Fi Provisioning via BLE

6. Wiring / Schematic
    Full hardware schematics included in:
    [PCB Layout](./layout.pdf)
    [Schematic](./schematic.pdf)


7. Getting Started
    ESP-IDF installed
    A Google account with Google Sheets API setup and can change in app script of Google Sheet: https://docs.google.com/spreadsheets/d/1bOf_m6btDb6PRxpN6TObC4pNcN5I2Ge17Xd1aixpjCk/edit?usp=sharing
    USB cable to flash and monitor ESP32
    Build & Flash Example (ESP-IDF)
        git clone https://github.com/MinhQuocNguyenHoang/attendance_SYS.git
        cd attendance_SYS/attendance_system
        idf.py set-target esp32
        idf.py build
        idf.py flash monitor

8. Demo
    A demonstration video is included:
    [Demo Video](./demo_IOT_attendance_sys.mp4)

9. Testing
    Scan RFID card → Data should instantly appear in Google Sheets
    Test Wi-Fi Provisioning → Device must reconnect automatically

10. Roadmap / Future Improvements
    Add fingerprint sensor for secure attendance
    Retry & error handling to prevent data loss
    Build a mobile/web dashboard for data visualization
    Improve data security with message encryption

12. Author
    Nguyễn Hoàng Minh Quốc
    GitHub: @MinhQuocNguyenHoang
