# Trasna LEXI-R10 Arduino MQTT Client

MQTT Client for Trasna LEXI-R10 shield.

This project demonstrates how to connect an Arduino UNO R4 to an MQTT broker using the TinyGSM library with a Trasna LEXI-R10 shield.

## Table of Contents
- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Software Dependencies](#software-dependencies)
- [Setup Instructions](#setup-instructions)
  - [Adafruit IO Account Setup](#adafruit-io-account-setup)
  - [Dashboard Creation](#dashboard-creation)
  - [Device Configuration](#device-configuration)
- [Testing the System](#testing-the-system)
- [Project Structure](#project-structure)
- [Security Notes](#security-notes)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Overview

This sketch shows how to:
1. Establish an LTE connection via TinyGSM using the LEXI-R10 module
2. Connect to an MQTT broker (Adafruit IO) using the ArduinoMqttClient library
3. Subscribe to MQTT topics to receive commands
4. Control an LED matrix based on received MQTT messages
5. Periodically publish signal strength and quality data
6. Use TLS/SSL for secure connections (handled by the LEXI-R10 module)

### Passthrough Mode

The project includes a passthrough mode feature that allows direct serial communication with the modem. During startup, press 'c' within 5 seconds to enter passthrough mode. In this mode, the Arduino acts as a serial-to-USB bridge, forwarding all data between the USB serial port and the LTE module's serial interface. This is useful for:
- Sending AT commands directly to the modem for debugging
- Testing modem configuration without uploading new sketches
- Monitoring modem responses in real-time.

To exit passthrough mode, reset the Arduino board.

## Hardware Requirements

- Arduino UNO R4 (or compatible board)
- Trasna LEXI/ADAM shield for Arduino UNO / UNO Q
- LED Matrix (optional) for visual feedback
- SIM card with data plan
- Antenna for LTE connectivity
- USB cable for power and debugging

## Software Dependencies

- PlatformIO VSCode Extension (for build system and library management)
- TinyGSM library (vshymanskyy/TinyGSM@^0.12.0)
- ArduinoMqttClient library (arduino-libraries/ArduinoMqttClient@^0.1.8)
- StreamDebugger library (vshymanskyy/StreamDebugger@^1.0.1)
- Arduino_LED_Matrix library (included with Arduino UNO R4 core)

These dependencies are automatically installed by PlatformIO when building the project.

## Setup Instructions

### Adafruit IO Account Setup

1. Go to [https://io.adafruit.com](https://io.adafruit.com) and create a free account
2. After logging in, click on "Adafruit IO Key" in the left sidebar
3. Note down your:
   - Username (shown as "Username")
   - Active Key (shown as "AIO Key")
4. These will be used as your MQTT credentials:
   - MQTT Username = Your Adafruit IO Username
   - MQTT Password = Your Adafruit IO AIO Key

### Dashboard Creation

1. In Adafruit IO, click on "Dashboards" in the top menu
2. Click "Actions" → "Create a new Dashboard"
3. Name your dashboard (e.g., "LEXI-R10 Monitor") and click "Create"
4. Create two feeds:
   - Click "Feeds" → "Actions" → "Create a new Feed"
   - Name: `led-control` (for receiving LED control commands)
   - Click "Create"
   - Repeat for: `signal-strength` (for publishing signal data)
5. Add blocks to your dashboard:
   - For the `led-control` feed:
     * Add a "Toggle" block or "Button" block
     * Set the block to publish "LED_ON" when turned on and "LED_OFF" when turned off
   - For the `signal-strength` feed:
     * Add a "Gauge" or "Chart" block to display the incoming signal data

### Device Configuration

1. Open `src/main.cpp` in your editor
2. Replace the following placeholder values with your actual Adafruit IO credentials:
   ```cpp
   const char mqttUser[] = "YOUR_ADAFRUIT_IO_USERNAME";   // Replace with your Adafruit IO username
   const char mqttPass[] = "YOUR_ADAFRUIT_IO_AIO_KEY";    // Replace with your Adafruit IO AIO key
   ```
3. Update the MQTT topics to match your username and feeds:
   ```cpp
   const char mqttInTopic[]  = "YOUR_ADAFRUIT_IO_USERNAME/feeds/led-control";  // Replace with your username
   const char mqttOutTopic[] = "YOUR_ADAFRUIT_IO_USERNAME/feeds/signal-strength"; // Replace with your username
   ```
4. Configure your cellular network APN:
   ```cpp
   const char apn[] = "YOUR_CELLULAR_APN"; // Replace with your carrier's APN
   ```
   - Common APNs: 
     - AT&T: "phone"
     - Verizon: "internet"
     - T-Mobile: "fast.t-mobile.com"
     - European carriers: often "internet" or "web"
   - If unsure, check with your carrier or try leaving it empty (some modules auto-detect)
5. If your SIM card requires a PIN, set it here:
   ```cpp
   #define GSM_PIN "YOUR_SIM_PIN" // Replace with your SIM PIN or leave empty if not required
   ```

## Testing the System

1. Connect your LEXI/ADAM shield to the Arduino UNO R4:
   - Connect the LTE antenna
   - Insert your SIM card

2. Connect the Arduino to your computer via USB

3. Build and upload the code using PlatformIO:
   - Click the PlatformIO icon in VS Code
   - Click "Build" to compile the code
   - Click "Upload" to flash the firmware to your board

4. Open the Serial Monitor (115200 baud) to view debug output

5. Expected behavior:
   - Board initializes and displays "Initializing LEXI-R10 modem..."
   - Modem restarts and connects to cellular network
   - Configures TLS profile for secure MQTT connection
   - Connects to Adafruit IO MQTT broker
   - Subscribes to the `led-control` feed
   - Publishes signal strength and quality every 20 seconds to `signal-strength` feed
   - LED matrix shows sad face on error, or waits for messages when connected

6. Test LED control:
   - In your Adafruit IO dashboard, toggle the switch/button for the `led-control` feed
   - You should see "LED_ON" or "LED_OFF" messages in the Serial Monitor
   - The LED matrix should turn on when "LED_ON" is received and clear when "LED_OFF" is received

7. Test signal publishing:
   - Check your Adafruit IO dashboard for the `signal-strength` feed
   - You should see periodic updates with RSSI and quality values (e.g., "23,99")

## Project Structure

```
R10-TinyGSM-ArduinoMqttClient/
├─ src/
│  └─ main.cpp          # Main sketch file
├─ platformio.ini       # PlatformIO project configuration
├─ .gitignore           # Git ignore file
└─ README.md            # This file
```

## Security Notes

**Important:** This project contains sensitive information that must be protected:

1. **MQTT Credentials**: Your Adafruit IO username and key provide access to your IoT data
2. **SIM PIN**: If used, protects your SIM card from unauthorized use

### Before Sharing or Committing Code:
- Replace all sensitive data with placeholders as shown in the setup instructions
- Never commit your actual credentials to version control
- Consider using secure storage methods for production deployments

### Current Protections in Place:
- The code includes comments marking sensitive data sections
- Default values in the repository are placeholders or generic examples

## Troubleshooting

### Common Issues:

1. **Modem Not Responding**
   - Check connection between Arduino and Trasna LEXI/ADAM shield
   - Try increasing the boot delay in `setup()` if the modem is slow to start

2. **Network Connection Failures**
   - Verify APN settings with your carrier
   - Check signal strength and antenna connection
   - Ensure SIM card is activated and has data plan
   - Try manually registering to network via AT commands

3. **MQTT Connection Problems**
   - Verify Adafruit IO username and key are correct
   - Check that your Adafruit IO account is active
   - Ensure port 1883 is not blocked by firewall (for non-TLS)
   - For TLS (port 8883), verify the LEXI-R10's TLS profile is configured

4. **No Messages Received**
   - Confirm you're subscribed to the correct topic format: `username/feeds/feedname`
   - Check that your dashboard blocks are publishing the expected payloads
   - Verify QoS levels match between publisher and subscriber

### Debugging Tips:
- Enable debug output by uncommenting:
  ```cpp
  #define TINY_GSM_DEBUG SerialMon
  #define DUMP_AT_COMMANDS
  ```
- Monitor the Serial Monitor for detailed AT command exchanges and error codes
- Check the LEXI-R10's signal strength with `AT+CSQ` command (values 0-31, 99=not detectable)

## License

This project is open source and available under the [MIT License](http://opensource.org/licenses/MIT).

## Acknowledgments

- TinyGSM library by Volodymyr Shymanskyy
- ArduinoMqttClient library by Arduino LLC
- Arduino UNO R4 platform by Arduino
- Trasna LEXI/ADAM shield for Arduino UNO / UNO Q
- Adafruit IO for IoT platform services

---
