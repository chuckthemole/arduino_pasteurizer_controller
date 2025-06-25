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

// === WiFi Status Helper ===
void printWiFiStatus() {
    switch(WiFi.status()) {
        case WL_IDLE_STATUS:
            Serial.println("WL_IDLE_STATUS");
            break;
        case WL_NO_SSID_AVAIL:
            Serial.println("WL_NO_SSID_AVAIL - Network not found");
            break;
        case WL_SCAN_COMPLETED:
            Serial.println("WL_SCAN_COMPLETED");
            break;
        case WL_CONNECTED:
            Serial.println("WL_CONNECTED");
            break;
        case WL_CONNECT_FAILED:
            Serial.println("WL_CONNECT_FAILED - Wrong password or connection issue");
            break;
        case WL_CONNECTION_LOST:
            Serial.println("WL_CONNECTION_LOST");
            break;
        case WL_DISCONNECTED:
            Serial.println("WL_DISCONNECTED");
            break;
        default:
            Serial.print("Unknown status: ");
            Serial.println(WiFi.status());
            break;
    }
}

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
    Serial.begin(115200);  // Increased baud rate for better debugging
    while (!Serial) {
        delay(100);
    }
    Serial.println("[Arduino] Boot complete");

    // Print configuration for debugging
    Serial.print("[Arduino] SSID: ");
    Serial.println(ssid);
    Serial.print("[Arduino] Password length: ");
    Serial.println(strlen(password));

#if SIM_MODE_WIFI
    // Check if WiFi hardware is available
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("[Arduino] WiFi module not detected!");
        while (true) {
            delay(1000);
        }
    }
    
    // Print firmware version before connecting
    String fv = WiFi.firmwareVersion();
    Serial.print("[Arduino] Firmware version: ");
    Serial.println(fv);
    
    // Scan for networks to verify WiFi is working
    Serial.println("[Arduino] Scanning for networks...");
    int numSsid = WiFi.scanNetworks();
    Serial.print("[Arduino] Found ");
    Serial.print(numSsid);
    Serial.println(" networks:");
    
    for (int i = 0; i < numSsid; i++) {
        Serial.print("  ");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm)");
        Serial.println();
    }
    
    // Attempt connection
    Serial.print("[Arduino] Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // Increased attempts
        delay(1000);
        Serial.print(".");
        Serial.print(" Status: ");
        printWiFiStatus();
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[Arduino] WiFi connected successfully!");
        Serial.print("[Arduino] IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("[Arduino] Signal strength: ");
        Serial.println(WiFi.RSSI());
        Serial.print("[Arduino] Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("[Arduino] Subnet: ");
        Serial.println(WiFi.subnetMask());
        Serial.print("[Arduino] DNS: ");
        Serial.println(WiFi.dnsIP());
        
        server.begin();
        Serial.println("[Arduino] Server started on port 12345");
    } else {
        Serial.println("[Arduino] Failed to connect to WiFi!");
        Serial.print("[Arduino] Final status: ");
        printWiFiStatus();
        
        // Additional debugging
        Serial.println("[Arduino] Troubleshooting tips:");
        Serial.println("1. Check SSID and password in config.h");
        Serial.println("2. Verify network is 2.4GHz (not 5GHz)");
        Serial.println("3. Check if network uses WPA2 (not WPA3)");
        Serial.println("4. Try moving closer to router");
        Serial.println("5. Check if MAC filtering is enabled");
    }
#else
    Serial.println("[Arduino] USB simulation mode - skipping WiFi setup.");
#endif
}

void loop() {
    unsigned long now = millis();

    // Check WiFi connection status periodically
#if SIM_MODE_WIFI
    static unsigned long lastStatusCheck = 0;
    if (now - lastStatusCheck > 30000) {  // Check every 30 seconds
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Arduino] WiFi connection lost!");
            printWiFiStatus();
            
            // Attempt to reconnect
            Serial.println("[Arduino] Attempting to reconnect...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid, password);
        }
        lastStatusCheck = now;
    }
#endif

    // Accept new WiFi client (only in WiFi mode)
#if SIM_MODE_WIFI
    if (!client || !client.connected()) {
        client = server.available();
        if (client) {
            Serial.println("[Arduino] New client connected");
        }
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