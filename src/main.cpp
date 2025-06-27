#include <Arduino.h>
#include <WiFiS3.h>
#include <SPI.h>
#include "config.h"

// === CONFIG FLAGS ===
// Set in config.h:
//   #define IS_SIMULATION true/false
//   #define USE_WIFI true/false
//   #define WIFI_SSID "your_ssid"
//   #define WIFI_PASS "your_password"

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
const unsigned int PORT = 12345;
WiFiServer server(PORT);
WiFiClient client;
char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// === WiFi Status Helper ===
void printWiFiStatus()
{
    switch (WiFi.status())
    {
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
float analogToTemp(int analogValue)
{
    float voltage = analogValue * (5.0 / 1023.0);
    float temperature = (voltage - 0.5) * 100.0; // TMP36-style conversion
    return temperature;
}

// === Simulate Temperature ===
void simulateTemperatures()
{
    float ambient = 25.0;

    if (mode == "HEAT")
    {
        T_core += HEAT_RATE * 0.6;
        T_water += HEAT_RATE;
    }
    else if (mode == "COOL")
    {
        T_core -= COOL_RATE * 0.6;
        T_water -= COOL_RATE;
    }
    else
    {
        // Natural equalization towards ambient
        T_core += (ambient - T_core) * 0.01;
        T_water += (ambient - T_water) * 0.01;
    }

    // Clamp within realistic range
    T_core = constrain(T_core, 0, 100);
    T_water = constrain(T_water, 0, 100);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }
    Serial.println("[Arduino] Boot complete");

    Serial.print("[Arduino] Mode: ");
    Serial.println(IS_SIMULATION ? "Simulation" : "Live Sensor");

    Serial.print("[Arduino] Communication: ");
    Serial.println(USE_WIFI ? "WiFi" : "USB Serial");

    if (USE_WIFI)
    {
        // Print configuration for debugging
        // Serial.print("[Arduino] SSID: ");
        // Serial.println(ssid);
        // Serial.print("[Arduino] Password length: ");
        // Serial.println(strlen(password));

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

        for (int i = 0; i < numSsid; i++)
        {
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
        while (WiFi.status() != WL_CONNECTED && attempts < 20)
        {
            delay(1000);
            Serial.print(".");
            Serial.print(" Status: ");
            printWiFiStatus();
            attempts++;
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED)
        {
            delay(5000); // Wait for DHCP

            // Force reconnect to ensure fresh IP
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid, password);

            int reconnectAttempts = 0;
            while (WiFi.status() != WL_CONNECTED && reconnectAttempts < 15)
            {
                delay(1000);
                Serial.print("*");
                reconnectAttempts++;
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                delay(3000);
                IPAddress ip = WiFi.localIP();
                IPAddress gateway = WiFi.gatewayIP();
                IPAddress subnet = WiFi.subnetMask();

                if (ip == IPAddress(0, 0, 0, 0))
                {
                    Serial.println("[Arduino] WARNING: Still getting 0.0.0.0 - trying DHCP refresh...");
                    WiFi.disconnect();
                    delay(2000);
                    WiFi.config(IPAddress(0, 0, 0, 0)); // Reset to DHCP
                    WiFi.begin(ssid, password);

                    int dhcpAttempts = 0;
                    while (WiFi.status() != WL_CONNECTED && dhcpAttempts < 20)
                    {
                        delay(1000);
                        Serial.print("#");
                        dhcpAttempts++;
                    }

                    delay(5000);
                    ip = WiFi.localIP();
                }

                Serial.print("[Arduino] IP: ");
                Serial.println(ip);
                Serial.print("[Arduino] Signal strength: ");
                Serial.println(WiFi.RSSI());
                Serial.print("[Arduino] Gateway: ");
                Serial.println(gateway);
                Serial.print("[Arduino] Subnet: ");
                Serial.println(subnet);

                if (ip != IPAddress(0, 0, 0, 0))
                {
                    server.begin();
                    Serial.print("[Arduino] Server started on port ");
                    Serial.println(PORT);
                    Serial.print("[Arduino] Connect to: ");
                    Serial.print(ip);
                    Serial.print(":");
                    Serial.println(PORT);
                }
                else
                {
                    Serial.println("[Arduino] ERROR: Failed to get valid IP address");
                }
            }
            else
            {
                Serial.println("[Arduino] Reconnection failed!");
            }
        }
        else
        {
            Serial.println("[Arduino] Failed to connect to WiFi!");
            printWiFiStatus();

            Serial.println("[Arduino] Troubleshooting tips:");
            Serial.println("1. Check SSID and password in config.h");
            Serial.println("2. Verify network is 2.4GHz (not 5GHz)");
            Serial.println("3. Check if network uses WPA2 (not WPA3)");
            Serial.println("4. Try moving closer to router");
            Serial.println("5. Check if MAC filtering is enabled");
        }
    }
    else
    {
        Serial.println("[Arduino] USB communication - skipping WiFi setup.");
    }
}

void loop()
{
    unsigned long now = millis();

    // Periodic WiFi connection check
    if (USE_WIFI)
    {
        static unsigned long lastStatusCheck = 0;
        if (now - lastStatusCheck > 30000)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                Serial.println("[Arduino] WiFi connection lost!");
                printWiFiStatus();

                Serial.println("[Arduino] Attempting to reconnect...");
                WiFi.disconnect();
                delay(1000);
                WiFi.begin(ssid, password);
            }
            lastStatusCheck = now;
        }

        if (!client || !client.connected())
        {
            client = server.available();
            if (client)
            {
                Serial.println("[Arduino] New client connected");
                delay(500);

                // Send one payload right away
                String initialPayload = "T_CORE:" + String(T_core, 1) +
                                        ",T_WATER:" + String(T_water, 1) +
                                        ",MODE:" + mode + "\n";
                client.print(initialPayload);
                Serial.print("[Arduino] Sent (initial): ");
                Serial.println(initialPayload);
            }
        }
    }

    // Update temperatures
    if (now - lastSimTime > 1000)
    {
        if (IS_SIMULATION)
        {
            simulateTemperatures();
        }
        else
        {
            T_core = analogToTemp(analogRead(CORE_TEMP_PIN));
            T_water = analogToTemp(analogRead(WATER_TEMP_PIN));
        }
        lastSimTime = now;
    }

    // Send data
    if (now - lastSendTime > 1000)
    {
        String payload = "T_CORE:" + String(T_core, 1) +
                         ",T_WATER:" + String(T_water, 1) +
                         ",MODE:" + mode + "\n";

        if (USE_WIFI)
        {
            if (client && client.connected())
            {
                client.print(payload);
            }
        }

        Serial.print("[Arduino] Sent: ");
        Serial.println(payload);

        lastSendTime = now;
    }

    // Read commands
    String command = "";

    if (USE_WIFI && client && client.available())
    {
        command = client.readStringUntil('\n');
    }
    else if (!USE_WIFI && Serial.available())
    {
        command = Serial.readStringUntil('\n');
    }

    if (command.length() > 0)
    {
        command.trim();
        Serial.print("[Arduino] Received command: ");
        Serial.println(command);

        if (command.equalsIgnoreCase("heat"))
        {
            mode = "HEAT";
        }
        else if (command.equalsIgnoreCase("cool"))
        {
            mode = "COOL";
        }
        else if (command.equalsIgnoreCase("stop"))
        {
            mode = "IDLE";
        }
    }
}
