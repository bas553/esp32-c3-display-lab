// Reconstructed initial display test; not an original historical snapshot.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int SDA_PIN = 4;  // Original wiring. Later projects use SDA3 / SCL2.
constexpr int SCL_PIN = 5;
Adafruit_SSD1306 display(128, 64, &Wire, -1, 100000, 100000);
bool ready = false;
uint32_t lastSecond = UINT32_MAX;

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    uint8_t address = 0;
    for (uint8_t candidate : {0x3C, 0x3D}) {
        Wire.beginTransmission(candidate);
        if (Wire.endTransmission() == 0) { address = candidate; break; }
    }
    if (!address) { Serial.println("OLED not found"); return; }
    ready = display.begin(SSD1306_SWITCHCAPVCC, address, true, false);
    if (!ready) { Serial.println("OLED framebuffer allocation failed"); return; }
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    Serial.println("OLED TEST OK");
}

void loop() {
    uint32_t seconds = millis() / 1000;
    if (seconds == lastSecond) return;
    lastSecond = seconds;
    Serial.printf("Uptime: %lu s\n", static_cast<unsigned long>(seconds));
    if (!ready) return;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ESP32-C3");
    display.println("OLED TEST OK");
    display.printf("Uptime: %lu s", static_cast<unsigned long>(seconds));
    display.display();
}
