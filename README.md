# 🌱 Smart Composter - Arduino GIGA R1 WiFi Project

This project implements the firmware for a **Smart Composter System** using the **Arduino GIGA R1 WiFi** and the **GIGA Display Shield**. It monitors compost conditions, controls aeration motors, and displays relevant data through a multi-screen touchscreen interface built with **LVGL**.

---

## Features

- **Sensor Dashboard**  
  Displays temperature, humidity, O₂, and other environmental metrics.

- **Warnings & Alerts**  
  Real-time warning system for abnormal composting conditions (e.g., overheating, Door Open).

- **Pump & Blower Control**  
  Shows status of air circulation motors and allows user control or automation.

- **Touchscreen Interface**  
  Built using LVGL for responsive, easy-to-navigate screens.

---

## Hardware Requirements

- Arduino GIGA R1 WiFi
- Arduino GIGA Display Shield
- Raspberry Pi Zero 2 W
- Environmental sensors
- 120VAC 1 Pump and 2 Blowers

---

## Libraries Used

- [`arduino_gigaDisplay`](https://github.com/arduino-libraries/arduino_gigaDisplay)
- [`arduino_gigaDisplayTouch`](https://github.com/arduino-libraries/arduino_gigaDisplayTouch)
- [`arduino_gigaDisplay_GFX`](https://github.com/arduino-libraries/arduino_gigaDisplay_GFX)
- [`arduinoGraphics`](https://github.com/arduino-libraries/arduinoGraphics)
- [`Adafruit GFX Library`](https://github.com/adafruit/Adafruit-GFX-Library)
- [`LVGL`](https://github.com/lvgl/lvgl)

---

## Getting Started w/ Arduino Giga R1 Wifi

1. **Clone the Repository**

   ```bash
   git clone https://github.com/yourusername/Smart_Composter.git
   cd Smart_Composter
   ```
2. **Open with PlatformIO in VSCode**

   ```bash
   Install Required Libraries
   PlatformIO should auto-install them via platformio.ini, but check lib_deps if needed.
   ```

3. **Build & Upload**

   ```bash
   Select the Arduino GIGA R1 WiFi board and upload via USB.
   ```
## Getting Started w/ Raspberry Pi Zero 2 W
  ```bash
   Download the file named "SmartComposter_RaspberryPi.py"
   ```
## Authors

**Thomas Zoldowski** – Computer Engineering Student at Grand Valley State University  
