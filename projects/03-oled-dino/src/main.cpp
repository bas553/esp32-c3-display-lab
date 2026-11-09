#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>

// Keep the wiring from the user's working OLED/button project.
constexpr int SDA_PIN = 3;
constexpr int SCL_PIN = 2;
constexpr int BUTTON_PIN = 1;
constexpr uint32_t DEBOUNCE_MS = 25;
constexpr uint32_t DOUBLE_CLICK_MS = 600;
constexpr uint32_t I2C_HZ = 400000;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int GROUND_Y = 55;
constexpr int DINO_X = 14;
constexpr int DINO_WIDTH = 16;
constexpr int DINO_HEIGHT = 18;
constexpr uint32_t PHYSICS_MS = 16;
constexpr uint32_t FRAME_MS = 33;
constexpr float GRAVITY = 430.0f;
constexpr float JUMP_VELOCITY = -160.0f;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1,
                        I2C_HZ, I2C_HZ);

enum class GameState { Title, Playing, GameOver };
GameState gameState = GameState::Title;
bool oledReady = false;
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
uint32_t buttonChangedAt = 0;
bool firstClickPending = false;
uint32_t firstClickAt = 0;
uint32_t lastPhysicsAt = 0;
uint32_t lastFrameAt = 0;
uint32_t score = 0;
uint32_t bestScore = 0;
float dinoY = GROUND_Y - DINO_HEIGHT;
float velocityY = 0.0f;
bool onGround = true;
float obstacleX = 140.0f;
int obstacleWidth = 8;
int obstacleHeight = 14;
bool obstacleScored = false;
float groundOffset = 0.0f;

// One event per physical press, only after the input is stable.
bool buttonPressed(uint32_t now) {
    const int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonReading) {
        lastButtonReading = reading;
        buttonChangedAt = now;
    }
    if (now - buttonChangedAt >= DEBOUNCE_MS &&
        reading != stableButtonState) {
        stableButtonState = reading;
        return stableButtonState == LOW;
    }
    return false;
}

uint8_t findOled() {
    constexpr uint8_t addresses[] = {0x3C, 0x3D};
    for (const uint8_t address : addresses) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) return address;
    }
    return 0;
}

void startGame(uint32_t now) {
    gameState = GameState::Playing;
    score = 0;
    dinoY = GROUND_Y - DINO_HEIGHT;
    velocityY = 0.0f;
    onGround = true;
    obstacleX = 140.0f;
    obstacleWidth = 8;
    obstacleHeight = 14;
    obstacleScored = false;
    groundOffset = 0.0f;
    firstClickPending = false;
    lastPhysicsAt = now;
    lastFrameAt = now - FRAME_MS;
    Serial.println("DINO RUN: started. Press to jump.");
}

void handlePress(uint32_t now) {
    switch (gameState) {
        case GameState::Title:
            if (firstClickPending && now - firstClickAt <= DOUBLE_CLICK_MS) {
                startGame(now);
            } else {
                firstClickPending = true;
                firstClickAt = now;
            }
            break;
        case GameState::Playing:
            if (onGround) {
                velocityY = JUMP_VELOCITY;
                onGround = false;
            }
            break;
        case GameState::GameOver:
            // Consume this press as restart, not as an immediate jump.
            startGame(now);
            break;
    }
}

bool hitsObstacle() {
    // Slightly inset hitboxes make the small pixel sprite forgiving.
    const float left = DINO_X + 3;
    const float right = DINO_X + DINO_WIDTH - 2;
    const float top = dinoY + 2;
    const float bottom = dinoY + DINO_HEIGHT - 1;
    return right > obstacleX + 1 &&
           left < obstacleX + obstacleWidth - 1 &&
           bottom > GROUND_Y - obstacleHeight + 1 && top < GROUND_Y;
}

void updateGame() {
    constexpr float dt = PHYSICS_MS / 1000.0f;
    const float speed = 55.0f + (score < 20 ? score : 20) * 2.0f;

    if (!onGround) {
        velocityY += GRAVITY * dt;
        dinoY += velocityY * dt;
        if (dinoY >= GROUND_Y - DINO_HEIGHT) {
            dinoY = GROUND_Y - DINO_HEIGHT;
            velocityY = 0.0f;
            onGround = true;
        }
    }
    obstacleX -= speed * dt;
    groundOffset += speed * dt;
    if (groundOffset >= 16.0f) groundOffset -= 16.0f;

    if (hitsObstacle()) {
        gameState = GameState::GameOver;
        Serial.printf("GAME OVER | Score: %lu | Best: %lu\n",
                      static_cast<unsigned long>(score),
                      static_cast<unsigned long>(bestScore));
        return;
    }
    if (!obstacleScored && obstacleX + obstacleWidth < DINO_X) {
        obstacleScored = true;
        if (score < 99999) ++score;
        if (score > bestScore) bestScore = score;
        Serial.printf("Score: %lu\n", static_cast<unsigned long>(score));
    }
    if (obstacleX + obstacleWidth < 0) {
        // One obstacle at a time, with enough space to land before the next.
        obstacleX = SCREEN_WIDTH + random(12, 45);
        obstacleWidth = random(7, 12);
        obstacleHeight = random(12, 19);
        obstacleScored = false;
    }
}

void drawDino(uint32_t now) {
    const int y = static_cast<int>(dinoY);
    display.fillRect(DINO_X + 7, y, 9, 7, SSD1306_WHITE);
    display.drawPixel(DINO_X + 12, y + 2, SSD1306_BLACK);
    display.drawFastHLine(DINO_X + 12, y + 6, 4, SSD1306_BLACK);
    display.fillRect(DINO_X + 5, y + 6, 7, 8, SSD1306_WHITE);
    display.fillTriangle(DINO_X, y + 7, DINO_X + 7, y + 12,
                         DINO_X + 7, y + 15, SSD1306_WHITE);
    display.drawFastHLine(DINO_X + 11, y + 9, 4, SSD1306_WHITE);
    display.drawPixel(DINO_X + 14, y + 10, SSD1306_WHITE);
    const bool stride = gameState == GameState::Playing && onGround &&
                        ((now / 110) % 2 == 0);
    display.fillRect(DINO_X + 5, y + 13, 2, stride ? 3 : 5, SSD1306_WHITE);
    display.fillRect(DINO_X + 10, y + 13, 2, stride ? 5 : 3, SSD1306_WHITE);
    display.drawFastHLine(DINO_X + 5, y + (stride ? 15 : 17), 4, SSD1306_WHITE);
    display.drawFastHLine(DINO_X + 10, y + (stride ? 17 : 15), 4, SSD1306_WHITE);
}

void drawCactus(int x, int width, int height) {
    const int y = GROUND_Y - height;
    display.fillRoundRect(x + width / 2 - 1, y, 3, height, 1, SSD1306_WHITE);
    display.drawFastVLine(x, y + 4, height / 2, SSD1306_WHITE);
    display.drawFastHLine(x, y + 4 + height / 2, width / 2, SSD1306_WHITE);
    display.drawFastVLine(x + width - 1, y + 2, height / 2, SSD1306_WHITE);
    display.drawFastHLine(x + width / 2, y + 2 + height / 2,
                         width - width / 2, SSD1306_WHITE);
}

void render(uint32_t now) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.drawFastHLine(0, GROUND_Y, SCREEN_WIDTH, SSD1306_WHITE);
    for (int x = -16; x < SCREEN_WIDTH + 16; x += 16) {
        display.drawFastHLine(x - static_cast<int>(groundOffset), GROUND_Y + 4,
                             3, SSD1306_WHITE);
    }
    drawDino(now);

    if (gameState == GameState::Title) {
        drawCactus(105, 9, 16);
        display.setTextSize(2);
        display.setCursor(16, 3);
        display.print("DINO RUN");
        display.setTextSize(1);
        display.setCursor(28, 26);
        display.print(firstClickPending ? "PRESS AGAIN!" : "DOUBLE CLICK");
        display.setCursor(40, 36);
        display.print("TO START");
        display.display();
        return;
    }

    drawCactus(static_cast<int>(obstacleX), obstacleWidth, obstacleHeight);
    display.setCursor(0, 0);
    display.printf("HI%05lu  %05lu", static_cast<unsigned long>(bestScore),
                   static_cast<unsigned long>(score));
    if (gameState == GameState::GameOver) {
        display.fillRect(3, 13, 122, 37, SSD1306_BLACK);
        display.drawRect(3, 13, 122, 37, SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(10, 18);
        display.print("GAME OVER");
        display.setTextSize(1);
        display.setCursor(16, 39);
        display.print("PRESS TO RESTART");
    }
    display.display();
}

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    // Ignore a held button at boot until it has been released.
    lastButtonReading = stableButtonState = digitalRead(BUTTON_PIN);
    buttonChangedAt = millis();
    Serial.begin(115200);
    const uint32_t started = millis();
    while (!Serial && millis() - started < 1500) delay(10);
    Serial.println("\nDINO RUN: double click to start, press to jump/restart.");
    Serial.printf("OLED SDA=%d SCL=%d | Button GPIO%d to GND\n",
                  SDA_PIN, SCL_PIN, BUTTON_PIN);
    randomSeed(esp_random());

    if (!Wire.begin(SDA_PIN, SCL_PIN, I2C_HZ)) {
        Serial.println("ERROR: Cannot start I2C.");
        return;
    }
    Wire.setTimeOut(50);
    const uint8_t address = findOled();
    if (address == 0) {
        Serial.println("ERROR: No OLED at 0x3C/0x3D. Check wiring, then RESET.");
        return;
    }
    oledReady = display.begin(SSD1306_SWITCHCAPVCC, address, true, false);
    if (!oledReady) {
        Serial.println("ERROR: Cannot allocate OLED framebuffer.");
        return;
    }
    Serial.printf("OLED ready at 0x%02X\n", address);
    lastFrameAt = millis();
    render(lastFrameAt);
}

void loop() {
    if (!oledReady) {
        delay(10);
        return;
    }
    const uint32_t now = millis();
    if (buttonPressed(now)) handlePress(now);
    if (firstClickPending && now - firstClickAt > DOUBLE_CLICK_MS) {
        firstClickPending = false;
    }
    if (gameState == GameState::Playing) {
        // Fixed physics step; cap catch-up work after an unexpected stall.
        if (now - lastPhysicsAt > PHYSICS_MS * 5) {
            lastPhysicsAt = now - PHYSICS_MS * 5;
        }
        while (now - lastPhysicsAt >= PHYSICS_MS &&
               gameState == GameState::Playing) {
            lastPhysicsAt += PHYSICS_MS;
            updateGame();
        }
    }
    if (now - lastFrameAt >= FRAME_MS) {
        lastFrameAt = now;
        render(now);
    }
    delay(1);
}
