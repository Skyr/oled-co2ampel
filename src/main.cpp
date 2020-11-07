// Written by Stefan Schlott
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International


#include <Arduino.h>
#include <Wire.h>

#include <SparkFun_SCD30_Arduino_Library.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// OLED display size, in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
SCD30 airSensor;
uint16_t co2buffer[SCREEN_WIDTH+1];

// Pin definitions
#define LED_RED     D6
#define LED_YELLOW  D7
#define LED_GREEN   D5
#define I2C_SCL     D1
#define I2C_SDA     D2

// Altitude (in meters). Atmospheric pressure (and thus the measurement) drops by 3% for every ~300m!
#define ALTITUDE 445
// Number of seconds between measurements
#define UPDATE_INTERVAL 15

#define CO2_BAR_MIN 350
#define CO2_BAR_MAX 1700
#define YELLOW_THRESHOLD 1000
#define RED_THRESHOLD 1500


bool display_enabled = false;


void ledsOff() {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
}

void blink(int pin, int d) {
    digitalWrite(pin, HIGH);
    delay(d);
    digitalWrite(pin, LOW);
    delay(d);
}

void blinkFail(int pin) {
    while (true) {
        blink(pin, 500);
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
    if (ALTITUDE>0) {
        if (!airSensor.setAltitudeCompensation(ALTITUDE)) {
            Serial.println("Failed to set altitude compensation");
        }
    }

    // Init display
    if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C, true)) {
        display.clearDisplay();
        display.display();
        display_enabled = true;
    } else {
        Serial.println("Failed to initialize display");
        display_enabled = false;
    }

    // All good!
    Serial.println("Starting up, waiting for first data...");
    for (int i=0; i<3; i++) {
        blink(display_enabled ? LED_GREEN : LED_YELLOW, 300);
    }
    // Continue blinking until data available
    while (!airSensor.dataAvailable()) {
        blink(LED_GREEN, 300);
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

#define BAR_MAX_LEN (SCREEN_HEIGHT - 15)

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

    // If data not yet available (clock drift?): Wait a moment
    int retries = 5;
    while (!airSensor.dataAvailable() && retries>0) {
        delay(500);
    }

    // Read data
    if (airSensor.dataAvailable()) {
        co2 = airSensor.getCO2();
        humidity = airSensor.getHumidity();

        digitalWrite(LED_RED, (co2 >= RED_THRESHOLD) ? HIGH : LOW);
        digitalWrite(LED_YELLOW, (co2 > YELLOW_THRESHOLD && co2 < RED_THRESHOLD) ? HIGH : LOW);
        digitalWrite(LED_GREEN, (co2 <= YELLOW_THRESHOLD) ? HIGH : LOW);
        

        Serial.print("co2(ppm): ");
        Serial.print(co2);

        Serial.print("  temp(C): ");
        Serial.print(airSensor.getTemperature(), 1);

        Serial.print("  humidity(%): ");
        Serial.print(humidity, 1);

        Serial.println();
    }

    // Update data
    co2buffer[SCREEN_WIDTH] = co2;
    if (display_enabled) {
        // Update chart
        for (int i=0; i<SCREEN_WIDTH; i++) {
            uint16_t old_len = barLen(co2buffer[i]);
            uint16_t new_len = barLen(co2buffer[i+1]);
            if (old_len!=new_len) {
                if (old_len<new_len) {
                    display.drawFastVLine(i, SCREEN_HEIGHT - 1 - new_len, new_len-old_len+1, WHITE);
                } else {
                    display.drawFastVLine(i, SCREEN_HEIGHT - 1 - old_len, old_len-new_len+1, BLACK);
                }
            }
        }
    }
    // Roll buffer
    for (int i=0; i<SCREEN_WIDTH; i++) {
        co2buffer[i] = co2buffer[i+1];
    }
    if (display_enabled) {
        // Draw 5 min lines
        for (int x=SCREEN_WIDTH-1-(300/UPDATE_INTERVAL); x>0; x-=300/UPDATE_INTERVAL) {
            uint16_t len = barLen(co2buffer[x]);
            for (int y=SCREEN_HEIGHT - BAR_MAX_LEN; y<SCREEN_HEIGHT - len; y+=2) {
                display.drawPixel(x, y, WHITE);
            }
        }
        // Draw line for yellow warning
        for (int x=0; x<SCREEN_WIDTH; x+=2) {
            display.drawPixel(x, SCREEN_HEIGHT - 1 - barLen(YELLOW_THRESHOLD), WHITE);
        }
        // Draw line for red warning
        for (int x=0; x<SCREEN_WIDTH; x+=2) {
            display.drawPixel(x, SCREEN_HEIGHT - 1 - barLen(RED_THRESHOLD), WHITE);
        }
        // Show text
        display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - BAR_MAX_LEN, BLACK);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.setTextColor(WHITE);
        display.printf("%d ppm", co2);
        display.setCursor(SCREEN_WIDTH * 2 / 3, 0);
        display.printf("%02.1f %%", humidity);
        display.display();
    }

    // Wait
    delay(UPDATE_INTERVAL * 1000);
}