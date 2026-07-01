# DEFCON Radar Scanner v4.1 📡

An interactive, ESP32-powered IoT radar system that combines hardware sensors with a high-performance, real-time web dashboard. The system handles live ambient telemetry, object tracking capabilities, and asynchronous WebSocket communication over a local network.



---

## 🚀 Features

*   **Real-Time Cyberpunk Dashboard:** A responsive, military-themed web UI broadcasting telemetry via WebSockets.
*   **Dynamic Telemetry:** Captures and pushes live temperature, humidity, and status updates every 2 seconds without blocking operations.
*   **Automated Tracking:** Integrates an IR proximity sensor to instantly flag local obstructions, locking the system and triggering an active visual/audible alarm framework.
*   **Asynchronous Architecture:** Built using `ESPAsyncWebServer` to ensure smooth web request handling independent of the hardware servo scanning loops.

---

## 🛠️ Hardware Component Stack

*   **Microcontroller:** ESP32-D0WD-V3 Development Board
*   **Actuator:** SG90 Micro Servo Motor
*   **Sensors:** DHT11 Climate Sensor, Infrared Proximity Sensor, Photoresistor (LDR)
*   **Alarms:** Active Buzzer, High-Bright Red LED
*   **Power Management:** HW-131 Breadboard Power Module (5V/3.3V rails)

---

## 🔌 Pin Mapping Configuration

| Component | ESP32 GPIO Pin | Description |
| :--- | :--- | :--- |
| **Servo Signal** | `GPIO 18` | Hardware PWM sweep control |
| **IR Sensor** | `GPIO 19` | Threat detection input (`LOW` = Target Spotted) |
| **Red LED** | `GPIO 2` | Active target visual indicator |
| **Buzzer** | `GPIO 4` | Target alert audible indicator |
| **DHT11 Sensor** | `GPIO 23` | Digital environmental data input |

---

## 🔧 Installation & Software Setup

### 1. Prerequisites
Ensure you have the following libraries installed via the Arduino IDE Library Manager:
*   `ESPAsyncWebServer`
*   `AsyncTCP`
*   `DHT sensor library` (Adafruit)
*   `ArduinoJson`

### 2. Linux Environment Configuration
If deploying via Flatpak on Linux (e.g., Ubuntu/Fedora), ensure the sandbox container has appropriate hardware and device tree permissions. Run the
following overrides in your terminal before flashing:



for working expectations of the radar watch video:https://drive.google.com/file/d/1VL09HgkcwVwQQXFvOs-QhSg-YGMHSd2G/view?usp=sharing


```bash
flatpak override --user --device=all cc.arduino.IDE2
flatpak override --user --filesystem=/dev cc.arduino.IDE2
