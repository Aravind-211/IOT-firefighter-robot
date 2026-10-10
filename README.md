# ESP32 Smart Safety & Monitoring System

An ESP32-based embedded monitoring and control project integrating environmental sensors, motor control, relay switching, and wireless telemetry.

## Features

- ESP32 DevKit as the central controller
- MQ-2 gas/smoke sensing
- Flame sensor inputs
- HC-SR04 ultrasonic distance measurement
- Motor control using an L298N driver
- Relay-based switching
- Wi-Fi telemetry
- PWM motor speed control using the Arduino-ESP32 LEDC API

## Hardware

- ESP32 DevKit
- MQ-2 gas sensor module
- Flame sensor modules
- HC-SR04 ultrasonic sensor
- L298N motor driver
- DC motors
- Relay module
- Regulated 5 V buck converter
- Appropriate power supply, wiring, and voltage-divider resistors

## Software Requirements

- Arduino IDE
- Arduino-ESP32 Core 3.x
- ESP32 board support package
- Wi-Fi credentials configured locally

The firmware uses `ledcAttach()` and `ledcWrite()` for PWM control and is intended for Arduino-ESP32 Core 3.x.

## Hardware Design

- MQ-2 analog signal: GPIO 36
- Flame sensor inputs: GPIO 34 and GPIO 35
- HC-SR04 echo: use a suitable voltage divider before connecting to the ESP32
- ESP32 supply: regulated 5 V through the board's 5 V input
- Shared ground: ESP32, sensor interfaces, motor-driver logic, and buck converter output

Verify the complete pin map against the firmware before wiring the hardware.

## Installation

1. Install Arduino IDE and ESP32 board support.
2. Clone this repository.
3. Open the firmware sketch.
4. Select the correct ESP32 board and serial port.
5. Configure Wi-Fi credentials in a local secrets file.
6. Verify and compile using Arduino-ESP32 Core 3.x.
7. Upload the firmware and inspect serial output.
8. Test sensors, motor control, relay operation, and Wi-Fi telemetry individually.

## Safety

This is an experimental embedded project, not a certified fire-protection system. Do not rely on it as the sole means of detecting fire or hazardous gas.

Never apply 5 V signals directly to ESP32 GPIO inputs. Check the MQ-2 analog output voltage, use appropriate level shifting, and power motors separately from the ESP32's logic supply as required by their specifications.

## License

See the LICENSE file.
