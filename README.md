# Smart Climate Control System

An embedded smart climate-control prototype that monitors temperature, humidity, and occupancy to automate fan behavior and display real-time environmental data.

## Overview

This project uses an STM32F401 sensor node to collect data from a DHT11 temperature/humidity sensor and an IR occupancy sensor. The system controls a DC motor fan based on room conditions and occupancy, with manual control through push buttons and local LCD feedback.

The system also uses CAN Bus communication through MCP2515 modules to send live readings from the STM32 node to an ESP32 receiver for dashboard or IoT monitoring integration.

## Features

- Real-time temperature and humidity monitoring
- Occupancy detection using IR sensor
- Automatic fan-off logic when the area is empty
- Manual fan-speed control using push buttons
- LCD display for local system feedback
- CAN Bus communication between STM32 and ESP32
- ESP32 receiver for future web/mobile dashboard integration

## Technologies

- STM32F401RCT6
- ESP32
- Embedded C
- Arduino IDE
- STM32CubeMX
- STM32CubeIDE
- CAN Bus
- MCP2515
- DHT11 Sensor
- IR Sensor
- LCD 16x2
- DC Motor

## Project Structure

- `STM32_SensorNode/`  
  STM32CubeIDE project containing the main embedded sensor-node code.

- `ESP32_Receiver/`  
  Arduino sketch for ESP32 CAN receiver.

## Hardware Components

- STM32F401RCT6
- ESP32 Dev Module
- MCP2515 CAN Module
- DHT11 Temperature and Humidity Sensor
- IR Occupancy Sensor
- 16x2 LCD
- DC Motor Fan
- Push Buttons
- Jumper wires and breadboard

## Future Work

- Add Firebase or cloud dashboard integration
- Add mobile app monitoring
- Improve HVAC automation logic
- Add multi-room support
- Add historical data logging and analytics
