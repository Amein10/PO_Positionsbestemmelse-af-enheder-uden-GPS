#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <math.h>

struct DeviceSeen
{
  String id;
  unsigned long lastSeen;
};

struct Point
{
  float x;
  float y;
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

float estimateDistance(int rssi)
{
  int txPower = -59;
  float n = 2.0;

  return pow(10, ((txPower - rssi) / (10.0 * n)));
}

Point trilaterate(
  float x1, float y1, float r1,
  float x2, float y2, float r2,
  float x3, float y3, float r3
)
{
  float A = 2 * (x2 - x1);
  float B = 2 * (y2 - y1);
  float C = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1 * y1 + y2 * y2;

  float D = 2 * (x3 - x1);
  float E = 2 * (y3 - y1);
  float F = r1 * r1 - r3 * r3 - x1 * x1 + x3 * x3 - y1 * y1 + y3 * y3;

  Point p;

  float denominator = A * E - B * D;

  if (denominator == 0)
  {
    p.x = 0;
    p.y = 0;
    return p;
  }

  p.x = (C * E - B * F) / denominator;
  p.y = (A * F - C * D) / denominator;

  return p;
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

  float d1 = estimateDistance(rssi);

  // Simulerede afstande fra to ekstra ESP32-stationer
  float d2 = d1 + 1.5;
  float d3 = d1 + 2.0;

  Point position = trilaterate(
    0, 0, d1,
    5, 0, d2,
    0, 5, d3
  );

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
    "\"distance\":" + String(d1, 2) + ","
    "\"x\":" + String(position.x, 2) + ","
    "\"y\":" + String(position.y, 2) +
    "}";

  Serial.println(json);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("WiFi sniffer med simuleret trilateration startet");

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