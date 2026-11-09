# ESP32-C3 Display Lab

รวมการทดลอง ESP32-C3 + Arduino + PlatformIO ตั้งแต่ทดสอบ OLED, ปุ่มนับครั้ง, เกมไดโนเสาร์ ไปจนถึงวิดีโอจอสี ST7735S เก็บเฟรมใน Flash ไม่ใช้ microSD หรือ Wi-Fi

**เวอร์ชันล่าสุด:** [07-st7735-spinning-cat](projects/07-st7735-spinning-cat) เล่นแมวหมุน 3 วินาทีวนซ้ำ โดยตั้ง `TFT_INVERT=0` ตามค่าที่เจ้าของบอร์ดยืนยันว่าสีถูกต้องแล้ว

## โปรเจกต์แต่ละช่วง

| โฟลเดอร์ | รายละเอียด |
| --- | --- |
| [01-oled-test](projects/01-oled-test) | ESP32-C3 / OLED TEST OK / uptime — สร้างใหม่ตามคำขอแรก ไม่ใช่สำเนาต้นฉบับ |
| [02-oled-button-counter](projects/02-oled-button-counter) | กดปุ่มแล้วเพิ่มจำนวนครั้ง จากสำเนาที่บันทึกไว้ก่อนเปลี่ยนเป็นเกม |
| [03-oled-dino](projects/03-oled-dino) | กดสองครั้งเริ่มเกม กดเพื่อกระโดด ชนแล้วกดหนึ่งครั้งเริ่มใหม่ |
| [04-oled-video](projects/04-oled-video) | SSD1306 128×64, 1-bit, ตัวแปลง MP4/GIF และภาพเคลื่อนไหวตัวอย่าง |
| [05-st7735-video](projects/05-st7735-video) | วิดีโอสีตัวอย่าง RGB565 จากโค้ด/เฟรมที่สำรองไว้ก่อนเพิ่มปุ่มและคลิปแมว |
| [06-st7735-color-test](projects/06-st7735-color-test) | ปุ่ม GPIO1 เปลี่ยนสีเต็มจอ ทดสอบแดง เขียว น้ำเงิน ขาว ดำ เหลือง ฟ้า ม่วง |
| [07-st7735-spinning-cat](projects/07-st7735-spinning-cat) | OIIA OIIA (Spinning Cat), 160×80, 10 FPS, 30 เฟรม ไม่มีเสียง |

เปิดโฟลเดอร์โปรเจกต์ที่ต้องการใน VS Code ทีละโปรเจกต์ ทุกโฟลเดอร์มี `platformio.ini` ของตนเอง

## สายจอและปุ่ม

OLED SSD1306 ใช้ I2C: โปรเจกต์ 01 ใช้ SDA **GPIO4**, SCL **GPIO5** ตามคำขอแรก ส่วนโปรเจกต์ 02–04 ใช้ SDA **GPIO3**, SCL **GPIO2** ตามการต่อภายหลัง ไฟ **3.3V**, GND ร่วม ตรวจหา address 0x3C/0x3D

ST7735S IPS 80×160 ใช้ SPI แม้ขาบนโมดูลเขียนว่า SCL/SDA:

| ขาจอ | ESP32-C3 |
| --- | --- |
| GND | GND |
| VCC | 3.3V |
| SCL | GPIO2 (SCK) |
| SDA | GPIO3 (MOSI) |
| RES | GPIO4 |
| DC | GPIO5 |
| CS | GPIO7 |
| BLK | 3.3V |

ปุ่มใช้ GPIO1 ต่อกับ GND แบบปกติเปิด ใช้ INPUT_PULLUP ภายใน ถอดไฟก่อนเปลี่ยนสาย

## Build / Upload / Serial Monitor

ติดตั้ง VS Code + PlatformIO แล้วเปิดโฟลเดอร์ที่มี `platformio.ini` หรือใช้ PlatformIO terminal:

```powershell
cd projects/07-st7735-spinning-cat
pio run -e esp32-c3
pio device list
# ตรวจสอบบอร์ดและแทน COMx ด้วยพอร์ตจริงก่อนรันคำสั่ง Upload
pio run -e esp32-c3 -t upload --upload-port COMx
pio device monitor --port COMx --baud 115200
```

มี environment `esp32-c3-usb` สำหรับบอร์ดที่ต่อ USB เข้าชิปโดยตรง ให้เลือกตามบอร์ดจริง ค่า board เริ่มต้นคือ `esp32-c3-devkitm-1` ไม่มีการ Upload อัตโนมัติเมื่อ Build

## เปลี่ยนวิดีโอ

ต้องมี Python 3 และ FFmpeg ใน PATH แปลง MP4/GIF บนคอมพิวเตอร์ก่อน แล้ว Build และ Upload เฟิร์มแวร์ใหม่ ไม่ได้คัดลอก MP4 เข้า ESP32 โดยตรง

```powershell
# จากโฟลเดอร์โปรเจกต์ 07 (จอสี)
python tools/mp4_to_tft.py videos/input.mp4 --fps 10 --seconds 3 --fit crop
pio run -e esp32-c3

# จากโฟลเดอร์โปรเจกต์ 04 (OLED)
python tools/mp4_to_oled.py videos/input.mp4 --fps 10 --seconds 5
pio run -e esp32-c3
```

เฟรม OLED ใช้ 1,024 ไบต์ ส่วนจอสี 160×80 RGB565 ใช้ 25,600 ไบต์ต่อเฟรม คลิปแมวในโปรเจกต์ใช้เฉพาะ 3 วินาทีแรก รวม 768,000 ไบต์ข้อมูลเฟรม ไม่ใช่คลิปเต็ม และไม่มีเสียง

เก็บไฟล์วิดีโอต้นฉบับที่ใช้แปลงไว้ที่ [media/oiia-spinning-cat-original.mp4](media/oiia-spinning-cat-original.mp4) ด้วย ไฟล์นี้สำหรับเปิดหรือแปลงบนคอมพิวเตอร์ ไม่ได้ฝังลงเฟิร์มแวร์ทั้งไฟล์ รูปโมดูลที่ผู้ใช้ส่งอยู่ที่ [docs/st7735s-module.jpg](docs/st7735s-module.jpg)

## ที่มาและผลทดสอบ

รวมไฟล์จาก workspace เดิมและสำเนาที่มีอยู่จริง ไม่ได้สร้างประวัติ commit ย้อนหลังขึ้นมา โฟลเดอร์ 01 เป็นตัวอย่างสร้างใหม่ตามคำขอเดิม; 05 และ 06 จัดเป็นโปรเจกต์แยกจากสำเนาโค้ด/โหมดที่บันทึกไว้ พร้อมใช้ค่า inversion ที่แก้แล้ว

ดู [ผลตรวจสอบรอบเผยแพร่](VALIDATION.md) และ [ประวัติไฟล์](history/README.md) ผล Build เก่าถูกเก็บแยกจากผลตรวจสอบรอบนี้ การยืนยันว่าสีถูกต้องมาจากเจ้าของบอร์ด ไม่มีการ Upload บอร์ดในรอบเผยแพร่นี้

ไม่รวม cache, toolchain, `.pio`, virtual environment และค่าตั้ง IDE ที่ผูกกับเครื่อง ไฟล์สื่อของบุคคลที่สามยังเป็นของเจ้าของสิทธิ์เดิม คลังนี้ไม่ได้ให้สิทธิ์เพิ่มเติมเหนือไฟล์เหล่านั้น
