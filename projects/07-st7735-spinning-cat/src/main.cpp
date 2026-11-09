#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <pgmspace.h>
#ifndef TFT_BUTTON_TEST
#define TFT_BUTTON_TEST 0
#endif
#if !TFT_BUTTON_TEST
#include "video_frames.h"
#endif

// SCL/SDA on this module are SPI clock/MOSI, not I2C.
constexpr int TFT_SCLK = 2;
constexpr int TFT_MOSI = 3;
constexpr int TFT_RST = 4;
constexpr int TFT_DC = 5;
constexpr int TFT_CS = 7;
constexpr int BUTTON_PIN = 1;
constexpr uint32_t DEBOUNCE_MS = 30;
// VCC and BLK connect to 3.3V. GND connects to GND. No MISO pin.
#ifndef TFT_SPI_HZ
#define TFT_SPI_HZ 8000000
#endif
#ifndef TFT_ROTATION
#define TFT_ROTATION 1
#endif
#ifndef TFT_INVERT
#define TFT_INVERT 0
#endif
#ifndef TFT_SWAP_RB
#define TFT_SWAP_RB 0
#endif
#ifndef TFT_TEST_ONLY
#define TFT_TEST_ONLY 0
#endif
#ifndef TFT_COLSTART
#define TFT_COLSTART -1
#endif
#ifndef TFT_ROWSTART
#define TFT_ROWSTART -1
#endif
#ifndef PLAYBACK_FPS
#define PLAYBACK_FPS 0
#endif
static_assert(TFT_ROTATION == 1 || TFT_ROTATION == 3, "Use landscape rotation 1 or 3.");
#if !TFT_BUTTON_TEST
static_assert(VIDEO_WIDTH == 160 && VIDEO_HEIGHT == 80, "Video must be 160x80.");
static_assert(VIDEO_FRAME_COUNT > 0 && VIDEO_FRAME_PIXELS == 12800, "Invalid frame metadata.");
static_assert(sizeof(VIDEO_FRAMES[0]) == 25600, "RGB565 frame must be 25600 bytes.");
static_assert(sizeof(VIDEO_FRAMES) == VIDEO_FRAME_COUNT * 25600UL, "Video size mismatch.");
constexpr uint32_t FPS_NUM = PLAYBACK_FPS > 0 ? PLAYBACK_FPS : VIDEO_FPS_NUM;
constexpr uint32_t FPS_DEN = PLAYBACK_FPS > 0 ? 1 : VIDEO_FPS_DEN;
static_assert(FPS_DEN > 0 && FPS_NUM > 0 && FPS_NUM <= 60 * FPS_DEN, "Invalid FPS.");
#endif

class Panel : public Adafruit_ST7735 {
public:
    Panel() : Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST) {}
    void configureOffsets() {
        if (TFT_COLSTART >= 0) _colstart = TFT_COLSTART;
        if (TFT_ROWSTART >= 0) _rowstart = TFT_ROWSTART;
        setRotation(TFT_ROTATION);
    }
};
Panel tft;
#if !TFT_BUTTON_TEST
// Only one scanline (320 bytes), not a 25600-byte framebuffer or the whole movie.
uint16_t rowBuffer[VIDEO_WIDTH];
uint32_t nextFrameIndex = 0, nextFrameAt = 0, fractionMs = 0;
uint32_t lastReportAt = 0, displayed = 0, skipped = 0;
bool playing = false;
#endif
bool ready = false;

#if TFT_BUTTON_TEST
struct TestColor {
    uint16_t value;
    const char *name;
};
constexpr TestColor testColors[] = {
    {ST77XX_RED, "RED"}, {ST77XX_GREEN, "GREEN"},
    {ST77XX_BLUE, "BLUE"}, {ST77XX_WHITE, "WHITE"},
    {ST77XX_BLACK, "BLACK"}, {ST77XX_YELLOW, "YELLOW"},
    {ST77XX_CYAN, "CYAN"}, {ST77XX_MAGENTA, "MAGENTA"}
};
constexpr uint8_t COLOR_COUNT = sizeof(testColors) / sizeof(testColors[0]);
uint8_t colorIndex = 0;
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
uint32_t buttonChangedAt = 0;

bool buttonPressed(uint32_t now) {
    const int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonReading) {
        lastButtonReading = reading;
        buttonChangedAt = now;
    }
    if (now - buttonChangedAt >= DEBOUNCE_MS && reading != stableButtonState) {
        stableButtonState = reading;
        return stableButtonState == LOW;
    }
    return false;
}
#endif

uint16_t panelColor(uint16_t color) {
#if TFT_SWAP_RB
    return ((color & 0xF800) >> 11) | (color & 0x07E0) | ((color & 0x001F) << 11);
#else
    return color;
#endif
}

#if TFT_BUTTON_TEST
void showColor() {
    // No text or border: every pixel is available for checking the panel.
    tft.fillScreen(panelColor(testColors[colorIndex].value));
    Serial.printf("COLOR %u/%u: %s | button GPIO%d to GND\n",
                  static_cast<unsigned>(colorIndex + 1),
                  static_cast<unsigned>(COLOR_COUNT), testColors[colorIndex].name, BUTTON_PIN);
}
#else
void drawTestPattern() {
    tft.fillScreen(ST77XX_BLACK);
    const uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_WHITE};
    for (int i = 0; i < 4; ++i) {
        tft.fillRect(i * 40, 16, 40, 36, panelColor(colors[i]));
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(i * 40 + 15, 29);
        tft.print("RGBW"[i]);
    }
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(4, 4);
    tft.print("ST7735S 160x80");
    tft.setCursor(4, 59);
    tft.print("MOSI3 CLK2 CS7");
    tft.drawRect(0, 0, 160, 80, ST77XX_WHITE);
}

void sendFrame(uint32_t frame) {
    tft.startWrite();
    tft.setAddrWindow(0, 0, VIDEO_WIDTH, VIDEO_HEIGHT);
    for (uint32_t y = 0; y < VIDEO_HEIGHT; ++y) {
        memcpy_P(rowBuffer, &VIDEO_FRAMES[frame][y * VIDEO_WIDTH], sizeof(rowBuffer));
#if TFT_SWAP_RB
        for (uint16_t &pixel : rowBuffer) pixel = panelColor(pixel);
#endif
        // Native uint16_t values. Adafruit/ESP32 swaps bytes for the SPI wire.
        tft.writePixels(rowBuffer, VIDEO_WIDTH, true, false);
    }
    tft.endWrite();
}

void advanceDeadline() {
    constexpr uint32_t numerator = 1000UL * FPS_DEN;
    nextFrameAt += numerator / FPS_NUM;
    fractionMs += numerator % FPS_NUM;
    nextFrameAt += fractionMs / FPS_NUM;
    fractionMs %= FPS_NUM;
}
#endif

void setup() {
#if TFT_BUTTON_TEST
    pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
    Serial.begin(115200);
    const uint32_t began = millis();
    while (!Serial && millis() - began < 1200) delay(1);
#if TFT_BUTTON_TEST
    Serial.println("\nST7735S BUTTON COLOR TEST: press once to change color.");
#else
    Serial.println("\nST7735S RGB565 VIDEO PLAYER");
#endif
    Serial.printf("SCL/CLK=%d SDA/MOSI=%d RES=%d DC=%d CS=%d; BLK=3.3V\n",
                  TFT_SCLK, TFT_MOSI, TFT_RST, TFT_DC, TFT_CS);
#if !TFT_BUTTON_TEST
    Serial.printf("Frames=%lu Video flash=%lu RAM scanline=%u SPI=%lu Hz\n",
                  static_cast<unsigned long>(VIDEO_FRAME_COUNT),
                  static_cast<unsigned long>(sizeof(VIDEO_FRAMES)),
                  static_cast<unsigned>(sizeof(rowBuffer)), static_cast<unsigned long>(TFT_SPI_HZ));
#endif
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.initR(INITR_MINI160x80);
    tft.configureOffsets();
    tft.invertDisplay(TFT_INVERT != 0);
    tft.setSPISpeed(TFT_SPI_HZ);
    if (tft.width() != 160 || tft.height() != 80) {
        Serial.println("ERROR: Wrong panel geometry; playback stopped.");
        return;
    }
    ready = true;
#if TFT_BUTTON_TEST
    // A button held during startup must be released before the first press.
    lastButtonReading = stableButtonState = digitalRead(BUTTON_PIN);
    buttonChangedAt = millis();
    showColor();
#else
    drawTestPattern();
    nextFrameAt = millis() + 2500;
    Serial.println("Color test R/G/B/W. No MISO: panel presence cannot be auto-detected.");
#if TFT_TEST_ONLY
    Serial.println("TEST ONLY: check colors, border and orientation on the actual panel.");
#endif
#endif
}

void loop() {
#if TFT_BUTTON_TEST
    if (ready && buttonPressed(millis())) {
        colorIndex = (colorIndex + 1) % COLOR_COUNT;
        showColor();
    }
    delay(1);
#else
    if (!ready || TFT_TEST_ONLY) {
        delay(1);
        return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextFrameAt) >= 0) {
        if (!playing) {
            playing = true;
            lastReportAt = now;
            Serial.println("PLAYING: looping video.");
        }
        if (now - nextFrameAt > 5000) {
            nextFrameAt = now;
            fractionMs = 0;
        }
        uint32_t frame = nextFrameIndex, due = 0;
        do {
            frame = nextFrameIndex;
            nextFrameIndex = (nextFrameIndex + 1) % VIDEO_FRAME_COUNT;
            advanceDeadline();
            ++due;
        } while (static_cast<int32_t>(now - nextFrameAt) >= 0);
        skipped += due - 1;
        sendFrame(frame);
        ++displayed;
    }
    const uint32_t reportNow = millis();
    if (playing && reportNow - lastReportAt >= 1000) {
        Serial.printf("PLAYING FPS=%.2f target=%.3f next=%lu/%lu skipped=%lu heap=%lu\n",
                      displayed * 1000.0f / (reportNow - lastReportAt),
                      static_cast<double>(FPS_NUM) / FPS_DEN,
                      static_cast<unsigned long>(nextFrameIndex + 1),
                      static_cast<unsigned long>(VIDEO_FRAME_COUNT),
                      static_cast<unsigned long>(skipped), static_cast<unsigned long>(ESP.getFreeHeap()));
        lastReportAt = reportNow;
        displayed = skipped = 0;
    }
    delay(1);
#endif
}
