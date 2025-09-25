Giới thiệu

Dự án Hệ thống điểm danh IoT được phát triển dựa trên ESP32 + RC522 RFID, cho phép:

Quét thẻ RFID để điểm danh.

Gửi dữ liệu trực tiếp lên Google Sheets thông qua HTTPS.

Quản lý dữ liệu điểm danh theo thời gian thực, an toàn và đáng tin cậy.

Ứng dụng: Quản lý lớp học, công ty, sự kiện hoặc bất kỳ hệ thống cần theo dõi sự có mặt của người dùng.

Phần cứng sử dụng

ESP32 (module WiFi + Bluetooth)

RC522 RFID Module (giao tiếp SPI)

Nguồn pin + mạch ổn áp

Custom PCB (thiết kế bằng Altium)

Phần mềm & Công nghệ

Ngôn ngữ: Embedded C (ESP-IDF)

Hệ điều hành: FreeRTOS (quản lý đa nhiệm)

Kết nối mạng: WiFi + HTTPS client

Cloud: Google Sheets API (RESTful)

Công cụ phát triển: ESP-IDF, VS Code, Altium Designer

Tính năng chính

Quét thẻ RFID và xác thực người dùng.

Gửi dữ liệu điểm danh (ID thẻ, thời gian) lên Google Sheets.

Cơ chế retry + error handling để đảm bảo dữ liệu không bị mất.

Deep Sleep tiết kiệm năng lượng, chỉ thức dậy khi có thẻ RFID.

Hỗ trợ WiFi Provisioning qua BLE: người dùng dùng app điện thoại để nhập SSID + Password, ESP32 tự động kết nối mà không cần sửa code. (ESP BLE Provisioning App trên CH play)

Cài đặt: clone repo, cài đặt các thư viện cần thiết, build và chạy thử, đi dây theo chân đã được định nghĩa trong code

Demo: 

Hướng phát triển: Tích hợp thêm cảm biến nhận dạng vân tay, quét QR thẻ để dễ sử dụng hơn trong môi trường trường học

Tác giả: Nguyễn Hoàng Minh Quốc
