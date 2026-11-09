# ESP32-C3 OLED Video Player

เล่นวิดีโอที่แปลงเป็นภาพขาวดำ 1-bit บน SSD1306 128×64 แบบวนซ้ำ โดยเก็บเฟรม
ใน Flash ของเฟิร์มแวร์ ไม่ใช้ microSD, Wi-Fi หรือการถอดรหัส MP4 บนบอร์ด
โปรเจกต์นี้แยกจากโปรเจกต์เกมเดิม

## เริ่มต้นได้ทันที

มี `include/video_frames.h` ของภาพเคลื่อนไหวตัวอย่างมาแล้ว: **50 เฟรม / 10 FPS / 5 วินาที**
ใช้ข้อมูลภาพ **51,200 ไบต์** ไม่ต้องมีไฟล์ MP4 ขณะ Build หรือขณะเล่นบนบอร์ด
ดูผลทดสอบจริงและขนาดโปรแกรมใน [TEST_RESULTS.md](TEST_RESULTS.md)

## การต่อสาย

ถอดไฟก่อนต่อสาย ตรวจชื่อขาบนโมดูลจริง ไม่อิงลำดับขาบนแถว header

| OLED | ESP32-C3 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO3 |
| SCL | GPIO2 |

เฟิร์มแวร์ตรวจ ACK ที่ `0x3C` แล้ว `0x3D` อัตโนมัติ หากไม่พบจอจะแจ้งทาง Serial
และหยุดเล่น ให้ตรวจสายแล้ว RESET การตอบ ACK ไม่สามารถยืนยันรุ่นชิปหรือภาพบนจอได้
โปรเจกต์นี้ไม่ใช้ปุ่มกด

## เครื่องมือ

- VS Code และส่วนเสริม PlatformIO IDE
- Python 3.9 ขึ้นไป ใช้เฉพาะ standard library ไม่ต้องติดตั้งแพ็กเกจ pip เพิ่ม
- FFmpeg 5.1 ขึ้นไป อยู่ใน PATH หรือระบุ `--ffmpeg "C:\path\ffmpeg.exe"`
- ไลบรารี Adafruit SSD1306 2.5.13 / GFX 1.11.11: PlatformIO ติดตั้งให้อัตโนมัติ

ตรวจสอบด้วย `python --version` และ `ffmpeg -version`
`requirements.txt` มีคำอธิบาย dependency และใช้ `python -m pip install -r requirements.txt` ได้
โดยไม่มีแพ็กเกจ Python เพิ่มเติมที่ต้องดาวน์โหลด

เครื่องที่ใช้สร้างโปรเจกต์นี้มี Python ที่ใช้งานได้อยู่แล้วที่:

```powershell
$pythonExe = '<USER_HOME>\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExe --version
& $pythonExe tools/mp4_to_oled.py videos/sample.mp4 --fps 10 --seconds 5
```

หากคำสั่ง `python` เปิด Microsoft Store ให้ใช้เส้นทางข้างต้นบนเครื่องนี้
หรือติดตั้ง Python และตั้ง PATH ก่อนใช้ตัวอย่างคำสั่งแบบ `python` ด้านล่าง

## แปลง MP4 หรือ GIF

เปิด Terminal ที่โฟลเดอร์ `ESP32_C3_Video`:

```powershell
python tools/mp4_to_oled.py videos/input.mp4 --fps 10 --seconds 5
python tools/mp4_to_oled.py videos/input.mp4 --fit crop --method ordered --fps 10 --seconds 5
python tools/mp4_to_oled.py videos/input.gif --fit letterbox --method threshold --threshold 128 --invert --seconds 3
```

| ตัวเลือก | ความหมาย |
| --- | --- |
| `--fps 10` | FPS เป้าหมาย ค่าเริ่มต้น 10 รองรับทศนิยม/อัตราส่วน เช่น 12.5 หรือ 30000/1001 |
| `--seconds 5` | จำกัดความยาวเอาต์พุต หากไม่ระบุจะอ่านทั้งคลิป |
| `--fit letterbox` | รักษาสัดส่วน เห็นภาพครบและเติมขอบดำ เป็นค่าเริ่มต้น |
| `--fit crop` | รักษาสัดส่วนและตัดขอบจากตรงกลางให้เต็ม 128×64 |
| `--method ordered` | Bayer 4×4 dithering เพื่อเลียนระดับเทาด้วยจุดขาวดำ เป็นค่าเริ่มต้น |
| `--method threshold` | เปรียบเทียบความสว่างกับ `--threshold` ช่วง 0–255 ค่าเริ่มต้น 128 |
| `--invert` | กลับขาว/ดำ รวมถึงขอบ Letterbox |
| `--output path.h` | กำหนดไฟล์ปลายทาง ค่าเริ่มต้นเป็น include/video_frames.h ของโปรเจกต์เสมอ |
| `--flash-budget 1310720` | ขนาดพาร์ทิชันแอป ไม่ใช่ Flash รวมทั้งชิป |
| `--firmware-reserve 327680` | เผื่อพื้นที่โค้ดและไลบรารีในการประมาณขนาด |
| `--strict-size` | ให้หยุดแปลงหากขนาดประมาณเกินงบ โดยรักษา header เดิม |

ผลลัพธ์เป็น `video_frames.h` และ `video_frames.json` ซึ่งบอกจำนวนเฟรม ขนาดข้อมูล
FPS ระยะเวลา และ SHA256 ของข้อมูลภาพ เมื่อแปลงล้มเหลว header เดิมจะไม่ถูกเขียนทับ
ไฟล์ GIF อ่านหนึ่งรอบโดยไม่ทำตาม loop ที่ฝังใน GIF เพราะเฟิร์มแวร์วนซ้ำให้อยู่แล้ว
วิดีโอไม่มีเสียงบน OLED และไม่มีการแปลงข้อมูลเสียง

## Flash และ RAM

หนึ่งเฟรมใช้ `128 × 64 ÷ 8 = 1,024` ไบต์ ที่ 10 FPS ใช้ 10,240 ไบต์ต่อวินาที
เฟรมเป็น `static const uint8_t ... PROGMEM` ใน Flash และคัดลอกทีละ 1,024 ไบต์
เข้า framebuffer ของ Adafruit โดยตรง ไม่เก็บวิดีโอทั้งชุดใน RAM
`display.begin()` ตรวจผลการจอง framebuffer หากไม่สำเร็จจะหยุดและแจ้งข้อผิดพลาด

บอร์ดโปรไฟล์นี้ระบุ Flash 4 MB แต่พาร์ทิชันแอปค่าเริ่มต้นรองรับ **1,310,720 ไบต์**
จึงไม่ควรใส่วิดีโอ 4 MB โดยตรง ตัวแปลงเตือนเมื่อข้อมูลภาพรวมพื้นที่เผื่อโค้ดเกินขนาดนี้
นี่เป็นการประมาณเท่านั้น ผล `pio run` เป็นตัวตรวจขนาดโปรแกรมจริง
หากเกินขนาดให้ลดความยาวหรือ FPS; โปรเจกต์นี้ไม่ได้เปลี่ยนพาร์ทิชันหรือโปรไฟล์บอร์ด

การจัดบิตตรงกับ `Adafruit_SSD1306`:

```text
byte_index = x + (y // 8) * 128
bit_mask   = 1 << (y % 8)
```

จุด (0,0) คือ bit 0 ของไบต์ 0, จุด (0,7) คือ bit 7 ของไบต์ 0,
จุด (0,8) คือ bit 0 ของไบต์ 128 และจุด (127,63) คือ bit 7 ของไบต์ 1023
นี่เป็น page buffer ไม่ใช่รูปแบบ bitmap ที่เรียงบิตตามแนวนอนของ `drawBitmap()`

## Build และเลือกโหมด Serial

เปิดโฟลเดอร์นี้ใน VS Code แล้วเลือก PlatformIO > Project Tasks > environment > Build
หรือใช้ PlatformIO Terminal:

```powershell
pio run
pio run -e esp32-c3 -e esp32-c3-usb
```

- `esp32-c3`: ค่าเริ่มต้น สำหรับ Serial ผ่าน USB-to-UART bridge
- `esp32-c3-usb`: Serial ผ่าน USB ของ ESP32-C3 โดยตรง

ทั้งสองใช้ `board = esp32-c3-devkitm-1`, Arduino และ GPIO3/GPIO2 เหมือนกัน
ต้องตรวจว่าบอร์ดจริงตรงกับโปรไฟล์ก่อน Upload หากเป็นรุ่นอื่นให้ยืนยันก่อนเปลี่ยนค่า
แพลตฟอร์มใช้ `espressif32` ตามคำขอ การ Build ครั้งนี้ใช้เวอร์ชัน 6.10.0 ที่ติดตั้งอยู่

## ความเร็ว I2C และ FPS

ใน `platformio.ini` เปลี่ยน `-D OLED_I2C_HZ=400000` เป็น `-D OLED_I2C_HZ=100000`
เพื่อใช้ 100 kHz ได้โดยไม่เปลี่ยนขา SDA/SCL แล้ว Build ใหม่
FPS ปกติตาม header ที่ตัวแปลงสร้าง หากต้องการเปลี่ยนเฉพาะความเร็วเล่นให้เพิ่ม
`-D PLAYBACK_FPS=10` ใน `build_flags` ของส่วน `[env]`
การเปลี่ยนเฉพาะความเร็วเล่นทำให้ความยาวคลิปเปลี่ยนตามสัดส่วน

ตัวกำหนดเวลาใช้ `millis()` พร้อมสะสมเศษมิลลิวินาที และชดเชยกรณีเฟรมล่าช้า
มี `delay(1)` สำหรับคืนเวลาให้ระบบเท่านั้น การส่งภาพผ่าน I2C ยังเป็นงานแบบบล็อก
ดังนั้น FPS จริงมีขีดจำกัดตามความเร็วบัส โดยเฉพาะ 100 kHz หากส่งไม่ทันจะข้ามเฟรม
เพื่อคงเวลาเล่นและรายงาน `skipped` ทาง Serial ควรเริ่มที่ 10 FPS และดูผลจริงบนบอร์ด

## Upload — ต้องยืนยันก่อน

ยังไม่ได้ Upload โปรเจกต์นี้ ตรวจรายการพอร์ตด้วย:

```powershell
pio device list
```

ถอด/เสียบสาย USB เพื่อระบุพอร์ตของบอร์ด ตรวจรุ่นบอร์ดและโหมด USB แล้วให้ผู้ใช้ยืนยันก่อน
คำสั่งต่อไปนี้เป็นตัวอย่าง **ต้องแทน COMx ด้วยพอร์ตที่ยืนยันแล้ว**:

```powershell
pio run -e esp32-c3 -t upload --upload-port COMx
pio device monitor -e esp32-c3 --port COMx --baud 115200
```

ถ้าใช้ USB โดยตรงให้เปลี่ยนชื่อ environment เป็น `esp32-c3-usb`
บนเครื่องนี้หากไม่มี `pio` ใน PATH ให้เปิด PlatformIO Terminal หรือใช้
`& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"` แทน `pio`
ปิดโปรแกรมที่จับพอร์ตไว้ก่อน Upload และตรวจพอร์ตใหม่หากเข้า download mode แล้ว COM เปลี่ยน

Serial แสดงจำนวนเฟรม ขนาดวิดีโอ Free Heap แอดเดรสจอ สถานะ `PLAYING`, FPS จริง,
FPS เป้าหมาย ลำดับเฟรมถัดไป และจำนวนเฟรมที่ข้ามในช่วงรายงานล่าสุด
เฟิร์มแวร์ไม่ตรวจการถอดจอหลังเริ่มเล่น และ `display()` ไม่คืนสถานะ ACK รายเฟรม
จึงต้องดูภาพบนฮาร์ดแวร์เพื่อยืนยันการเล่นจริง

## สร้างตัวอย่างและทดสอบซ้ำ

```powershell
python tools/make_sample.py
python tools/mp4_to_oled.py videos/sample.mp4 --fps 10 --seconds 5
python tools/test_converter.py
pio run -e esp32-c3 -e esp32-c3-usb
```

ชุดทดสอบใช้ FFmpeg จริงและสร้างไฟล์ชั่วคราว ตรวจจุดอ้างอิงบิต ขอบ page ทุกเฟรม 1,024 ไบต์
MP4/GIF, Crop/Letterbox, Ordered/Threshold, Invert, fractional FPS,
การเตือน Flash และการรักษา header เมื่อแปลงไฟล์เสียหรือเกินขนาดในโหมด strict

## โครงสร้าง

```text
ESP32_C3_Video/
  platformio.ini
  src/main.cpp
  include/video_frames.h
  include/video_frames.json
  tools/mp4_to_oled.py
  tools/make_sample.py
  tools/test_converter.py
  videos/sample.mp4
  videos/README.md
  requirements.txt
  README.md
  TEST_RESULTS.md
```
