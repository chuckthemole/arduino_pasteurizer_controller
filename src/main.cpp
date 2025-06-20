#include <Arduino.h>
#include <WiFiNINA.h>
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
    if (mode == "HEAT") {
        T_water += HEAT_RATE;
        if (T_water > T_core) {
            T_core += HEAT_RATE * 0.9;
        }
    } else if (mode == "COOL") {
        T_water -= COOL_RATE;
        if (T_water < T_core) {
            T_core -= COOL_RATE * 0.8;
        }
    }

    // Clamp within 0-100°C
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
    
    int attempts = 0;
    int status = WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        attempts++;
        if (attempts > 10) {
            Serial.println("\n[Arduino] Failed to connect after 10 attempts.");
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Arduino] WiFi connected.");
        Serial.print("[Arduino] IP: ");
        Serial.println(WiFi.localIP());
        server.begin();
        Serial.println("[Arduino] Server started.");
    }
#else
    Serial.println("[Arduino] USB simulation mode - skipping WiFi setup.");
#endif
}

void loop() {
    unsigned long now = millis();

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
    String command;
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
