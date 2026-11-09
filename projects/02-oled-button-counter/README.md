# ESP32-C3 + AA412 OLED 0.96 Blue

โปรเจกต์ PlatformIO / Arduino สำหรับจอ I2C SSD1306 128x64
แสดง `ESP32-C3`, `OLED TEST OK` และ uptime แบบวัน ชั่วโมง นาที วินาที
อัปเดตทุกวินาที พร้อมข้อความ Serial ที่ 115200 baud
เพิ่มตัวนับ `Presses:` บนจอและ Serial เมื่อตรวจพบการกดปุ่ม

## ปุ่มนับจำนวนครั้ง

- ใช้ปุ่มกดชั่วขณะแบบปกติเปิด (NO) ต่อระหว่าง **GPIO1 กับ GND**
- ใช้ `INPUT_PULLUP` ภายใน ไม่ต้องต่อตัวต้านทานดึงขึ้นเพิ่มสำหรับการทดลองด้วยสายสั้น
- กดหนึ่งครั้งเพิ่ม 1; กดค้างไม่นับซ้ำ ต้องปล่อยก่อนกดครั้งถัดไป
- กรองหน้าสัมผัสเด้ง 30 ms และแสดงค่าหลังยืนยันการกด
- เริ่มนับจาก 0 เมื่อเปิดเครื่องหรือกด RESET ไม่บันทึกค่าลงแฟลช
- ถ้าใช้ปุ่ม 4 ขา ให้เลือกขาสองฝั่งที่ไม่ต่อถึงกันตอนปล่อยปุ่ม
  ตรวจด้วยมิเตอร์หากไม่แน่ใจ เพราะขาบางคู่เชื่อมกันอยู่แล้วภายใน
- จอใช้ SDA=GPIO3, SCL=GPIO2 ตามโค้ดล่าสุดที่ผู้ใช้แก้ไว้

## สถานะการตรวจสอบ 7 พฤศจิกายน 2026

- สร้างไฟล์และเปิดโปรเจกต์ใน VS Code แล้ว; PlatformIO Core 6.2.0 พร้อมใช้งาน
- **โค้ดเพิ่มปุ่ม Build ผ่านแล้วทั้ง `esp32-c3` และ `esp32-c3-usb`**
  โดยใช้ PlatformIO และไลบรารีตาม `platformio.ini`
- พบ COM4 เป็น USB ของ Espressif (VID:PID 303A:1001) แต่ยังไม่ได้ยืนยันว่าเป็นบอร์ด ESP32-C3 เป้าหมาย
  หมายเลข COM อาจเปลี่ยนได้ อย่าเลือกโดยอิงบันทึกนี้เพียงอย่างเดียว
- การแก้ไขเพิ่มปุ่มรอบนี้ยังไม่ได้ Upload หรือทดสอบปุ่มกับบอร์ดจริง

## การต่อสาย

ถอดไฟก่อนต่อสาย และตรวจชื่อขาบนโมดูลจริง ไม่อิงลำดับขา

| ขาจอ OLED | ขาบอร์ด ESP32-C3 |
| --- | --- |
| VCC | 3V3 (3.3V) |
| GND | GND |
| SDA | GPIO3 |
| SCL | GPIO2 |

ใช้หมายเลข GPIO จริง ไม่ใช่ลำดับขาบนแถว header
โค้ดตรวจ ACK ที่ 0x3C แล้ว 0x3D; หากไม่พบ ให้ดูข้อความ Serial ตรวจสายแล้วกด RESET
การพบ ACK ยืนยันเพียงอุปกรณ์ I2C ตอบรับ ไม่ได้ยืนยันชิปหรือภาพบนจอ

## เลือก environment ให้ตรงบอร์ด

ค่าเริ่มต้นใช้ board profile `esp32-c3-devkitm-1` (แฟลช 4 MB)
นี่เป็นค่าตั้งต้นสำหรับ Build ต้องตรวจรุ่นและขนาดแฟลชของบอร์ดจริงก่อน Upload

- `esp32-c3`: Serial ผ่าน UART0 สำหรับบอร์ดที่ใช้ USB-to-UART bridge เช่น CP210x/CH340
- `esp32-c3-usb`: Serial ผ่าน USB ของ ESP32-C3 โดยตรง เปิด USB CDC on boot

เลือกจาก PlatformIO > Project Tasks > ชื่อ environment
หากต้องการเปลี่ยนค่าเริ่มต้น ให้แก้ `default_envs` ใน `platformio.ini`

## Build ใน VS Code

1. เปิดโฟลเดอร์นี้ใน VS Code และรอ PlatformIO เตรียมเครื่องมือเสร็จ
2. เลือก PlatformIO > Project Tasks > environment > General > Build
3. หรือกดเครื่องหมายถูก Build บนแถบ PlatformIO สำหรับ environment ที่เลือก

ใน PlatformIO Terminal ใช้:

```text
pio run -e esp32-c3
pio run -e esp32-c3-usb
```

## Upload เมื่อยืนยันบอร์ดและพอร์ตแล้วเท่านั้น

1. ต่อบอร์ดด้วยสาย USB ที่ส่งข้อมูลได้
2. เปิด PlatformIO > Devices หรือใช้ `pio device list`
3. เทียบรายการก่อนและหลังถอด/เสียบบอร์ด เพื่อระบุ COM ของบอร์ดจริง
4. ตรวจรุ่น ESP32-C3, ขนาดแฟลช และวิธีเชื่อมต่อ USB แล้วเลือก environment ให้ตรง
5. ใส่ `upload_port = COMx` และ `monitor_port = COMx` ใต้ environment นั้น โดยเปลี่ยน COMx เป็นพอร์ตที่ยืนยันแล้ว
6. กด Upload (ลูกศรขวา) ของ environment นั้น

ตัวอย่างคำสั่งต่อไปนี้มี placeholder ต้องเปลี่ยนก่อนใช้งาน:

```text
pio run -e esp32-c3 -t upload --upload-port COMx
pio device monitor --port COMx --baud 115200
```

ถ้าใช้ USB โดยตรง เปลี่ยน `-e esp32-c3` เป็น `-e esp32-c3-usb`
ถ้า Upload ค้างที่ Connecting ให้ทำตามวิธีเข้า download mode ของบอร์ดรุ่นนั้น
(หลายบอร์ดใช้กด BOOT ค้าง กด/ปล่อย RESET แล้วปล่อย BOOT)
พอร์ตอาจเปลี่ยนหลังเข้า download mode หรือหลัง Upload ให้ตรวจใหม่เสมอ

## Serial Monitor

เปิด PlatformIO > Project Tasks > environment > General > Monitor
หรือปุ่มปลั๊ก Serial Monitor หลังเลือกพอร์ตแล้ว ใช้ความเร็ว 115200 baud
กด RESET หากต้องการดูข้อความเริ่มต้น หากใช้ USB โดยตรงแต่ Serial เงียบ ให้ตรวจว่าเลือก `esp32-c3-usb`
ปิด Monitor ก่อน Upload หากมีโปรแกรมอื่นจับพอร์ตไว้

## ข้อจำกัดและแหล่งอ้างอิง

ยังไม่ได้ยืนยันการแสดงผลกับจอจริง การ Build ผ่านไม่ได้ยืนยันวงจรหรือชนิดจอ
AA412 ในโปรเจกต์นี้ถือเป็น SSD1306 128x64 ตามรายละเอียดที่ผู้ใช้ระบุ

- [PlatformIO ESP32-C3-DevKitM-1](https://docs.platformio.org/en/latest/boards/espressif32/esp32-c3-devkitm-1.html)
- [Adafruit SSD1306 source](https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp)
