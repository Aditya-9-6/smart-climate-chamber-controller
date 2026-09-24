# Smart Climate Chamber & Automated Dehumidification Controller 🌬️🌡️

An intelligent, microcontroller-based environmental control system engineered for climate chambers, enclosed testing booths, and automated dehumidification enclosures. 

The system utilizes dual DHT22 high-precision sensors, servo-actuated airflow damper flaps, PTC heating elements, forced-air blower intake, and forced-air exhaust evacuation to maintain tight internal climate regulation under fluctuating external ambient conditions.

---

## 👨‍💻 Author & Maintainer

- **Developer & Maintainer:** **Sarthak Jorvekar** ([@jorvekarsarthak7-code](https://github.com/jorvekarsarthak7-code))  
  *Second Year Mechanical Engineer, Amrutvahini College of Engineering (AVCOE)*

---

## 📌 Key System Features

1. **Dual Environmental Sensing (Internal & External):**
   - Continuously monitors ambient environmental humidity/temperature and internal chamber microclimate.
2. **Automated Flap & Airflow Damper Control:**
   - 90° Servo mechanism seals or ventilates the chamber based on ambient conditions.
3. **High Ambient Humidity Isolation Mode:**
   - When external humidity exceeds **80%**, the intake flap automatically closes, the blower shuts down to prevent damp air intake, and the **PTC heater engages** to prevent condensation.
4. **Automated Internal Air-Purge Cycle:**
   - If internal humidity rises to **85%** during isolation, an automated **40-second high-velocity air exchange cycle** triggers (Flap opens, Intake Blower engages, Exhaust Fan activates) to rapidly evacuate moisture.
5. **Humidity Stability Detection & Acoustic Alert:**
   - Tracks humidity rate of change. If relative humidity remains constant within ±1.0% for extended operation, a piezo buzzer alerts the operator.
6. **Foot Pedal Traffic Light Interlock Sequence:**
   - Integrated industrial pedal trigger executing a safety sequencing cycle (`RED` 1200ms ➔ `YELLOW` 1000ms ➔ `GREEN` maintained while depressed).

---

## 🔌 Hardware Pinout Mapping

| Component | Pin / GPIO | Type | Function / Description |
| :--- | :---: | :---: | :--- |
| **Outer DHT22** | `GPIO 4` | Digital Input | External ambient temperature & humidity sensor |
| **Inner DHT22** | `GPIO 33` | Digital Input | Chamber internal temperature & humidity sensor |
| **Flap Servo Motor** | `GPIO 5` | PWM Output | Servo controlling air damper flap (0° Closed / 90° Open) |
| **PTC Heater Relay** | `GPIO 21` | Digital Output | Solid State Relay / Driver for PTC heating element |
| **Blower Fan Relay** | `GPIO 22` | Digital Output | Forced intake blower control |
| **Exhaust Fan Relay**| `GPIO 32` | Digital Output | Forced extraction exhaust fan control |
| **Button 1 (System OFF)**| `GPIO 18` | Digital Input (Pullup) | Emergency / Manual system stop |
| **Button 2 (System ON)** | `GPIO 19` | Digital Input (Pullup) | System initialization and resume |
| **Foot Pedal Switch**| `GPIO 27` | Digital Input (Pullup) | Operator foot pedal safety trigger |
| **Piezo Buzzer** | `GPIO 23` | Digital Output | Acoustic alarm for stability and alerts |
| **Red LED** | `GPIO 13` | Digital Output | Safety sequence Red / Temp indicator |
| **Yellow LED** | `GPIO 14` | Digital Output | Safety sequence Yellow / Humidity warning |
| **Green LED** | `GPIO 26` | Digital Output | Safety sequence Green (Pedal engaged) |
| **Status LED 3** | `GPIO 25` | Digital Output | System active heartbeat indicator |

---

## 🔄 Operational Logic & State Flow

```
                      +-----------------------------+
                      |   System Power (Button 2)   |
                      +--------------+--------------+
                                     |
                                     v
                      +-----------------------------+
                      |  Normal Mode: Flap OPEN     |
                      |  Blower ON, Exhaust OFF     |
                      +--------------+--------------+
                                     |
                [ Outer Humidity > 80%? ]
               /                         \
             YES                          NO
             /                              \
            v                                v
+-----------------------------+     +-----------------------------+
| High Humidity Isolation:    |     | Maintain Normal Ventilation |
| - Flap CLOSED (0°)          |     | - Flap OPEN (90°)           |
| - Blower OFF                |     | - Blower ON, PTC OFF        |
| - PTC Heater ON             |     +-----------------------------+
+--------------+--------------+
               |
      [ Inner Humidity >= 85%? ]
               |
              YES
               v
+-------------------------------------------+
| 40-Second Automated Purge Cycle:          |
| - Flap OPEN (90°)                         |
| - Intake Blower ON + Exhaust Fan ON       |
| - Timer: 40,000 ms                        |
| - On Completion -> Return to Isolation    |
+-------------------------------------------+
```

---

## 🛠️ Software & Library Requirements

Ensure the following libraries are installed via the Arduino IDE Library Manager:
- **`DHT sensor library`** by Adafruit
- **`Adafruit Unified Sensor`**
- **`ESP32Servo`** (or standard `Servo` library for AVR/ARM boards)

---

## 🚀 Getting Started

1. **Clone the repository:**
   ```bash
   git clone https://github.com/Aditya-9-6/smart-climate-chamber-controller.git
   ```
2. Open `smart_climate_chamber_controller.ino` in Arduino IDE or VS Code (with PlatformIO).
3. Select your development board (e.g., `ESP32 Dev Module`).
4. Select the matching COM port.
5. Click **Verify**, then **Upload**.
6. Open **Serial Monitor** at `9600 baud` to view live telemetry and state changes.

---

## 📜 License
Distributed under the **MIT License**. Free for educational, academic, and research applications.
