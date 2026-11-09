#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <pgmspace.h>
#include "video_frames.h"

constexpr int SDA_PIN = 3;
constexpr int SCL_PIN = 2;
#ifndef OLED_I2C_HZ
#define OLED_I2C_HZ 400000
#endif
#ifndef PLAYBACK_FPS
#define PLAYBACK_FPS 0
#endif
static_assert(OLED_I2C_HZ == 400000 || OLED_I2C_HZ == 100000,
              "Use 400000 or 100000 Hz for this project.");
static_assert(VIDEO_WIDTH == 128 && VIDEO_HEIGHT == 64, "OLED must be 128x64.");
static_assert(VIDEO_FRAME_BYTES == 1024 && VIDEO_FRAME_COUNT > 0,
              "Each frame must contain 1024 bytes; at least one frame is required.");
static_assert(sizeof(VIDEO_FRAMES) == VIDEO_FRAME_COUNT * VIDEO_FRAME_BYTES,
              "Video data size mismatch.");
constexpr uint32_t FPS_NUM = PLAYBACK_FPS > 0 ? PLAYBACK_FPS : VIDEO_FPS_NUM;
constexpr uint32_t FPS_DEN = PLAYBACK_FPS > 0 ? 1 : VIDEO_FPS_DEN;
static_assert(FPS_DEN > 0 && FPS_NUM > 0 && FPS_NUM <= 60 * FPS_DEN,
              "FPS must be greater than zero and at most 60.");

Adafruit_SSD1306 display(128, 64, &Wire, -1, OLED_I2C_HZ, OLED_I2C_HZ);
bool ready = false;
uint32_t nextFrameAt = 0;
uint32_t nextFrameIndex = 0;
uint32_t fractionMs = 0;
uint32_t lastReportAt = 0;
uint32_t displayedSinceReport = 0;
uint32_t skippedSinceReport = 0;

uint8_t findOled() {
    constexpr uint8_t addresses[] = {0x3C, 0x3D};
    for (uint8_t address : addresses) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) return address;
    }
    return 0;
}

void scheduleNextFrame() {
    // Rational FPS, including 29.97: carry fractional milliseconds to avoid drift.
    constexpr uint32_t numerator = 1000UL * FPS_DEN;
    nextFrameAt += numerator / FPS_NUM;
    fractionMs += numerator % FPS_NUM;
    if (fractionMs >= FPS_NUM) {
        nextFrameAt += fractionMs / FPS_NUM;
        fractionMs %= FPS_NUM;
    }
}

void setup() {
    Serial.begin(115200);
    const uint32_t serialWaitAt = millis();
    while (!Serial && millis() - serialWaitAt < 1200) delay(1);
    Serial.println("\nESP32-C3 OLED VIDEO PLAYER");
    Serial.printf("Frames=%lu | Bytes/frame=%lu | Video flash=%lu bytes\n",
                  static_cast<unsigned long>(VIDEO_FRAME_COUNT),
                  static_cast<unsigned long>(VIDEO_FRAME_BYTES),
                  static_cast<unsigned long>(sizeof(VIDEO_FRAMES)));
    Serial.printf("Target FPS=%.3f | Heap=%lu | SDA=%d SCL=%d | I2C=%lu Hz\n",
                  static_cast<double>(FPS_NUM) / FPS_DEN,
                  static_cast<unsigned long>(ESP.getFreeHeap()), SDA_PIN, SCL_PIN,
                  static_cast<unsigned long>(OLED_I2C_HZ));
    if (!Wire.begin(SDA_PIN, SCL_PIN, OLED_I2C_HZ)) {
        Serial.println("ERROR: I2C initialization failed. Playback stopped.");
        return;
    }
    Wire.setTimeOut(50);
    const uint8_t address = findOled();
    if (address == 0) {
        Serial.println("ERROR: No OLED ACK at 0x3C/0x3D. Check 3.3V/GND/SDA/SCL.");
        Serial.println("Try OLED_I2C_HZ=100000, rebuild, then reset after fixing wiring.");
        return;
    }
    // begin() allocates the single 1024-byte display buffer; check its result.
    if (!display.begin(SSD1306_SWITCHCAPVCC, address, true, false) ||
        display.getBuffer() == nullptr) {
        Serial.println("ERROR: Not enough memory for the OLED framebuffer.");
        return;
    }
    display.clearDisplay();
    ready = true;
    nextFrameAt = lastReportAt = millis();
    Serial.printf("OLED ACK at 0x%02X; status=PLAYING, loop enabled.\n", address);
}

void loop() {
    if (!ready) {
        delay(1); // Yield while stopped; the startup error remains in Serial.
        return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextFrameAt) >= 0) {
        // Avoid an unbounded catch-up loop after a long debugger pause.
        if (now - nextFrameAt > 5000) {
            nextFrameAt = now;
            fractionMs = 0;
            Serial.println("Timing resynchronized after a long stall.");
        }
        uint32_t frameToShow = nextFrameIndex;
        uint32_t due = 0;
        do {
            frameToShow = nextFrameIndex;
            nextFrameIndex = (nextFrameIndex + 1) % VIDEO_FRAME_COUNT;
            scheduleNextFrame();
            ++due;
        } while (static_cast<int32_t>(now - nextFrameAt) >= 0);
        skippedSinceReport += due - 1;
        // SSD1306 page layout: byte=x+(y/8)*128, bit=(y%8), bit 1 lights pixel.
        // Read one frame from flash directly into Adafruit's display buffer.
        memcpy_P(display.getBuffer(), VIDEO_FRAMES[frameToShow], VIDEO_FRAME_BYTES);
        display.display();
        ++displayedSinceReport;
    }
    const uint32_t reportNow = millis();
    if (reportNow - lastReportAt >= 1000) {
        const float actualFps = displayedSinceReport * 1000.0f /
                                (reportNow - lastReportAt);
        Serial.printf("PLAYING | FPS=%.2f target=%.3f | next=%lu/%lu | skipped=%lu\n",
                      actualFps, static_cast<double>(FPS_NUM) / FPS_DEN,
                      static_cast<unsigned long>(nextFrameIndex + 1),
                      static_cast<unsigned long>(VIDEO_FRAME_COUNT),
                      static_cast<unsigned long>(skippedSinceReport));
        lastReportAt = reportNow;
        displayedSinceReport = skippedSinceReport = 0;
    }
    delay(1); // Yield only; frame deadlines are controlled by millis(), not delay().
}
