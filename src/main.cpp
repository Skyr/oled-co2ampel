#include <Arduino.h>
#include <Wire.h>

#include <SparkFun_SCD30_Arduino_Library.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// OLED display size, in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);   // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SCD30 airSensor;
uint16_t co2buffer[SCREEN_WIDTH+1];

// Pin definitions
#define LED_RED     D6
#define LED_YELLOW  D7
#define LED_GREEN   D5
#define I2C_SCL     D1
#define I2C_SDA     D2

// Number of seconds between measurements
#define UPDATE_INTERVAL 15


void ledsOff() {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
}

void blinkFail(int pin) {
    while (true) {
        digitalWrite(pin, HIGH);
        delay(500);
        digitalWrite(pin, LOW);
        delay(500);
    }
}

void selfCheck() {
    // All LEDs on
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    Serial.println("All LEDs on");
    delay(1000);
    
    // All LEDs off
    ledsOff();
    Serial.println("All LEDs off");
    delay(500);
 
    // I2C pull-up check
    bool checkFailed = false;
    if (digitalRead(I2C_SCL) != HIGH) {
        Serial.println("Pull-up check for I2C SCL failed");
        checkFailed = true;
    }
    if (digitalRead(I2C_SDA) != HIGH) {
        Serial.println("Pull-up check for I2C SDA failed");
        checkFailed = true;
    }
    if (checkFailed) {
        // Blink red led infinitely
        blinkFail(LED_RED);
    }
 
    // I2C communication check
    Wire.begin();
    Wire.beginTransmission(SCD30_ADDRESS);
    if (Wire.endTransmission() != 0) {
        Serial.println("Failed to start I2C communication with SCD30 module");
        digitalWrite(LED_RED, HIGH);
        blinkFail(LED_YELLOW);       
    }
    delay(200);

    airSensor.begin();
    delay(500);
    if (!airSensor.beginMeasuring()) {
        Serial.println("Failed to initialize SCD30 module");
        blinkFail(LED_YELLOW);
    }

    // All good!
    for (int i=0; i<3; i++) {
        digitalWrite(LED_GREEN, HIGH);
        delay(300);
        digitalWrite(LED_GREEN, LOW);
        delay(300);
    }
    // Continue blinking until data available
    while (!airSensor.dataAvailable()) {
        digitalWrite(LED_GREEN, HIGH);
        delay(300);
        digitalWrite(LED_GREEN, LOW);
        delay(300);
    }
}


void setup() {
    Serial.begin(115200);

    // Empty buffer
    memset(co2buffer, 0, sizeof(co2buffer));

    // Setup pins
    pinMode(I2C_SCL, INPUT);
    pinMode(I2C_SDA, INPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    selfCheck();
    ledsOff();

    airSensor.setMeasurementInterval(UPDATE_INTERVAL);
}


#define CO2_BAR_MIN 400
#define CO2_BAR_MAX 1700
#define BAR_MAX_LEN (SCREEN_HEIGHT - 25)

uint16_t barLen(uint16_t co2) {
    if (co2<CO2_BAR_MIN) {
        return 0;
    }
    if (co2>CO2_BAR_MAX) {
        return BAR_MAX_LEN;
    }
    return (co2 - CO2_BAR_MIN) * BAR_MAX_LEN / (CO2_BAR_MAX - CO2_BAR_MIN);
}


void loop() {
    uint16_t co2 = 0;
    float humidity = 0;

    // Read data
    if (airSensor.dataAvailable()) {
        co2 = airSensor.getCO2();
        humidity = airSensor.getHumidity();

        digitalWrite(LED_RED, (co2 >= 1500) ? HIGH : LOW);
        digitalWrite(LED_YELLOW, (co2 > 1000 && co2 < 1500) ? HIGH : LOW);
        digitalWrite(LED_GREEN, (co2 <= 1000) ? HIGH : LOW);
        

        Serial.print("co2(ppm): ");
        Serial.print(co2);

        Serial.print("  temp(C): ");
        Serial.print(airSensor.getTemperature(), 1);

        Serial.print("  humidity(%): ");
        Serial.print(humidity, 1);

        Serial.println();
    }

    // Update display
    co2buffer[SCREEN_WIDTH] = co2;
    // Update chart
    for (int i=0; i<SCREEN_WIDTH; i++) {
        uint16_t old_len = barLen(co2buffer[i]);
        uint16_t new_len = barLen(co2buffer[i+1]);
        if (old_len!=new_len) {
            if (old_len<new_len) {
                display.drawFastVLine(i, SCREEN_HEIGHT - 1 - new_len, new_len-old_len, WHITE);
            } else {
                display.drawFastVLine(i, SCREEN_HEIGHT - 1 - old_len, old_len-new_len, BLACK);
            }
        }
    }
    // Roll buffer
    for (int i=0; i<SCREEN_WIDTH; i++) {
        co2buffer[i] = co2buffer[i+1];
    }
    // Draw 5 min lines
    for (int x=SCREEN_WIDTH-1-(300/UPDATE_INTERVAL); x>0; x-=300/UPDATE_INTERVAL) {
        uint16_t len = barLen(co2buffer[x]);
        for (int y=BAR_MAX_LEN; y<SCREEN_HEIGHT - len; y+=2) {
            display.drawPixel(x, y, WHITE);
        }
    }
    // Show text
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - BAR_MAX_LEN, BLACK);
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.printf("%d ppm", co2);
    display.setCursor(SCREEN_WIDTH * 3 / 4, 0);
    display.printf("%02.1f %%", humidity);

    // Wait
    delay(UPDATE_INTERVAL * 1000);
}