#include <Arduino.h>
#include "WiFi.h"
#include "esp_wifi.h"

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buff;

  int rssi = pkt->rx_ctrl.rssi;

  Serial.print("RSSI: ");
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

  Serial.println("WiFi Sniffer Started");
}

void loop()
{
}