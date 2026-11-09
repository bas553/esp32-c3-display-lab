# ST7735S video sample

กู้คืนจากสำเนาโค้ดก่อนเพิ่มปุ่มทดสอบสี และเฟรมตัวอย่างก่อนเปลี่ยนเป็นคลิปแมว
ตั้ง `TFT_INVERT=0` ตามค่าที่แก้สีแล้วบนจอของเจ้าของโปรเจกต์

ใช้ SPI: SCK2, MOSI3, RES4, DC5, CS7; VCC/BLK 3.3V และ GND ร่วม
Build: `pio run -e esp32-c3`

ตัวแปลง: `python tools/mp4_to_tft.py videos/input.mp4 --fps 10 --seconds 3 --fit crop`
รายละเอียดการต่อสายและ Upload อยู่ใน README หลัก
