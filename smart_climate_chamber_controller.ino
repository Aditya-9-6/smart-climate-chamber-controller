/*
 * =====================================================================
 * Smart Climate Chamber & Automated Dehumidification Controller
 * =====================================================================
 * Microcontroller: ESP32 / Arduino Compatible
 * 
 * Hardware Features:
 *   - Dual DHT22 Temperature & Humidity Sensors (Internal Chamber & External Ambient)
 *   - Automated Servo-Driven Air Flap Mechanism
 *   - PTC Heating Element & Relay Control
 *   - Intake Blower & Exhaust Fan Control
 *   - Humidity Stability Detection with Acoustic Alarm (Buzzer)
 *   - Foot Pedal Safety Traffic Light Sequence (Red -> Yellow -> Green)
 *   - System Power Pushbutton Control (ON / OFF)
 * 
 * Maintainer: Sarthak Jorvekar (GitHub: @jorvekarsarthak7-code)
 * Architecture & Review: Aditya Dahale (GitHub: @Aditya-9-6)
 * =====================================================================
 */

#include <DHT.h>
#include <Servo.h>

// =====================================================
// ================= PIN DEFINITIONS ===================
// =====================================================

// ---------- OUTER DHT22 ----------
#define DHTPIN 4
#define DHTTYPE DHT22

// ---------- INNER DHT22 ----------
#define INNER_DHTPIN 33

// ---------- EXISTING LEDs ----------
#define LED1 13
#define LED2 14
#define LED3 25

// ---------- BUZZER ----------
#define BUZZER 23

// ---------- PUSH BUTTONS ----------
#define BUTTON1 18
#define BUTTON2 19

// ---------- PEDAL / EXISTING LEDs ----------
#define RED_LED     13
#define YELLOW_LED  14
#define GREEN_LED   26

#define PEDAL_PIN   27

// =====================================================
// ================ NEW SERVO SYSTEM ===================
// =====================================================

#define SERVO_PIN   5
#define PTC_PIN     21
#define BLOWER_PIN  22
#define EXHAUST_PIN 32

// =====================================================

DHT outerDHT(DHTPIN, DHTTYPE);
DHT innerDHT(INNER_DHTPIN, DHTTYPE);

Servo flapServo;

// =====================================================
// ================= SYSTEM VARIABLES ==================
// =====================================================

bool systemEnabled = true;

float previousHumidity = 0;

unsigned long stableStartTime = 0;
unsigned long lastDHTRead = 0;

bool humidityTimerStarted = false;

// =====================================================
// ================= SERVO VARIABLES ===================
// =====================================================

const int SERVO_OPEN  = 90;
const int SERVO_CLOSE = 0;

bool highOutsideHumidity = false;
bool flushingCycle = false;

unsigned long flushStartTime = 0;
const unsigned long FLUSH_TIME = 40000;   // 40 seconds

// =====================================================
// ================= TIMING & THRESHOLDS ===============
// =====================================================

const unsigned long DHT_INTERVAL = 2000;   // Read sensors every 2 seconds
const unsigned long STABLE_TIME = 1000000; // Stability time limit
const float HUMIDITY_TOLERANCE = 1.0;     // Stability delta threshold in %

// =====================================================
// ================= SETUP ==============================
// =====================================================

void setup() {
  Serial.begin(9600);

  outerDHT.begin();
  innerDHT.begin();

  // Existing LEDs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // Buzzer
  pinMode(BUZZER, OUTPUT);

  // Buttons
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  // Pedal
  pinMode(PEDAL_PIN, INPUT_PULLUP);

  // Existing traffic LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Actuator Relay Outputs
  pinMode(PTC_PIN, OUTPUT);
  pinMode(BLOWER_PIN, OUTPUT);
  pinMode(EXHAUST_PIN, OUTPUT);

  // Servo Initialization
  flapServo.attach(SERVO_PIN);

  // ===================================================
  // INITIAL CONDITIONS
  // ===================================================
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  digitalWrite(BUZZER, LOW);

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  digitalWrite(PTC_PIN, LOW);
  digitalWrite(BLOWER_PIN, HIGH);
  digitalWrite(EXHAUST_PIN, LOW);

  // Normal condition = flap OPEN
  flapServo.write(SERVO_OPEN);

  Serial.println("=================================");
  Serial.println("SMART CLIMATE CHAMBER READY");
  Serial.println("MODE: NORMAL VENTILATION");
  Serial.println("=================================");
}

// =====================================================
// ================= MAIN LOOP ==========================
// =====================================================

void loop() {

  // ===================================================
  // BUTTON 1 → SYSTEM OFF
  // ===================================================
  if (digitalRead(BUTTON1) == LOW) {
    systemEnabled = false;

    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(BUZZER, LOW);

    digitalWrite(PTC_PIN, LOW);
    digitalWrite(BLOWER_PIN, LOW);
    digitalWrite(EXHAUST_PIN, LOW);

    humidityTimerStarted = false;
    previousHumidity = 0;

    Serial.println("SYSTEM STATUS: DISABLED (OFF)");
    delay(50); // Debounce
  }

  // ===================================================
  // BUTTON 2 → SYSTEM ON
  // ===================================================
  if (digitalRead(BUTTON2) == LOW) {
    systemEnabled = true;

    digitalWrite(BUZZER, LOW);

    humidityTimerStarted = false;
    previousHumidity = 0;

    // Return to normal ventilation
    flapServo.write(SERVO_OPEN);

    digitalWrite(BLOWER_PIN, HIGH);
    digitalWrite(EXHAUST_PIN, LOW);
    digitalWrite(PTC_PIN, LOW);

    flushingCycle = false;

    Serial.println("SYSTEM STATUS: ENABLED (ON)");
    delay(50); // Debounce
  }

  // ===================================================
  // DHT22 SENSOR MONITORING & ACTUATION
  // ===================================================
  if (millis() - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = millis();

    // -------------------------------------------------
    // OUTER DHT22 READINGS
    // -------------------------------------------------
    float outerTemperature = outerDHT.readTemperature();
    float outerHumidity = outerDHT.readHumidity();

    // -------------------------------------------------
    // INNER DHT22 READINGS
    // -------------------------------------------------
    float innerTemperature = innerDHT.readTemperature();
    float innerHumidity = innerDHT.readHumidity();

    // Check Outer Sensor
    if (isnan(outerTemperature) || isnan(outerHumidity)) {
      Serial.println("[WARN] Outer DHT22 sensor read failed!");
    } else {
      Serial.print("OUTER -> Temp: ");
      Serial.print(outerTemperature);
      Serial.print(" °C | Humidity: ");
      Serial.print(outerHumidity);
      Serial.print(" %    ");
    }

    // Check Inner Sensor
    if (isnan(innerTemperature) || isnan(innerHumidity)) {
      Serial.println("[WARN] Inner DHT22 sensor read failed!");
    } else {
      Serial.print("INNER -> Temp: ");
      Serial.print(innerTemperature);
      Serial.print(" °C | Humidity: ");
      Serial.print(innerHumidity);
      Serial.println(" %");
    }

    // =================================================
    // ACTIVE CONTROL LOGIC
    // =================================================
    if (systemEnabled) {

      // LED 1: Low Ambient Temperature Warning (< 38.0 °C)
      if (outerTemperature < 38.0) {
        digitalWrite(LED1, HIGH);
      } else {
        digitalWrite(LED1, LOW);
      }

      // LED 2: High Ambient Humidity Warning (> 55.0 %)
      if (outerHumidity > 55.0) {
        digitalWrite(LED2, HIGH);
      } else {
        digitalWrite(LED2, LOW);
      }

      // LED 3: System Running Heartbeat
      digitalWrite(LED3, HIGH);

      // =================================================
      // HUMIDITY STABILITY CHECK & BUZZER ALERT
      // =================================================
      if (previousHumidity == 0) {
        previousHumidity = outerHumidity;
        stableStartTime = millis();
        humidityTimerStarted = true;
      }

      float difference = abs(outerHumidity - previousHumidity);

      if (difference <= HUMIDITY_TOLERANCE) {
        if (!humidityTimerStarted) {
          stableStartTime = millis();
          humidityTimerStarted = true;
        }

        if (millis() - stableStartTime >= STABLE_TIME) {
          digitalWrite(BUZZER, HIGH);
          Serial.println("[ALERT] Ambient humidity constant for threshold duration -> BUZZER ACTIVE");
        }
      } else {
        humidityTimerStarted = false;
        stableStartTime = millis();
        digitalWrite(BUZZER, LOW);
        Serial.println("[INFO] Ambient humidity shifted -> Timer reset");
      }

      previousHumidity = outerHumidity;

      // =================================================
      // AUTOMATED SERVO & AIR EXCHANGE LOGIC
      // =================================================
      if (!isnan(outerHumidity) && !isnan(innerHumidity)) {

        // CASE A: HIGH OUTSIDE HUMIDITY (> 80.0%)
        if (outerHumidity > 80.0) {
          if (!highOutsideHumidity) {
            highOutsideHumidity = true;
            Serial.println("=========================================");
            Serial.println("EVENT: Outside Humidity > 80%");
            Serial.println("ACTION: FLAP -> CLOSED | BLOWER -> OFF | PTC -> ON");
            Serial.println("=========================================");
          }

          // Seal chamber and engage PTC heating
          flapServo.write(SERVO_CLOSE);
          digitalWrite(BLOWER_PIN, LOW);
          digitalWrite(PTC_PIN, HIGH);

          // SUB-CASE: Chamber internal humidity reaches >= 85%
          if (innerHumidity >= 85.0 && !flushingCycle) {
            flushingCycle = true;
            flushStartTime = millis();

            Serial.println("-----------------------------------------");
            Serial.println("EVENT: Inner Humidity >= 85%");
            Serial.println("ACTION: 40-SECOND AIR PURGE CYCLE STARTED");
            Serial.println("ACTION: FLAP -> OPEN | BLOWER -> ON | EXHAUST -> ON");
            Serial.println("-----------------------------------------");

            flapServo.write(SERVO_OPEN);
            digitalWrite(BLOWER_PIN, HIGH);
            digitalWrite(EXHAUST_PIN, HIGH);
          }

          // Complete 40-second air purge cycle
          if (flushingCycle) {
            if (millis() - flushStartTime >= FLUSH_TIME) {
              flushingCycle = false;

              // Re-seal chamber, stop purge fans
              flapServo.write(SERVO_CLOSE);
              digitalWrite(BLOWER_PIN, LOW);
              digitalWrite(EXHAUST_PIN, LOW);
              digitalWrite(PTC_PIN, HIGH); // PTC heater remains active

              Serial.println("-----------------------------------------");
              Serial.println("EVENT: 40-Second Air Purge Completed");
              Serial.println("ACTION: FLAP -> RE-CLOSED | EXHAUST -> OFF");
              Serial.println("-----------------------------------------");
            }
          }
        } 
        // CASE B: NORMAL OUTSIDE HUMIDITY (<= 80.0%)
        else {
          if (highOutsideHumidity) {
            Serial.println("=========================================");
            Serial.println("EVENT: Outside Humidity returned <= 80%");
            Serial.println("ACTION: Restoring Normal Ventilation Mode");
            Serial.println("=========================================");
            highOutsideHumidity = false;
          }

          flushingCycle = false;
          flapServo.write(SERVO_OPEN);
          digitalWrite(BLOWER_PIN, HIGH);
          digitalWrite(EXHAUST_PIN, LOW);
          digitalWrite(PTC_PIN, LOW);
        }
      }
    } 
    // SYSTEM DISABLED STATE
    else {
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(BUZZER, LOW);

      digitalWrite(PTC_PIN, LOW);
      digitalWrite(BLOWER_PIN, LOW);
      digitalWrite(EXHAUST_PIN, LOW);

      flushingCycle = false;
    }
  }

  // ===================================================
  // PEDAL SEQUENCE (SAFETY / TRAFFIC LIGHT SYSTEM)
  // ===================================================
  if (digitalRead(PEDAL_PIN) == LOW) {
    // 1. RED PHASE
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    delay(1200);

    // 2. YELLOW PHASE
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    delay(1000);

    // 3. GREEN PHASE (Active while pedal is held down)
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);

    while (digitalRead(PEDAL_PIN) == LOW) {
      delay(10);
    }

    // PEDAL RELEASED
    digitalWrite(GREEN_LED, LOW);
  }
}
