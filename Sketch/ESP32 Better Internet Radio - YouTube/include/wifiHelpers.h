// All things WiFi go here
#include "Arduino.h"
#include "main.h"

// Connect to WiFi
void connectToWifi() {
    log_d("-----------------------------------");
    log_d("Connecting to SSID: %s", ssid.c_str());
    log_d("-----------------------------------");

    // Ensure we disconnect WiFi first to stop re-connection problems
    if (WiFi.status() == WL_CONNECTED) {
        log_w("WiFi already connected with (local) IP address of: %s", WiFi.localIP().toString().c_str());
        wiFiDisconnected = false;
        return;
    }

    // WE want to be a client not a server
    log_d("Setting ESP32 to STA mode");
    WiFi.mode(WIFI_STA);

    // Don't store the SSID and password
    log_d("Setting ESP32 to NOT store credentials in NVR");
    WiFi.persistent(false);

    // Don't allow WiFi sleep
    log_d("Setting ESP32 to NOT allow sleep");
    WiFi.setSleep(false);

    log_d("Setting ESP32 to ALLOW auto re-connect");
    WiFi.setAutoReconnect(true);

    // Lock down the WiFi stuff - not mormally necessary unless you need a static IP in AP mode
    // IPAddress local_IP(192, 168, 1, 102);
    // IPAddress gateway(192, 168, 1, 254);
    // IPAddress subnet(255, 255, 255, 0);
    // IPAddress DNS1(8, 8, 8, 8);
    // IPAddress DNS2(8, 8, 4, 4);
    // WiFi.config(local_IP, gateway, subnet, DNS1, DNS2);

    // Connect to the required WiFi
    log_d("Initiating connection with WiFi.");
    WiFi.begin(ssid.c_str(), wifiPassword.c_str());

    log_d("Setting ESP32 to REDUCE maximum transmit power");
    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

    // Give things a chance to settle, avoid problems
    delay(2000);

    log_d("Waiting for WiFi connection...");
    uint8_t wifiStatus = WiFi.waitForConnectResult();

    // Successful connection?
    if ((wl_status_t)wifiStatus != WL_CONNECTED) {
        log_e("WiFi Status: %s, exiting", wl_status_to_string((wl_status_t)wifiStatus));
        return;
    }

    log_d("WiFi connected with (local) IP address of: %s", WiFi.localIP().toString().c_str());
    wiFiDisconnected = false;
}

// Get the WiFi SSID
std::string getSSID() {
    std::string currSSID = readLITTLEFSInfo((char *)"SSID");
    if (currSSID == "") {
        return "Benny";
    } else {
        return currSSID;
    }
}

// Get the WiFi Password
std::string getWiFiPassword() {
    std::string currWiFiPassword = readLITTLEFSInfo((char *)"WiFiPassword");
    if (currWiFiPassword == "") {
        return "BestCatEver";
    } else {
        return currWiFiPassword;
    }
}

// Convert the WiFi (error) response to a string we can understand
const char *wl_status_to_string(wl_status_t status) {
    switch (status) {
    case WL_NO_SHIELD:
        return "WL_NO_SHIELD";
    case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
        return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}