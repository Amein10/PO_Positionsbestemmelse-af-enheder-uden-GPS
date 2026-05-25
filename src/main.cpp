#include <Arduino.h>

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("MQTT Placeholder Test startet");
}

void loop()
{
  String json =
    "{"
    "\"id\":\"placeholder-device-id\","
    "\"timestamp\":123456,"
    "\"x\":2.1,"
    "\"y\":1.4,"
    "\"rssi\":-67"
    "}";

  Serial.println(json);

  delay(3000);
}