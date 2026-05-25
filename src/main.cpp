#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

struct DeviceSeen
{
  String id;
  unsigned long lastSeen;
};

DeviceSeen devices[50];
int deviceCount = 0;

const unsigned long PRINT_INTERVAL = 5000;

void macToString(const uint8_t *mac, char *macStr)
{
  snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

String simpleHash(String input)
{
  unsigned long hash = 5381;

  for (int i = 0; i < input.length(); i++)
  {
    hash = ((hash << 5) + hash) + input[i];
  }

  return String(hash, HEX);
}

bool shouldPrint(String id)
{
  unsigned long now = millis();

  for (int i = 0; i < deviceCount; i++)
  {
    if (devices[i].id == id)
    {
      if (now - devices[i].lastSeen >= PRINT_INTERVAL)
      {
        devices[i].lastSeen = now;
        return true;
      }

      return false;
    }
  }

  if (deviceCount < 50)
  {
    devices[deviceCount].id = id;
    devices[deviceCount].lastSeen = now;
    deviceCount++;
  }

  return true;
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

  String rawMac = String(macStr);
  String hashedId = simpleHash(rawMac);

  if (!shouldPrint(hashedId))
  {
    return;
  }

  String json =
    "{"
    "\"id\":\"" + hashedId + "\","
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

  Serial.println("WiFi sniffer med hashed ID og tæller startet");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void loop()
{
  static unsigned long lastCountPrint = 0;

  if (millis() - lastCountPrint >= 10000)
  {
    lastCountPrint = millis();

    Serial.print("Antal unikke enheder set: ");
    Serial.println(deviceCount);
  }
}