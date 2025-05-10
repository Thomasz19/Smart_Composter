# 🌱 Smart Composter - Arduino GIGA R1 WiFi Project

This project implements the firmware for a **Smart Composter System** using the **Arduino GIGA R1 WiFi** and the **GIGA Display Shield**. It monitors compost conditions, controls aeration motors, and displays relevant data through a multi-screen touchscreen interface built with **LVGL**.

---

## 🛠️ Features

- 📊 **Sensor Dashboard**  
  Displays temperature, humidity, CO₂, O₂, and other environmental metrics.

- 🚨 **Warnings & Alerts**  
  Real-time warning system for abnormal composting conditions (e.g., overheating, low oxygen).

- 🧠 **Motor & Fan Control**  
  Shows status of air circulation motors and allows user control or automation.

- 🕒 **Historical Data Logging**  
  Stores and displays logs of sensor readings over time.

- 🖥️ **Touchscreen Interface**  
  Built using LVGL for responsive, easy-to-navigate screens.

---

## 📦 Hardware Requirements

- Arduino GIGA R1 WiFi
- Arduino GIGA Display Shield
- Environmental sensors (e.g. DHT22, MQ-135, O₂ sensor, etc.)
- 12V Fans or Blowers
- Relay or MOSFET motor drivers
- Optional: SD card module or external EEPROM for data logging

---

## 📚 Libraries Used

- [`arduino_gigaDisplay`](https://github.com/arduino-libraries/arduino_gigaDisplay)
- [`arduino_gigaDisplayTouch`](https://github.com/arduino-libraries/arduino_gigaDisplayTouch)
- [`arduino_gigaDisplay_GFX`](https://github.com/arduino-libraries/arduino_gigaDisplay_GFX)
- [`arduinoGraphics`](https://github.com/arduino-libraries/arduinoGraphics)
- [`Adafruit GFX Library`](https://github.com/adafruit/Adafruit-GFX-Library)
- [`LVGL`](https://github.com/lvgl/lvgl)

---

## 🚀 Getting Started

1. **Clone the Repository**

   ```bash
   git clone https://github.com/yourusername/smart-composter.git
   cd smart-composter
