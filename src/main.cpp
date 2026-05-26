#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "esp_wifi.h"
#include <math.h>

// WiFi
const char* ssid = "IoT_H3/4";
const char* wifiPassword = "98806829";

// MQTT TLS
const char* mqtt_server = "192.168.0.161";
const int mqtt_port = 8883;
const char* mqtt_user = "user11";
const char* mqtt_password = "fZ4J5sjH";
const char* mqtt_topic = "/users/user11/position";

// CA certificate
const char MQTT_CA_CERT[] = R"(
-----BEGIN CERTIFICATE-----
MIIFBTCCAu2gAwIBAgIUZNXNRbyQrl4kJeE1awmG4JCBT14wDQYJKoZIhvcNAQEL
BQAwEjEQMA4GA1UEAwwHTVFUVC1DQTAeFw0yNjA1MTkyMjA5MDZaFw0zNjA1MTYy
MjA5MDZaMBIxEDAOBgNVBAMMB01RVFQtQ0EwggIiMA0GCSqGSIb3DQEBAQUAA4IC
DwAwggIKAoICAQDuOe5w6gAq0x0BUgTMTDtMmY3uVNz3TmRkB4cfC5wg86ZcOA/E
Zs27a3InRlbgS9Ak+WrUWeB5Budx010xGsTW7G1h1/TVf8yOq0qN1NKknNYxcO63
CvdnNcHSj0LCyzYDSRSz1qdmMh+j7/ZZ0N74is1L7EXT7uOcdDXvXAm8lUUH+v3L
YaHKX5BHuSy5+EwG9OlzpluajYxUxDBm3ip3Iyrax3mdSdDCkLeJwnj/Hvq944yg
BZxrmSyrV0q0R8M5Tfcw97TWNEFkY/Q0dg4QOSZcXmFviakftlOyqnMPkQyP8YhO
gz6O9pNjG8VCnVkSP6B2SyBUaGZskZEJwtXF2yISmMo9blRfuOHFVAnlYD97y6Is
ZRw75kEfQcjRf3r6twwiNdW3KL8McZh+M0JK1wLOX/HzmazaQDb4VfhR90VV3UkR
aJa4QRk8ow7aSgUHe43NPYU1u6CzTaLhKhvBO9kAIgegCAyVMgHQBsFrnltF/drW
yDdg15mFf1CBxNz1YYUGjz0h5Hz2vvsOl7xVhcaUHuP+DwaflIPqbANyso6JJ5LG
AW0qFRxPNSaZLyZ+n9XHkfUdTdmOMcwGIIzob+evcUR2jLqG2HsoGwSA4Vq5+sZx
pSHoEgsDhHQtD1cjX0pvpPcVMT3LSoBd30ztZKMuTEiCScqZqB8PvYSaJwIDAQAB
o1MwUTAdBgNVHQ4EFgQUvz3l/B8ajI63TusuyGktdy4lSXkwHwYDVR0jBBgwFoAU
vz3l/B8ajI63TusuyGktdy4lSXkwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAgEA3LSR7YzWrSubNoGu8cc7OW3OwXjvJgSLukR0CEuqlscrzcZ4vJRV
Kdrl3qbll8d4JzBRXryLl7QtPYaQLoQqKL9b4rNQHZVGW8iIgt/YDCVjNy8fvhgx
lppih66Czv/hv5WLPm4j9xjVE9HaPmCIE+Xr+zziPMsN8ehyre209imkXGWHadEq
QUwzDoet/qwWNW0LA1Z9Ct6e8534ctWKBGlJW/9NiAnPa+zAjn78GSrqqeFH1IAI
lNR3axk2UQWfcLXa6TBEkJeCANn0I6VMwb4BeJ8q4Jx93gu8+pQcoT10E/ZwUVWF
ckOILTatAXpfXGMQCPJHm5lOdeX4nrGwNJ4SKi9ThUo17eqNQ1ojxwDvNWCKFngl
uRZG+ZXAe2eVernpWGG7EjwZ1H+66Gvx4CeKyRbVumr9fwh8FC1uVah0/DxTZotP
LFRwtabFYmQfTcZbOMU1bQ5obROp8GIhlJY5vcREt2O1Ey9rmyjMlKrTWoflZwEw
4xi+h5vDEB9fgFBuf5j5bybyOxZRKs8PzyTj9Sr5V/5QnCMrdcVa31h7XuVr6Z6j
yfJk6/yX3fDN5i4A01bFqVaSZbwDhVyFtK1ersNkMflWjN3gomHE9LyXkke+3iF3
3wviuKAFTPSYhltv0q/myn5xnZD/yMXOfBzsvQqCZtV4gRfF0m7dgi0=
-----END CERTIFICATE-----
)";

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

struct DeviceSeen {
  String id;
  unsigned long lastSeen;
};

struct Point {
  float x;
  float y;
};

DeviceSeen devices[50];
int deviceCount = 0;

const unsigned long PRINT_INTERVAL = 5000;

void macToString(const uint8_t *mac, char *macStr) {
  snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

String simpleHash(String input) {
  unsigned long hash = 5381;

  for (int i = 0; i < input.length(); i++) {
    hash = ((hash << 5) + hash) + input[i];
  }

  return String(hash, HEX);
}

float estimateDistance(int rssi) {
  int txPower = -59;
  float n = 2.0;

  return pow(10, ((txPower - rssi) / (10.0 * n)));
}

Point trilaterate(
  float x1, float y1, float r1,
  float x2, float y2, float r2,
  float x3, float y3, float r3
) {
  float A = 2 * (x2 - x1);
  float B = 2 * (y2 - y1);
  float C = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1 * y1 + y2 * y2;

  float D = 2 * (x3 - x1);
  float E = 2 * (y3 - y1);
  float F = r1 * r1 - r3 * r3 - x1 * x1 + x3 * x3 - y1 * y1 + y3 * y3;

  Point p;
  float denominator = A * E - B * D;

  if (denominator == 0) {
    p.x = 0;
    p.y = 0;
    return p;
  }

  p.x = (C * E - B * F) / denominator;
  p.y = (A * F - C * D) / denominator;

  return p;
}

bool shouldPrint(String id) {
  unsigned long now = millis();

  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].id == id) {
      if (now - devices[i].lastSeen >= PRINT_INTERVAL) {
        devices[i].lastSeen = now;
        return true;
      }
      return false;
    }
  }

  if (deviceCount < 50) {
    devices[deviceCount].id = id;
    devices[deviceCount].lastSeen = now;
    deviceCount++;
  }

  return true;
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);

  Serial.print("Forbinder til WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi forbundet!");
  Serial.print("IP adresse: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Forbinder til MQTT TLS...");

    String clientId = "ESP32-Position-user11-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("forbundet!");
    } else {
      Serial.print("fejl, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" prøver igen om 5 sekunder");
      delay(5000);
    }
  }
}

void publishPosition(String payload) {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.loop();

  Serial.print("Sender MQTT: ");
  Serial.println(payload);

  bool sent = mqttClient.publish(mqtt_topic, payload.c_str());

  if (sent) {
    Serial.print("MQTT besked sendt til topic: ");
    Serial.println(mqtt_topic);
  } else {
    Serial.println("MQTT besked fejlede");
  }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buff;

  int rssi = pkt->rx_ctrl.rssi;
  unsigned long timestamp = millis();

  float d1 = estimateDistance(rssi);

  // Simulerede afstande fra to ekstra stationer
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

  if (!shouldPrint(hashedId)) {
    return;
  }

  String json =
    "{"
    "\"user\":\"user11\","
    "\"id\":\"" + hashedId + "\","
    "\"timestamp\":" + String(timestamp) + ","
    "\"rssi\":" + String(rssi) + ","
    "\"distance\":" + String(d1, 2) + ","
    "\"x\":" + String(position.x, 2) + ","
    "\"y\":" + String(position.y, 2) + ","
    "\"unique_devices\":" + String(deviceCount) +
    "}";

  Serial.println(json);
  publishPosition(json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("WiFi sniffer med MQTT TLS startet - user11");

  initWiFi();

  espClient.setInsecure();

  mqttClient.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();

  // VIGTIGT:
  // IKKE kald WiFi.disconnect(false) her.
  // Det afbryder WiFi/MQTT forbindelsen.

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

  Serial.println("Sniffer startet");
}

void loop() {
  mqttClient.loop();

  static unsigned long lastCountPrint = 0;

  if (millis() - lastCountPrint >= 10000) {
    lastCountPrint = millis();

    Serial.print("Antal unikke enheder set: ");
    Serial.println(deviceCount);
  }
}