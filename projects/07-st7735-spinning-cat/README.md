# ESP32-C3 + ST7735S IPS 80×160 — Video Player

## ค่าสีล่าสุด

ตั้ง TFT_INVERT=0 แล้ว เจ้าของบอร์ดยืนยันว่าสีถูกต้องหลังแก้ค่านี้

## โหมดปัจจุบัน: OIIA OIIA (Spinning Cat)

ตั้ง `TFT_BUTTON_TEST=0` เพื่อเล่น 3 วินาทีแรกจากไฟล์ของผู้ใช้
`W&W - OIIA OIIA (Spinning Cat).mp4` แบบวนซ้ำ ไม่มีเสียง
แปลงเป็น 160×80 RGB565, Crop, 10 FPS, 30 เฟรม รวม 768,000 ไบต์
ไฟล์ต้นฉบับยาวประมาณ 134.73 วินาที ไม่ได้ใส่ทั้งคลิป เพราะพื้นที่แอปไม่พอ
มีคลิปย่อสำหรับดูบนคอมพิวเตอร์ที่ `videos/oiia-cat-3s.mp4`

## โหมดเสริม: กดปุ่มเปลี่ยนสีทดสอบจอ

หากต้องการใช้โหมดทดสอบสี ให้ตั้ง `TFT_BUTTON_TEST=1` ใน `platformio.ini` แล้ว Build ใหม่
ต่อปุ่มกดแบบปกติเปิด (NO) ระหว่าง **GPIO1 กับ GND** ใช้ `INPUT_PULLUP` ภายใน
เปิดเครื่องเริ่มสีแดง แล้วกดหนึ่งครั้งเปลี่ยนสีตามลำดับ:

**แดง → เขียว → น้ำเงิน → ขาว → ดำ → เหลือง → ฟ้า (Cyan) → ม่วง (Magenta) → แดง**

แสดงสีเต็มจอ ไม่มีตัวอักษรทับภาพ เพื่อดูพิกเซลและความสม่ำเสมอของสี
ชื่อสีแสดงทาง Serial Monitor 115200 baud กดค้างไม่เปลี่ยนซ้ำ ต้องปล่อยก่อนกดใหม่
กรองหน้าสัมผัสเด้ง 30 ms; รีเซ็ตแล้วเริ่มสีแดงใหม่
ถ้าเป็นปุ่ม 4 ขา ให้เลือกคู่ขาที่ไม่ต่อถึงกันตอนปล่อยปุ่ม

สายจอยังคงใช้ตารางด้านล่าง โหมดนี้ไม่ฝังข้อมูลวิดีโอลงเฟิร์มแวร์
หากต้องการกลับไปเล่นวิดีโอตามคู่มือส่วนที่เหลือ ให้เปลี่ยน `TFT_BUTTON_TEST=0` แล้ว Build ใหม่
ยังต้องยืนยันบอร์ดและพอร์ตก่อน Upload

โปรเจกต์สำหรับจอสีในภาพผู้ใช้ ขาเรียง `GND VCC SCL SDA RES DC CS BLK`
ใช้ชิป ST7735S ตามข้อมูลที่ผู้ใช้ระบุ เล่นวิดีโอสี RGB565 ในแนวนอน **160×80**
เก็บภาพใน Flash โดยตรง ไม่ใช้ microSD และไม่ใช้ Wi-Fi
รูปถ่ายยืนยันชื่อขา แต่ยังยืนยัน revision ของแผงจอ/ค่า offset และสีจริงไม่ได้

## ต่อสายตามโปรเจกต์นี้

ถอดไฟก่อนต่อสาย ใช้หมายเลข GPIO จริงบนบอร์ด ESP32-C3

| ขาบนจอ (ซ้ายไปขวาตามภาพ) | ต่อเข้าบอร์ด | หน้าที่ |
| --- | --- | --- |
| GND | GND | กราวด์ร่วม |
| VCC | 3.3V | ไฟเลี้ยง |
| SCL | GPIO2 | SPI Clock / SCK |
| SDA | GPIO3 | SPI Data / MOSI |
| RES | GPIO4 | Reset จอ |
| DC | GPIO5 | เลือกคำสั่ง/ข้อมูล |
| CS | GPIO7 | Chip Select |
| BLK | 3.3V | เปิดไฟแบ็กไลต์ตลอด |

SCL/SDA ของจอนี้เป็น **SPI** ไม่มี I2C address `0x3C/0x3D`
คง GPIO3/GPIO2 ตามงานเดิม แต่เปลี่ยนโปรโตคอลจาก I2C เป็น SPI
ไม่มีขา MISO ในการต่อแบบนี้ จึงอ่านรหัสชิปหรือตรวจว่าจอต่ออยู่โดยอัตโนมัติไม่ได้
GPIO1 ใช้สำหรับปุ่มเปลี่ยนสีในโหมด `TFT_BUTTON_TEST=1`

## เปิดเครื่องในโหมดวิดีโอ (`TFT_BUTTON_TEST=0`)

1. แสดงกรอบขาวและแถบสี **แดง เขียว น้ำเงิน ขาว (R/G/B/W)** ประมาณ 2.5 วินาที
2. เล่นคลิปจาก `video_frames.h` ซึ่งปัจจุบันเป็นแมวหมุน 3 วินาที ที่ 10 FPS วนซ้ำ
3. Serial 115200 แสดงสถานะ จำนวนเฟรม ขนาด Flash, FPS ที่ส่งได้จริง และเฟรมที่ข้าม

มี `include/video_frames.h` พร้อมใช้แล้ว จึง Build ได้โดยไม่ต้องแปลงไฟล์เอง
โปรแกรมส่งภาพทีละแถว ใช้บัฟเฟอร์ภาพ **320 ไบต์** ใน RAM
คลิปตัวอย่างมี 30 เฟรม รวม **768,000 ไบต์** อยู่ใน Flash

## Build / Upload

เปิดโฟลเดอร์นี้ใน VS Code แล้วเลือก PlatformIO > Project Tasks > environment > Build
หรือเปิด PlatformIO Terminal ที่โฟลเดอร์นี้:

```powershell
pio run -e esp32-c3 -e esp32-c3-usb
pio device list
```

- `esp32-c3`: Serial ผ่านชิป USB-to-UART ภายนอก
- `esp32-c3-usb`: Serial ผ่าน USB ของ ESP32-C3 โดยตรง
- โปรไฟล์บอร์ดยังคง `esp32-c3-devkitm-1`, Flash 4 MB, พาร์ทิชันแอปเริ่มต้น 1,310,720 ไบต์

**ยังไม่ได้ Upload โปรเจกต์นี้ ต้องให้ผู้ใช้ยืนยันบอร์ดและพอร์ตก่อน**
ตัวอย่างหลังยืนยันแล้ว ให้แทน `COMx` ด้วยพอร์ตจริงและเลือก environment ให้ตรงบอร์ด:

```powershell
pio run -e esp32-c3-usb -t upload --upload-port COMx
pio device monitor -e esp32-c3-usb --port COMx --baud 115200
```

หากไม่พบคำสั่ง `pio` ให้ใช้ PlatformIO Terminal หรือแทนด้วย:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
```

## เปลี่ยนวิดีโอ

ใช้ Python 3.9+ และ FFmpeg 5.1+ ไม่ต้องติดตั้งแพ็กเกจ pip เพิ่ม
วาง MP4/GIF ไว้ใน `videos/` แล้วเรียก:

```powershell
python tools/mp4_to_tft.py videos/input.mp4 --fps 10 --seconds 3 --strict-size
python tools/mp4_to_tft.py videos/input.gif --fps 5 --seconds 5 --fit crop --strict-size
```

บนเครื่องนี้มี Python ที่ใช้ทดสอบจริงอยู่ที่:

```powershell
$pythonExe = '<USER_HOME>\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExe tools/mp4_to_tft.py videos/input.mp4 --fps 10 --seconds 3 --strict-size
```

ตัวแปลงรักษาอัตราส่วนภาพ รองรับ `--fit letterbox` (ค่าเริ่มต้น) หรือ `--fit crop`
รองรับ `--invert`, `--swap-rb`, `--output path.h`, `--ffmpeg "C:\path\ffmpeg.exe"`
และ FPS แบบเศษส่วน/ทศนิยม สร้าง `include/video_frames.h` กับ metadata `.json`
อ่าน GIF หนึ่งรอบและทิ้งเสียงทุกชนิด เมื่อแปลงเสร็จให้ Build และ Upload ใหม่
ไม่มีการส่งไฟล์ MP4 ตรงไปยังบอร์ด

## พื้นที่ Flash

RGB565 ใช้ 2 ไบต์ต่อพิกเซล: `160 × 80 × 2 = 25,600` ไบต์ต่อเฟรม
ที่ 10 FPS ใช้ 256,000 ไบต์ต่อวินาที วิดีโอ 5 วินาทีจะใช้ 1,280,000 ไบต์
เฉพาะภาพ จึงไม่พอรวมโค้ดในพาร์ทิชันเริ่มต้น 1,310,720 ไบต์
เริ่มด้วย 3 วินาที/10 FPS หรือ 5 วินาที/5 FPS แล้วตรวจขนาดจาก Build

ตัวแปลงเผื่อโค้ด 327,680 ไบต์ ปรับได้ด้วย `--firmware-reserve`
และกำหนดขนาดแอปด้วย `--flash-budget` (ไม่ใช่ขนาด Flash ทั้งชิป)
`--strict-size` หยุดก่อนเขียนทับ header หากขนาดประมาณเกินงบ
หากไม่ใช้ strict จะเตือนและสร้างไฟล์ต่อ แต่ Build อาจไม่ผ่านเพราะขนาดเกิน
ผล Build เป็นหลักฐานขนาดสุดท้าย ไม่ได้เพิ่มพาร์ทิชันเพื่อให้ใส่คลิปยาวขึ้นโดยอัตโนมัติ

## ปรับให้ตรงจอแต่ละล็อต

ตั้งต้นด้วยไลบรารี Adafruit, `INITR_MINI160x80`, landscape rotation 1,
inversion เปิด และ SPI 8 MHz ค่าพวกนี้เป็นค่าเริ่มทดสอบ ไม่ใช่ผลยืนยันบนแผงจริง
แก้ `build_flags` ใน `[env]` ของ `platformio.ini` แล้ว Build/Upload ใหม่:

| อาการ/สิ่งที่ต้องการ | ค่าให้ปรับ |
| --- | --- |
| ค้างหน้าแถบสีเพื่อตรวจสาย | `TFT_TEST_ONLY=1` |
| ภาพกลับหัว 180 องศา | `TFT_ROTATION=3` (ใช้ 1 หรือ 3 สำหรับคลิป 160×80) |
| สีแดงกับน้ำเงินสลับกัน | `TFT_SWAP_RB=1` |
| สีเหมือนภาพเนกาทีฟ | เปลี่ยน `TFT_INVERT=1` เป็น `0` |
| ภาพเลื่อนหรือขอบถูกตัด | ลองเพิ่ม `-D TFT_COLSTART=26` และ `-D TFT_ROWSTART=1` |
| ภาพรบกวน/สายยาว | ลด `TFT_SPI_HZ=4000000` และตรวจจุดต่อ |
| เปลี่ยนความเร็วเล่นอย่างเดียว | เพิ่ม `-D PLAYBACK_FPS=5` (ระยะเวลาคลิปจะเปลี่ยนตาม) |

อย่าสลับแดง/น้ำเงินทั้งในตัวแปลงและในเฟิร์มแวร์พร้อมกัน เพราะจะหักล้างกัน
หากจอสว่างแต่ไม่มีภาพ ให้ตรวจ CS/DC/RES, BLK, กราวด์ และรุ่นชิป
การ Build ผ่านหรือ Serial บอก PLAYING ไม่ได้ยืนยันว่าจอได้รับ/แสดงภาพจริง

## ทดสอบซ้ำ

```powershell
python tools/make_sample.py
python tools/mp4_to_tft.py videos/sample.mp4 --fps 10 --seconds 3 --strict-size
python tools/test_converter.py
pio run -e esp32-c3 -e esp32-c3-usb
```

ดูผลที่ดำเนินการจริงใน `TEST_RESULTS.md`

## แหล่งอ้างอิง

- [LCDWiki: ST7735S 80×160 และคำอธิบายขา รวม BLK](https://www.lcdwiki.com/0.96inch_IPS_Module)
- [Adafruit ST7735 library และโปรไฟล์ MINI160x80](https://github.com/adafruit/Adafruit-ST7735-Library)

เอกสาร LCDWiki เป็นตัวอย่างโมดูลที่ชื่อขาและสเปกสอดคล้องกับภาพ ไม่ได้ยืนยันยี่ห้อของจอในภาพ
