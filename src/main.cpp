// #include <Arduino.h>
#include "WiFi.h"

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();

  delay(1000);

  Serial.println("Scanning...");

  int networks = WiFi.scanNetworks();

  for (int i = 0; i < networks; i++) {
    Serial.print(WiFi.SSID(i));
    Serial.print(" RSSI: ");
    Serial.println(WiFi.RSSI(i));
  }
}

void loop() {
}