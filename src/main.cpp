/**
 * @file main.cpp
 * @brief MQTT Client for LEXI-R10 LTE Module
 * 
 * This sketch demonstrates how to connect an Arduino UNO R4 to an MQTT broker
 * using the TinyGSM library with a Trasna LEXI-R10 shield.
 * 
 * Hardware Requirements:
 * - Arduino UNO R4 (or compatible board)
 * - Trasna LEXI/ADAM shield for Arduino UNO / UNO Q
 * - LED Matrix (optional) for visual feedback
 * 
 * Features:
 * - LTE connection via TinyGSM
 * - MQTT client using ArduinoMqttClient library
 * - Subscribe to MQTT topics and control LED matrix
 * - TLS/SSL support for secure connections
 * 
 * Copyright (c) 2026 Trasna
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the “Software”), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 */

//==============================================================================
// INCLUDES
//==============================================================================

#include <Arduino.h>
#include <StreamDebugger.h>
#include <ArduinoMqttClient.h>  // Generic MQTT library for Arduino
#include "Arduino_LED_Matrix.h" // LED Matrix library for visual feedback


//==============================================================================
// PIN CONFIGURATION FOR SHIELD CONTROL
//==============================================================================

// Shield Power Supply Enable Pin
#define SHIELD_VCC_EN (A0)

// Modem Power On Pin
#define MODEM_PWRON   (7u)

// Modem Reset Control Pin 
#define MODEM_RESET   (3u)

//==============================================================================
// MODEM CONFIGURATION
//==============================================================================

// Select the modem type - using Trasna SARA-R4 series as compatible to Trasna LEXI-R10
#define TINY_GSM_MODEM_SARAR4

//==============================================================================
// SERIAL CONFIGURATION
//==============================================================================

// Serial monitor for debug output (USB Serial, default speed 115200)
#define SerialMon Serial

// Serial1 configuration for LTE module communication
// On UNO R4, Serial1 is available on pins 0 (RX) and 1 (TX)
#define SerialAT Serial1

// Uncomment to dump all AT commands to Serial Monitor for debugging
//#define DUMP_AT_COMMANDS

// Uncomment to enable TinyGSM debug prints
//#define TINY_GSM_DEBUG SerialMon

//==============================================================================
// GPRS/CELLULAR NETWORK CONFIGURATION
//==============================================================================

// APN (Access Point Name) for your cellular provider
// WARNING: Replace with your actual carrier APN, (e.g.:"web.omnitel.it")
const char apn[]      = "";

// GPRS authentication credentials (leave empty if not required)
const char user[]     = "";
const char pass[]     = "";

// GSM SIM card PIN (leave empty if no PIN is required)
#define GSM_PIN ""

//==============================================================================
// TINYGSM LIBRARY
//==============================================================================

#include <TinyGsmClient.h>  // LTE modem driver library

//==============================================================================
// MQTT CONFIGURATION
//==============================================================================

// MQTT Broker connection settings
// NOTE: IoT broker hostnames should be replaced with your specific broker
const char broker_name[]       = "io.adafruit.com";
const unsigned int broker_port = 1883;

//==============================================================================
// SENSITIVE DATA - TO BE REPLACED WITH GENERIC PLACEHOLDERS
//==============================================================================
// WARNING: The following credentials contain sensitive data!
// Please replace with generic placeholders before committing to version control:
// - MQTT username: Replace with "YOUR_MQTT_USERNAME"
// - MQTT password: Replace with "YOUR_MQTT_PASSWORD" or "aio_XXXXXXXXXXXXXXXXXXXX"
// - MQTT topic: Replace with "yourusername/feeds/yourfeed"
//==============================================================================

const char mqttUser[] = "YOUR_MQTT_USERNAME";   // MQTT username (REPLACE WITH YOUR ACTUAL VALUE)
const char mqttPass[] = "YOUR_MQTT_PASSWORD";  // MQTT password/token (REPLACE WITH YOUR ACTUAL VALUE)

// MQTT topic to subscribe to for receiving commands
// Format: "username/feedname"
const char mqttInTopic[]  = "YOUR_MQTT_USERNAME/feeds/led-control";  // IN Topic path (REPLACE WITH YOUR USERNAME)
const char mqttOutTopic[] = "YOUR_MQTT_USERNAME/feeds/signal-strength"; // OUT Topic path (REPLACE WITH YOUR USERNAME)

// QoS (Quality of Service) level for MQTT subscriptions
// 0 = At most once (fire and forget)
// 1 = At least once (acknowledged delivery)
// 2 = Exactly once (assured delivery)
const unsigned int mqttqos = 0;

//==============================================================================
// HARDWARE INITIALIZATION
//==============================================================================

// Create LED matrix instance for visual feedback demonstration
ArduinoLEDMatrix matrix;

// Pre-defined pattern: all LEDs on
const uint32_t fullOn[] = {
  0xffffffff,
  0xffffffff,
  0xffffffff
};

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// Conditionally create modem object with or without debug logging
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

// TinyGSM client instance for LTE modem communication
TinyGsmClient client(modem);

// MQTT client wrapper around TinyGSM client
MqttClient mqttClient(client);

// Forward declaration of MQTT message handler callback
void onMqttMessage(int messageSize);

bool passthroughRequested = false;

//==============================================================================
// SETUP FUNCTION - Runs once at board startup
//==============================================================================

void setup() {

  // Configure Shield/Modem control pins and set them to default state
  pinMode(SHIELD_VCC_EN, OUTPUT);
  digitalWrite(SHIELD_VCC_EN, LOW);
  pinMode(MODEM_PWRON,   OUTPUT);  
  digitalWrite(MODEM_PWRON,  HIGH);
  pinMode(MODEM_RESET,   OUTPUT);
  digitalWrite(MODEM_RESET,  HIGH);
  delay(500);

  // Initialize LED matrix
  matrix.begin();
  matrix.clear();

  // Start serial interfaces
  SerialMon.begin(115200);    // USB serial for debug output
  SerialAT.begin(115200);  // Hardware serial for LTE module communication

  // Print Powering on message
  SerialMon.println("Powering on LEXI board and modem...");

  // Power On Shield and LTE module
  digitalWrite(SHIELD_VCC_EN, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWRON,  LOW);
  delay(1000);
  digitalWrite(MODEM_PWRON,  HIGH);

  // Wait for LTE module to fully boot up
  delay(3000);

  SerialMon.println("Press 'c' within 5 secs to enter passthrough mode...");
  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    if (SerialMon.available() > 0) {
      char c = SerialMon.read();
      if (c == 'c' || c == 'C') {
        passthroughRequested = true;
        break;
      }
    }
  }

  if (passthroughRequested) {
    SerialMon.println("\n=== PASSTHROUGH MODE ===");
    return;
  }

  // Application setup

  // Print initialization message
  SerialMon.println("Initializing LEXI-R10 modem...");

  // Restart/initialize the modem
  if (!modem.restart()) {
    SerialMon.println("Modem restart failed!");
    matrix.loadFrame(LEDMATRIX_EMOJI_SAD);  // Display sad face on error
    while (true) delay(100);  // Halt execution on failure
  }

  // Retrieve and display modem information
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: "); SerialMon.println(modemInfo);

  String name = modem.getModemName();
  SerialMon.print("Modem Name: "); SerialMon.println(name);

  String manufacturer = modem.getModemManufacturer();
  SerialMon.print("Modem Manufacturer: "); SerialMon.println(manufacturer);

  String hw_ver = modem.getModemModel();
  SerialMon.print("Modem Hardware Version: "); SerialMon.println(hw_ver);

  String fv_ver = modem.getModemRevision();
  SerialMon.print("Modem Firmware Version: "); SerialMon.println(fv_ver);

  String mod_sn = modem.getSimCCID();
  SerialMon.print("SIM CCID (or modem serial): "); SerialMon.println(mod_sn);

  // Unlock SIM card with PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { 
    modem.simUnlock(GSM_PIN); 
  }

  // Connect to cellular network
  SerialMon.print("Connecting to LTE network...");
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" FAILED");
    matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
    while (true) delay(100);
  }
  SerialMon.println(" SUCCESS!");

  // Configure TLS security profile on LEXI-R10
  // Security profile index 0 is configured for TLS (second parameter = 1)
  // Note: TinyGSM can auto-configure in some cases, but explicit config is recommended
  SerialMon.print("Configuring TLS profile... ");
  modem.sendAT("+USECPRF=0,0,1");  // Configure profile 0 for TLS
  if (modem.waitResponse() != 1) {
    SerialMon.println("TLS profile configuration error");
  }
  else {
    SerialMon.println("Success!");
  }

  // Configure MQTT credentials
  mqttClient.setUsernamePassword(mqttUser, mqttPass);

  // Attempt MQTT broker connection
  SerialMon.print("Connecting to MQTT broker...");
  if (!mqttClient.connect(broker_name, broker_port)) {
    SerialMon.print("MQTT Error: ");
    SerialMon.println(mqttClient.connectError());
    matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
    while (1) delay(100);
  }

  SerialMon.println("Connected!");

  // Inform user about encryption handling
  if (broker_port == 8883) {
    SerialMon.println("TLS encryption is handled by the LEXI-R10 module.");
  }

  // Register callback function for incoming MQTT messages
  mqttClient.onMessage(onMqttMessage);

  // Subscribe to the specified topic
  SerialMon.print("Subscribing to topic: ");
  SerialMon.println(mqttInTopic);
  SerialMon.println();
  mqttClient.subscribe(mqttInTopic, mqttqos);

  // Display waiting message
  SerialMon.print("Waiting for messages on topic: ");
  SerialMon.println(mqttInTopic);
  SerialMon.println();
}

//==============================================================================
// LOOP FUNCTION - Runs continuously after setup
//==============================================================================

void loop() {

  if (passthroughRequested) {
    // From PC to Modem
    if (SerialMon.available()) {
      SerialAT.write(SerialMon.read());
    }
    // From Modem to PC
    if (SerialAT.available()) {
      SerialMon.write(SerialAT.read());
    }
    return;
  }

  // Application loop

  // Poll for incoming MQTT messages (non-blocking)
  mqttClient.poll();
  
  // Track time for periodic status checks
  static unsigned long lastUpdate = 0;
  
  // Check every 20 seconds for network registration status
  if (millis() - lastUpdate > 20000) {
    lastUpdate = millis();

    // Registration status tracking
    static SaraR4RegStatus last_status = REG_NO_RESULT;
    static const char *statusStr[] {
      "UNREGISTERED ",  // 0 - Not registered, not searching
      "OK,HOME      ",  // 1 - Registered on home network
      "SEARCHING    ",  // 2 - Searching for network
      "DENIED       ",  // 3 - Registration denied
      "UNKNOWN      ",  // 4 - Unknown/unspecified
      "OK,ROAMING   "   // 5 - Registered on roaming network
    };
    
    // Get current registration status
    SaraR4RegStatus status = modem.getRegistrationStatus();
    
    // Print status if it changed
    if (status != last_status) {
      SerialMon.print("Modem registration status: ");
      SerialMon.println((status > 0) ? statusStr[status] : "UNDEFINED");
      last_status = status;
    }

    // Signal strength tracking
    modem.sendAT("+CSQ");  // Get RSSI level
    String csq_rsp;
    int index = modem.waitResponse(1000, csq_rsp);
    csq_rsp.replace("\r", "");
    csq_rsp.replace("\n", "");
    csq_rsp.trim();
    //SerialMon.println(csq_rsp.c_str());
    if ( index == 1) {
      int rssi = 0;
      int quality = 0;
      int scanned = sscanf(csq_rsp.c_str(), "+CSQ: %d,%d", &rssi, &quality);
      if ( scanned == 2){
        String payload = String(rssi)+","+String(quality);
        SerialMon.print("Model signal (RSSI,quality):");
        SerialMon.println(payload);

        // Publish signal and quality
        bool retained = false;
        int qos = 1;
        bool dup = false;
        mqttClient.beginMessage(mqttOutTopic, payload.length(), retained, qos, dup);
        mqttClient.print(payload);
        mqttClient.endMessage();
      }
      else {
        SerialMon.println(scanned);
      }

    }
    else {
      SerialMon.println(index);
    }

  }
}

//==============================================================================
// MQTT MESSAGE HANDLER CALLBACK
//==============================================================================

/**
 * @brief Callback function called when an MQTT message is received
 * 
 * This function handles incoming messages on subscribed topics.
 * Currently supports two commands:
 * - "LED_ON": Turns all LEDs on the matrix
 * - "LED_OFF": Clears the LED matrix
 * 
 * @param messageSize Size of the received message payload in bytes
 */
void onMqttMessage(int messageSize) {
  // Buffer for message content
  uint8_t buf[100] = {0};

  // Print message details
  SerialMon.print("Received a message with topic '");
  SerialMon.print(mqttClient.messageTopic());
  SerialMon.print("', length ");
  SerialMon.print(messageSize);
  SerialMon.println(" bytes:");

  // Limit message size to buffer capacity
  if (messageSize > sizeof(buf)) {
    messageSize = sizeof(buf) - 1;
  }
  
  // Read message payload into buffer
  if (mqttClient.read(buf, messageSize) > 0) {
    SerialMon.println((char*)buf);
    
    // Parse and execute commands
    if (strncmp((char*)buf, "LED_ON", messageSize) == 0) {
      // Turn on all LEDs
      matrix.loadFrame(fullOn);
    }
    else if (strncmp((char*)buf, "LED_OFF", messageSize) == 0) {
      // Turn off all LEDs (clear matrix)
      matrix.clear();
    }
  }

  SerialMon.println();
}
