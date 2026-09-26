# Smart Classroom Energy Management System

A computer-vision-based system that automatically controls a classroom's electrical load based on whether people are present.

## What It Does

* Camera detects people using YOLOv8.
* If people are present, the relay keeps the connected load ON.
* If the room is empty, the relay automatically turns the load OFF.
* A phone-based dashboard allows monitoring and manual control.
* An OLED display shows the current room status.

## How It Works

A laptop runs a Python program using YOLOv8 to detect and count people through a camera.

The Python program sends the occupancy information to an ESP32 over Wi-Fi. The ESP32 processes the commands and controls a relay connected to the simulated classroom load.

## Hardware Used

* ESP32
* Relay Module
* OLED Display (SSD1306 SPI)
* Light Bulb for simulating the electrical load
* Jumper Wires

## Software Used

* Python
* YOLOv8 (Ultralytics)
* OpenCV
* Arduino IDE
* ESPAsyncWebServer
* Adafruit SSD1306 Library

## Setup

### ESP32

1. Install the required libraries in Arduino IDE.
2. Upload the ESP32 code.
3. The ESP32 creates a Wi-Fi hotspot named `SmartClassroom`.

### Python

Install the required Python dependencies:

```bash
pip install ultralytics opencv-python requests
```

Run the Python program:

```bash
python main.py
```

### Dashboard

Connect a phone to the `SmartClassroom` Wi-Fi network and open:

```text
192.168.4.1
```

in a browser.

## Features

* Automatic people detection using YOLOv8
* Automatic relay control based on occupancy
* Manual override from a phone dashboard
* Force ON / Force OFF controls
* Resume Auto option
* OLED display for room status
* WebSocket-based real-time dashboard updates

## Limitations

* Requires a laptop with a camera for computer-vision processing.
* Camera performance can be affected by poor lighting.
* A single camera may have blind spots in a larger room.
* For real-world deployment, a ceiling-mounted wide-FOV camera and a dedicated edge-computing device could replace the laptop.

## Built During

4-week Embedded Systems and IoT Training Program — 2nd Year Summer Break.
