#include <Arduino.h>
#include <Wire.h>

#include <SparkFun_SCD30_Arduino_Library.h>

SCD30 airSensor;


// Pin definitions
#define LED_RED     D6
#define LED_YELLOW  D7
#define LED_GREEN   D5
#define I2C_SCL     D1
#define I2C_SDA     D2

// Number of seconds between measurements
#define UPDATE_INTERVAL 10


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
    delay(500);
    
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
}


void setup() {
    Serial.begin(115200);

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


void loop() {
    if (airSensor.dataAvailable()) {
        uint16_t co2 = airSensor.getCO2();

        digitalWrite(LED_RED, (co2 >= 1500) ? HIGH : LOW);
        digitalWrite(LED_YELLOW, (co2 > 1000 && co2 < 1500) ? HIGH : LOW);
        digitalWrite(LED_GREEN, (co2 <= 1000) ? HIGH : LOW);
        

        Serial.print("co2(ppm): ");
        Serial.print(co2);

        Serial.print(" temp(C): ");
        Serial.print(airSensor.getTemperature(), 1);

        Serial.print(" humidity(%): ");
        Serial.print(airSensor.getHumidity(), 1);

        Serial.println();
    }
    delay(UPDATE_INTERVAL * 1000);
}