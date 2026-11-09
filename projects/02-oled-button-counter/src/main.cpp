#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_timer.h>

constexpr int SDA_PIN = 3;
constexpr int SCL_PIN = 2;
constexpr int BUTTON_PIN = 1;
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint32_t I2C_HZ = 100000;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1,
                        I2C_HZ, I2C_HZ);
bool oledReady = false;
uint8_t oledAddress = 0;
uint64_t lastSeconds = UINT64_MAX;
uint32_t pressCount = 0;
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
uint32_t buttonChangedAt = 0;

// Count only a debounced HIGH -> LOW transition, not a held button.
bool updateButton() {
    const int reading = digitalRead(BUTTON_PIN);
    const uint32_t now = millis();
    if (reading != lastButtonReading) {
        lastButtonReading = reading;
        buttonChangedAt = now;
    }
    if (now - buttonChangedAt >= DEBOUNCE_MS &&
        reading != stableButtonState) {
        stableButtonState = reading;
        if (stableButtonState == LOW) {
            ++pressCount;
            return true;
        }
    }
    return false;
}

uint8_t findOled() {
    constexpr uint8_t addresses[] = {0x3C, 0x3D};
    for (const uint8_t address : addresses) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            return address;
        }
    }
    return 0;
}

void showUptime(uint64_t seconds) {
    const unsigned long days = seconds / 86400ULL;
    const unsigned hours = (seconds / 3600ULL) % 24;
    const unsigned minutes = (seconds / 60ULL) % 60;
    const unsigned secs = seconds % 60;
    char uptime[24];
    snprintf(uptime, sizeof(uptime), "%lud %02u:%02u:%02u",
             days, hours, minutes, secs);

    Serial.printf("Presses: %lu | Uptime: %s | OLED: %s\n",
                  static_cast<unsigned long>(pressCount), uptime,
                  oledReady ? "OK" : "NOT READY");
    if (!oledReady) return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("ESP32-C3");
    display.setTextSize(1);
    display.setCursor(0, 19);
    display.print("OLED TEST OK");
    display.setCursor(0, 30);
    display.print("Presses:");
    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print(pressCount);
    display.setTextSize(1);
    display.setCursor(0, 57);
    display.print(uptime);
    display.display();
}

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    // A button held during boot must be released before it can be counted.
    lastButtonReading = digitalRead(BUTTON_PIN);
    stableButtonState = lastButtonReading;
    buttonChangedAt = millis();
    Serial.begin(115200);
    // Bounded wait: the display also works without a Serial Monitor.
    const uint32_t started = millis();
    while (!Serial && millis() - started < 1500) delay(10);
    Serial.println("\nESP32-C3 / SSD1306 128x64 OLED test");
    Serial.printf("VCC=3.3V GND=GND SDA=GPIO%d SCL=GPIO%d\n",
                  SDA_PIN, SCL_PIN);
    Serial.printf("Button: GPIO%d to GND (INPUT_PULLUP)\n", BUTTON_PIN);

    if (!Wire.begin(SDA_PIN, SCL_PIN, I2C_HZ)) {
        Serial.println("ERROR: Could not start I2C.");
        return;
    }
    Wire.setTimeOut(50);
    oledAddress = findOled();
    if (oledAddress == 0) {
        Serial.println("ERROR: No OLED ACK at 0x3C or 0x3D.");
        Serial.println("Check wiring and 3.3V, then press RESET.");
        return;
    }
    Serial.printf("I2C device found at 0x%02X\n", oledAddress);
    // Wire is already initialized with SDA_PIN/SCL_PIN; do not begin it again.
    oledReady = display.begin(SSD1306_SWITCHCAPVCC, oledAddress, true, false);
    if (!oledReady) {
        Serial.println("ERROR: SSD1306 framebuffer allocation failed.");
    }
}

void loop() {
    const bool pressed = updateButton();
    // 64-bit timer keeps uptime correct beyond the millis() rollover.
    const uint64_t seconds = esp_timer_get_time() / 1000000ULL;
    if (pressed || seconds != lastSeconds) {
        lastSeconds = seconds;
        showUptime(seconds);
    }
    delay(1);
}
