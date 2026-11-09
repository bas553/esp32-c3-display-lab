# Button color test

แยกจากโหมด `TFT_BUTTON_TEST=1` ของโปรเจกต์จอสี ไม่ต้องใช้ไฟล์เฟรมวิดีโอ
ต่อปุ่ม GPIO1 กับ GND กดครั้งละสี: แดง เขียว น้ำเงิน ขาว ดำ เหลือง ฟ้า ม่วง
ตั้ง `TFT_INVERT=0` ตามค่าที่เจ้าของบอร์ดทดสอบแล้ว

ใช้ SPI: SCK2, MOSI3, RES4, DC5, CS7; VCC/BLK 3.3V และ GND ร่วม
Build: `pio run -e esp32-c3` ดู Upload/Monitor ใน README หลัก
