#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include "config.h"

// === CONFIG FLAGS ===
// Define one of these as true in config.h
// #define SIM_MODE_WIFI true
// #define SIM_MODE_USB false
// Define these in config.h
// #define WIFI_SSID "my_cool_network"
// #define WIFI_PASS "my_cool_password"

// === Hardware Pins (for real sensor mode) ===
const int CORE_TEMP_PIN = A0;
const int WATER_TEMP_PIN = A1;

// === Simulation Parameters ===
float T_core = 25.0;
float T_water = 25.0;
String mode = "IDLE";

const float HEAT_RATE = 0.3;
const float COOL_RATE = 0.25;

unsigned long lastSimTime = 0;
unsigned long lastSendTime = 0;

// === WiFi Setup ===
WiFiServer server(12345);
WiFiClient client;
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

// === Analog Sensor to Temperature ===
// Replace with real sensor's formula if using sensors
float analogToTemp(int analogValue) {
    float voltage = analogValue * (5.0 / 1023.0);
    float temperature = (voltage - 0.5) * 100.0;  // TMP36-style conversion
    return temperature;
}

// === Simulate Temperature ===
void simulateTemperatures() {
    float ambient = 25.0;

    if (mode == "HEAT") {
        T_core += HEAT_RATE * 0.6;
        T_water += HEAT_RATE;
    } else if (mode == "COOL") {
        T_core -= COOL_RATE * 0.6;
        T_water -= COOL_RATE;
    } else {
        // Natural equalization towards ambient
        T_core += (ambient - T_core) * 0.01;
        T_water += (ambient - T_water) * 0.01;
    }

    // Clamp within realistic range
    T_core = constrain(T_core, 0, 100);
    T_water = constrain(T_water, 0, 100);
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(100);
    }
    Serial.println("[Arduino] Boot complete");

#if SIM_MODE_WIFI
    Serial.print("[Arduino] Connecting to WiFi: ");
    Serial.println(ssid);
    
    // Initialize WiFi module
    if (WiFi.status() == 255) {
        Serial.println("[Arduino] Communication with WiFi module failed!");
        while (true);
    }
    
    // Check firmware version
    String fv = WiFi.firmwareVersion();
    Serial.print("[Arduino] Firmware version: ");
    Serial.println(fv);
    
    // Begin connection with proper initialization
    WiFi.disconnect();
    delay(1000);
    
    int status = WiFi.begin((char*)ssid, (char*)password);
    Serial.print("[Arduino] Initial connection status: ");
    Serial.println(status);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        Serial.print(WiFi.status());
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Arduino] WiFi connected.");
        Serial.print("[Arduino] IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("[Arduino] Signal strength: ");
        Serial.println(WiFi.RSSI());
        server.begin();
        Serial.println("[Arduino] Server started.");
    } else {
        Serial.println("\n[Arduino] Failed to connect to WiFi.");
        Serial.print("[Arduino] Final status: ");
        Serial.println(WiFi.status());
    }
#else
    Serial.println("[Arduino] USB simulation mode - skipping WiFi setup.");
#endif
}

void loop() {
    unsigned long now = millis();

    // Serial.print("[Arduino] Mode: ");
    // Serial.print(mode);
    // Serial.print(" | T_core: ");
    // Serial.print(T_core);
    // Serial.print(" | T_water: ");
    // Serial.println(T_water);

    // === Send only structured data for parsing:
    // String payload = "T_CORE:" + String(T_core, 1) +
    //                 ",T_WATER:" + String(T_water, 1) +
    //                 ",MODE:" + mode + "\n";
    // Serial.print(payload);

    // Accept new WiFi client (only in WiFi mode)
#if SIM_MODE_WIFI
    if (!client || !client.connected()) {
        client = server.available();
    }
#endif

    // Update temperatures every second
    if (now - lastSimTime > 1000) {
#if SIM_MODE_WIFI || SIM_MODE_USB
        simulateTemperatures();
#else
        T_core = analogToTemp(analogRead(CORE_TEMP_PIN));
        T_water = analogToTemp(analogRead(WATER_TEMP_PIN));
#endif
        lastSimTime = now;
    }

    // Send data
    if (now - lastSendTime > 1000) {
        String payload = "T_CORE:" + String(T_core, 1) +
                         ",T_WATER:" + String(T_water, 1) +
                         ",MODE:" + mode + "\n";

#if SIM_MODE_WIFI
        if (client && client.connected()) {
            client.print(payload);
        }
#endif

        Serial.print("[Arduino] Sent: ");
        Serial.println(payload);

        lastSendTime = now;
    }

    // Read commands (from client or Serial)
    String command = "";

#if SIM_MODE_WIFI
    if (client && client.available()) {
        command = client.readStringUntil('\n');
    }
#endif

#if SIM_MODE_USB
    if (Serial.available()) {
        command = Serial.readStringUntil('\n');
    }
#endif

    if (command.length() > 0) {
        command.trim();
        Serial.print("[Arduino] Received command: ");
        Serial.println(command);

        if (command.equalsIgnoreCase("heat")) {
            mode = "HEAT";
        } else if (command.equalsIgnoreCase("cool")) {
            mode = "COOL";
        } else if (command.equalsIgnoreCase("stop")) {
            mode = "IDLE";
        }
    }
}