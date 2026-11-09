# OLED test (reconstructed)

ตัวอย่างสร้างใหม่ตามคำขอเริ่มต้น เพราะไม่พบสำเนาโค้ดช่วงแรกแยกจากตัวนับปุ่ม/เกม
แสดง ESP32-C3, OLED TEST OK และ uptime บน SSD1306 128×64

ต่อ SDA GPIO4, SCL GPIO5, VCC 3.3V และ GND ตามการต่อเริ่มแรก
โครงการภายหลังเปลี่ยนเป็น SDA3/SCL2; ตรวจสายก่อนใช้ตัวอย่างนี้

Build: `pio run -e esp32-c3` ดูคำสั่ง Upload/Monitor ใน README หลัก
