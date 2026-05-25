#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Placeholder WiFi-info
const char* ssid = "DIT_WIFI";
const char* password = "DIT_PASSWORD";

// Placeholder MQTT broker IP
const char* mqtt_server = "192.168.1.100";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi()
{
  delay(100);

  Serial.println();
  Serial.print("Forbinder til WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi forbundet");
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Forsøger MQTT forbindelse...");

    if (client.connect("ESP32Client"))
    {
      Serial.println("forbundet");
    }
    else
    {
      Serial.print("fejl, rc=");
      Serial.print(client.state());
      Serial.println(" prøver igen om 5 sekunder");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("MQTT placeholder prototype startet");

  // WiFi er slået fra indtil vi tester på skolen
  // setup_wifi();

  // MQTT server er defineret, men bruges ikke aktivt endnu
  client.setServer(mqtt_server, 1883);
}

void loop()
{
  // MQTT forbindelse er slået fra indtil vi tester på skolen
  // if (!client.connected())
  // {
  //   reconnect();
  // }

  // client.loop();

  String payload =
    "{"
    "\"id\":\"placeholder-device-id\","
    "\"timestamp\":123456,"
    "\"x\":2.1,"
    "\"y\":1.4,"
    "\"rssi\":-67"
    "}";

  Serial.println(payload);

  // MQTT publish er slået fra indtil vi tester på skolen
  // client.publish("iot/position", payload.c_str());

  delay(5000);
}