> บันทึกย้อนหลังจากวันที่ 8 พฤศจิกายน ดูผล Build หลังแก้ TFT_INVERT=0 ใน [VALIDATION.md](../../VALIDATION.md) เจ้าของบอร์ดยืนยันว่าสีถูกต้องแล้วภายหลังบันทึกนี้

# ผลทดสอบจริง — 8 พฤศจิกายน 2026

## อัปเดตล่าสุด: วิดีโอ OIIA OIIA

นำ 3 วินาทีแรกจากไฟล์ผู้ใช้ใน Downloads มาแปลงโดยตรงเป็น RGB565 160×80
Crop / 10 FPS / 30 เฟรม รวม 768,000 ไบต์ ไม่มีเสียง
ต้นฉบับยาว 134.732290 วินาที ไม่ได้ฝังทั้งคลิป
ตั้ง `TFT_BUTTON_TEST=0` แล้ว Build จริงทั้งสอง environment สำเร็จ:

- esp32-c3: Flash 1,043,182 ไบต์; static RAM 14,508 ไบต์
- esp32-c3-usb: Flash 1,028,782 ไบต์; static RAM 14,420 ไบต์
- SHA256 เฟรม: `9da5859186170e3a43da0a7344a106f0024b4eff97b0d1ee5102319ba5787f62`
- ล่าสุดพบ COM4 เป็น USB VID:PID 303A:1001 อีกครั้ง ยังรอผู้ใช้ยืนยันก่อน Upload
- ยังไม่มีการ Upload หรือทดสอบคลิปนี้บนจอจริง บันทึก Build อยู่ที่ `validation/cat-build.log`

## ผลก่อนหน้า: ปุ่มเปลี่ยนสีเต็มจอ

ตั้ง `TFT_BUTTON_TEST=1` เป็นค่าเริ่มต้น ใช้ปุ่ม GPIO1 ↔ GND แบบ INPUT_PULLUP
กรองหน้าสัมผัสเด้ง 30 ms รับหนึ่งเหตุการณ์ต่อการกด และต้องปล่อยก่อนกดใหม่
เริ่มสีแดง วนผ่านเขียว น้ำเงิน ขาว ดำ เหลือง Cyan และ Magenta
แสดงชื่อสีทาง Serial ไม่มีข้อความทับสีบนจอ

Build จริงด้วย `pio run -e esp32-c3 -e esp32-c3-usb`:

| Environment | ผล | Flash | Static RAM |
| --- | --- | --- | --- |
| esp32-c3 | SUCCESS | 274,312 / 1,310,720 ไบต์ | 14,172 / 327,680 ไบต์ |
| esp32-c3-usb | SUCCESS | 259,914 / 1,310,720 ไบต์ | 14,084 / 327,680 ไบต์ |

โหมดปุ่มไม่ include อาร์เรย์วิดีโอ จึงไม่ใช้ Flash สำหรับเฟรมตัวอย่าง
ยังไม่ได้ Upload หรือทดสอบปุ่ม/สีบนบอร์ดจริงในรอบนี้
บันทึกล่าสุดอยู่ที่ `validation/build.log`; ผลต่อไปนี้เป็นผลโหมดวิดีโอจากรอบก่อน
และเก็บบันทึกไว้ที่ `validation/video-build.log`

## Converter

- สร้าง sample.mp4 แบบสีเอง ความละเอียด 320×160, 30 เฟรม / 10 FPS / 3 วินาที
- แปลงเป็น RGB565 160×80 ได้ 30 เฟรม เฟรมละ 25,600 ไบต์ รวม 768,000 ไบต์
- SHA256 ของเฟรม RGB565 little-endian:
  `154d483b6b4908e689b9e299c7a3ed4afd89a8b9fb872d0af1e627ed3c9ac830`
- `python tools/test_converter.py`: **8 tests ผ่านทั้งหมด**
- ตรวจค่ามาตรฐาน red=0xF800, green=0x07E0, blue=0x001F, white=0xFFFF, black=0
- ตรวจลำดับพิกเซลและขอบแถว, Invert, Swap R/B, short pipe reads, การปฏิเสธเฟรมไม่ครบ
- แปลง MP4/GIF จริง, Crop/Letterbox, FPS แบบเศษส่วน และขนาดทุกเฟรม
- ตรวจคำเตือน Flash, strict-size และไฟล์เสียไม่ทำลาย header เดิม

## Build จริง

คำสั่ง: `pio run -e esp32-c3 -e esp32-c3-usb`

| Environment | ผล | Flash | Static RAM |
| --- | --- | --- | --- |
| esp32-c3 | SUCCESS | 1,043,182 / 1,310,720 ไบต์ (79.6%) | 14,508 / 327,680 ไบต์ (4.4%) |
| esp32-c3-usb | SUCCESS | 1,028,782 / 1,310,720 ไบต์ (78.5%) | 14,420 / 327,680 ไบต์ (4.4%) |

เครื่องมือ: PlatformIO Core 6.2.0, Espressif32 6.10.0, Arduino ESP32 2.0.17,
Adafruit ST7735/ST7789 1.11.0, Adafruit GFX 1.11.11, Python 3.12.14, FFmpeg 9.0
PlatformIO ติดตั้ง BusIO 1.17.4 และ seesaw 1.7.9 เป็น dependency ของไลบรารี
เฟิร์มแวร์ไม่ใช้ Wi-Fi หรือ microSD

ตรวจ ELF ด้วย objdump:

```text
3c030320 l O .flash.rodata 000bb800 _ZL12VIDEO_FRAMES
3fc8d590 g O .dram0.bss    00000140 rowBuffer
```

ข้อมูลภาพอยู่ใน Flash ขนาด 0xBB800 = 768,000 ไบต์
บัฟเฟอร์แถวใน RAM ขนาด 0x140 = 320 ไบต์
ค่า Static RAM ข้างต้นรวมตัวแปรอื่น ไม่ใช่การวัด heap ขณะทำงานจริง

## ฮาร์ดแวร์

อ่านชื่อขาจากภาพ: GND, VCC, SCL, SDA, RES, DC, CS, BLK
เก็บ SDA/MOSI=GPIO3, SCL/SCK=GPIO2 และเพิ่ม RES=4, DC=5, CS=7
โมดูลจริงยังไม่ได้ทดสอบกับโค้ดนี้ ค่า inversion, offset และ rotation อาจต้องปรับ
ภาพถ่ายไม่ยืนยัน revision ของแผงจอ

ตรวจพอร์ตล่าสุดพบเฉพาะ COM1 (`Communications Port`)
ไม่พบ COM4 ที่เคยปรากฏในงานก่อน จึงไม่ได้เลือก upload_port
**ไม่มีการ Upload และยังไม่ได้วัด FPS/สี/ตำแหน่งภาพบนจอจริง**
ต้องยืนยันบอร์ดและพอร์ตก่อน Upload ตามข้อกำหนดของผู้ใช้

บันทึกการ Build และทดสอบอยู่ใน `validation/`
