#include <Arduino.h>
#include "WiFi.h"
#include "esp_wifi.h"

void printMac(const uint8_t *mac)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(macStr);
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buff;

  int rssi = pkt->rx_ctrl.rssi;

  // WiFi header starter her
  uint8_t *payload = pkt->payload;

  // MAC-adressen ligger typisk fra byte 10
  uint8_t *mac = payload + 10;

  Serial.print("MAC: ");
  printMac(mac);
  Serial.print(" | RSSI: ");
  Serial.println(rssi);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

  Serial.println("WiFi Sniffer med MAC + RSSI startet");
}

void loop()
{
}