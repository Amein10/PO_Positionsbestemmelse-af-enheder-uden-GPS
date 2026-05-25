#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

void macToString(const uint8_t *mac, char *macStr)
{
  snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buff;

  int rssi = pkt->rx_ctrl.rssi;
  unsigned long timestamp = millis();

  uint8_t *payload = pkt->payload;
  uint8_t *mac = payload + 10;

  char macStr[18];
  macToString(mac, macStr);

  String json =
    "{"
    "\"id\":\"" + String(macStr) + "\","
    "\"timestamp\":" + String(timestamp) + ","
    "\"rssi\":" + String(rssi) + ","
    "\"x\":0,"
    "\"y\":0"
    "}";

  Serial.println(json);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("WiFi sniffer JSON prototype startet");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void loop()
{
}