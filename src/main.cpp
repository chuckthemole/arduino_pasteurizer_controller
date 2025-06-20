#include <Arduino.h>

#include <WiFiNINA.h>
#include <SPI.h>
#include "config.h"

// ======== CONFIG ========
#define USE_SIMULATION true   // <<< SET TO true FOR SIMULATION MODE

WiFiServer server(12345);
WiFiClient client;

// === Temp sensor pins (for production mode) ===
const int CORE_TEMP_PIN = A0;
const int WATER_TEMP_PIN = A1;

// === State ===
float T_core = 25.0;
float T_water = 25.0;
String mode = "IDLE";

float HEAT_RATE = 0.3;
float COOL_RATE = 0.25;

unsigned long lastSimTime = 0;
unsigned long lastSendTime = 0;

const char* ssid = WIFI_SSID;      // <-- exact name of your 2.4 GHz WiFi
const char* password = WIFI_PASS; // pass

// === Read temperature from analog (mock conversion for now) ===
float analogToTemp(int analogValue) {
    // You should replace this with your actual sensor's formula.
    float voltage = analogValue * (5.0 / 1023.0);
    float temperature = (voltage - 0.5) * 100.0; // Example for TMP36
    return temperature;
}

// === Simulated temperature model ===
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

    // Clamp
    T_core = constrain(T_core, 0, 100);
    T_water = constrain(T_water, 0, 100);
}

void setup() {
    Serial.begin(9600);  // or 9600, depending on your sketch
    // Especially needed for Uno R4 WiFi to wait for Serial connection
    while (!Serial)
    delay(1000);
    Serial.println("Boot complete");

    int status = WiFi.begin(ssid, password);

    Serial.print("Attempting to connect to ");
    Serial.println(ssid);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        attempts++;
        if (attempts > 10) {
        Serial.println("\nFailed to connect after 10 attempts.");
        break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Arduino] Connected to WiFi.");
        Serial.print("[Arduino] IP address: ");
        Serial.println(WiFi.localIP());
    }

    // Connect to WiFi
    // Serial.println("Wifi:");
    // Serial.println(WIFI_SSID);
    // Serial.println("Password:");
    // Serial.println(WIFI_PASS);
    // while (WiFi.begin("mntjyspkl", "bebubbly") != WL_CONNECTED) {
    //     Serial.print(".");
    //     delay(1000);
    // }

    // Serial.println("\n[Arduino] Connected to WiFi.");
    // Serial.print("[Arduino] IP address: ");
    // Serial.println(WiFi.localIP());

    server.begin();
    Serial.println("[Arduino] Server started.");
}

void loop() {
    // Accept new client if none
    if (!client || !client.connected()) {
        client = server.available();
    }

    unsigned long now = millis();

    // Simulate or read real temperatures every second
    if (now - lastSimTime > 1000) {
        if (USE_SIMULATION) {
            simulateTemperatures();
        } else {
            T_core = analogToTemp(analogRead(CORE_TEMP_PIN));
            T_water = analogToTemp(analogRead(WATER_TEMP_PIN));
        }
        lastSimTime = now;
    }

    // Send data to client
    if (client && client.connected() && now - lastSendTime > 1000) {
        String payload = "T_CORE:" + String(T_core, 1) + ",T_WATER:" + String(T_water, 1) + ",MODE:" + mode + "\n";
        client.print(payload);
        Serial.print("[Arduino] Sent: ");
        Serial.println(payload);
        lastSendTime = now;
    }

    // Handle command from GUI
    if (client && client.available()) {
        String command = client.readStringUntil('\n');
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
