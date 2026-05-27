#include <Arduino.h>          // Grundlæggende Arduino-funktioner som setup(), loop(), Serial, delay osv.
#include <WiFi.h>             // Bruges til at forbinde ESP32 til skolens WiFi-netværk
#include <WiFiClientSecure.h> // Bruges til sikker TLS/SSL-forbindelse til MQTT-brokeren
#include <PubSubClient.h>     // MQTT-klientbibliotek til at sende data til MQTT-serveren
#include "esp_wifi.h"         // ESP32 WiFi-driver, bruges til promiscuous mode / WiFi sniffing
#include <math.h>             // Bruges til matematiske funktioner, fx pow() til afstandsberegning

// --------------------------------------------------
// WiFi-konfiguration
// --------------------------------------------------

// SSID er navnet på det WiFi-netværk ESP32 skal forbinde til.
// I dette projekt bruges skolens IoT-netværk.
const char* ssid = "IoT_H3/4";

// Password til WiFi-netværket.
// Bruges af ESP32 til at oprette forbindelse til netværket.
const char* wifiPassword = "98806829";

// --------------------------------------------------
// MQTT TLS-konfiguration
// --------------------------------------------------

// IP-adressen på den lokale MQTT-broker.
// Brokerens opgave er at modtage data fra ESP32-enhederne.
const char* mqtt_server = "192.168.0.161";

// MQTT-port 8883 bruges til MQTT over TLS.
// TLS betyder, at forbindelsen er krypteret.
const int mqtt_port = 8883;

// Brugernavn til MQTT-brokeren.
// Dette bruges til at identificere brugeren/enheden på MQTT-serveren.
const char* mqtt_user = "user11";

// Password til MQTT-brugeren.
// Bruges sammen med mqtt_user til login på MQTT-brokeren.
const char* mqtt_password = "fZ4J5sjH";

// Base-topic for projektets positionsdata.
// Den enkelte ESP32 tilføjer senere sit eget node-navn,
// fx /users/user11/position/ESP32_A
const char* mqtt_base_topic = "/users/user11/position";

// --------------------------------------------------
// NODE CONFIGURATION
// --------------------------------------------------

// Hver ESP32-enhed får et unikt node-id.
// Dette bruges til at identificere hvilken ESP32,
// der sender data til MQTT-serveren.
//
// Eksempel:
// ESP32_A
// ESP32_B
// ESP32_C
//
// På den måde kan man se hvilken målestation,
// der har registreret en bestemt mobilenhed.
//
// Node-navnet bliver også brugt i MQTT-topic:
// /users/user11/position/ESP32_A
//
// Hvis flere ESP32-enheder bruges samtidigt,
// skal hver ESP32 have sit eget node-id.

const char* node_id = "ESP32_A";

// --------------------------------------------------
// CA CERTIFICATE (TLS CERTIFIKAT)
// --------------------------------------------------

// Dette certifikat bruges til sikker MQTT-kommunikation over TLS.
//
// Certifikatet bruges til at verificere MQTT-serverens identitet,
// så ESP32 kan oprette en krypteret og sikker forbindelse.
//
// TLS beskytter data mod:
// - aflytning
// - manipulation
// - uautoriseret adgang
//
// Certifikatet fungerer som en digital signatur,
// der hjælper ESP32 med at stole på MQTT-serveren.
//
// I dette projekt bruges MQTT over port 8883,
// som er standardporten for sikker MQTT/TLS.
//
// Certifikatet blev udleveret sammen med MQTT-serverens adgang.
//
// Bemærk:
// Under test blev espClient.setInsecure() brugt,
// fordi certifikatverificering gav problemer.
// I en rigtig produktion bør korrekt certifikatvalidering bruges.
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

// --------------------------------------------------
// MQTT CLIENT OBJECTS
// --------------------------------------------------

// WiFiClientSecure bruges til at oprette en sikker TLS-forbindelse.
// Denne forbindelse bruges af MQTT-klienten til krypteret kommunikation.
//
// TLS beskytter data mod:
// - aflytning
// - manipulation
// - uautoriseret adgang
WiFiClientSecure espClient;

// PubSubClient er selve MQTT-klienten.
//
// MQTT-klienten bruger den sikre TLS-forbindelse (espClient)
// til at sende data til MQTT-brokeren.
//
// MQTT bruges i projektet til:
// - at sende positionsdata
// - sende RSSI-værdier
// - sende anonymiserede device-id'er
// - sende data fra flere ESP32-enheder
PubSubClient mqttClient(espClient);

// --------------------------------------------------
// STRUCT: DeviceSeen
// --------------------------------------------------

// Denne struct bruges til at gemme oplysninger
// om enheder (mobiltelefoner) som ESP32 har registreret.
//
// Formålet er:
// - at undgå spam fra samme enhed
// - holde styr på unikke devices
// - registrere hvornår enheden sidst blev set
//
// Hver registreret enhed får:
// - et anonymiseret ID
// - tidspunkt for sidste registrering
struct DeviceSeen {

  // Hash'et/anonymiseret ID for den registrerede enhed.
  // ID'et er baseret på MAC-adressen, men den rigtige
  // MAC-adresse gemmes ikke direkte af hensyn til GDPR.
  String id;

  // Tidspunkt (millis) for hvornår enheden sidst blev set.
  // Bruges til at undgå at samme device bliver sendt
  // til MQTT for ofte.
  unsigned long lastSeen;
};

// --------------------------------------------------
// STRUCT: Point
// --------------------------------------------------

// Denne struct repræsenterer et punkt
// i et 2D-koordinatsystem.
//
// Bruges til positionsbestemmelse
// og trilateration.
//
// Punktet består af:
// - x-koordinat
// - y-koordinat
//
// Disse koordinater bruges til at estimere
// hvor en mobilenhed befinder sig i området.

struct Point {

  // X-koordinat i det 2-dimensionelle område.
  // Bruges til positionsbestemmelse/trilateration.
  float x;

  // Y-koordinat i det 2-dimensionelle område.
  // Sammen med x bruges den til at estimere
  // hvor enheden befinder sig.
  float y;
};

// Array som gemmer registrerede enheder.
//
// Hver plads i arrayet indeholder:
// - anonymiseret device-id
// - tidspunkt hvor enheden sidst blev set
//
// Systemet kan maksimalt gemme 50 enheder samtidigt.
DeviceSeen devices[50];

// Variabel som holder styr på hvor mange
// unikke enheder der er registreret.
int deviceCount = 0;

// Tidsinterval i millisekunder.
//
// Bruges til at begrænse hvor ofte samme device
// må sendes til MQTT.
//
// 5000 ms = 5 sekunder
const unsigned long PRINT_INTERVAL = 5000;

// Funktion som konverterer en MAC-adresse
// fra bytes til læsbar tekst.
//
// Eksempel:
// 24:6F:28:AB:CD:EF
//
// ESP32 modtager MAC-adressen som rå bytes.
// Derfor skal den omdannes til tekstformat,
// så den senere kan hashes og bruges i JSON.
void macToString(const uint8_t *mac, char *macStr) {

  // snprintf formatterer MAC-adressen til tekst.
  //
  // %02X betyder:
  // - hexadecimal værdi
  // - 2 tegn
  // - store bogstaver
  // - tilføj 0 foran hvis nødvendigt
  //
  // Resultatet bliver standard MAC-format:
  // XX:XX:XX:XX:XX:XX
  snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Funktion som laver et simpelt hash af en tekst.
//
// I projektet bruges funktionen til at anonymisere
// MAC-adresser før de sendes til MQTT.
//
// Formålet er at mindske behandling af persondata,
// så rå MAC-adresser ikke gemmes direkte.
//
// Funktionen returnerer et hash i hexadecimal tekstformat.
String simpleHash(String input) {

  // Startværdi til hash-beregningen.
  // 5381 bruges ofte i den simple DJB2 hash-algoritme.
  unsigned long hash = 5381;

  // Gennemløber alle tegn i input-strengen.
  for (int i = 0; i < input.length(); i++) {

    // Beregner nyt hash baseret på tidligere værdi
    // og det aktuelle tegn.
    //
    // hash << 5 svarer til hash * 32
    // hvilket giver variation i hash-værdien.
    hash = ((hash << 5) + hash) + input[i];
  }

  // Returnerer hash-værdien som hexadecimal tekst.
  //
  // Eksempel:
  // "4d2e6037"
  return String(hash, HEX);
}

// Funktion som estimerer afstand ud fra RSSI.
//
// RSSI (Received Signal Strength Indicator)
// fortæller hvor stærkt WiFi-signalet er.
//
// Svagere signal betyder typisk større afstand.
//
// Funktionen bruger en simpel path-loss model
// til at estimere afstanden mellem ESP32 og mobilenhed.
float estimateDistance(int rssi) {

  // Forventet RSSI-værdi ved 1 meters afstand.
  //
  // Denne værdi varierer i praksis afhængigt af:
  // - hardware
  // - antenner
  // - omgivelser
  int txPower = -59;

  // Path-loss faktor.
  //
  // Beskriver hvor hurtigt signalet svækkes.
  //
  // Typiske værdier:
  // 2.0 = åbent område
  // 3-4 = indendørs med vægge/støj
  float n = 2.0;

  // Beregner estimeret afstand med RSSI-formlen.
  //
  // pow() bruges til potensberegning.
  //
  // Resultatet er kun et estimat,
  // fordi WiFi-signaler påvirkes af:
  // - vægge
  // - mennesker
  // - refleksioner
  // - interferens
  return pow(10, ((txPower - rssi) / (10.0 * n)));
}

// Funktion som udfører trilateration.
//
// Trilateration bruges til at estimere positionen
// af en enhed ud fra afstande til 3 kendte punkter.
//
// x1,y1 = position for station 1
// r1    = afstand til station 1
//
// x2,y2 = position for station 2
// r2    = afstand til station 2
//
// x3,y3 = position for station 3
// r3    = afstand til station 3
Point trilaterate(
  float x1, float y1, float r1,
  float x2, float y2, float r2,
  float x3, float y3, float r3
) {
  // Beregner værdier til ligningssystemet
  float A = 2 * (x2 - x1);
  float B = 2 * (y2 - y1);
  float C = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1 * y1 + y2 * y2;

  float D = 2 * (x3 - x1);
  float E = 2 * (y3 - y1);
  float F = r1 * r1 - r3 * r3 - x1 * x1 + x3 * x3 - y1 * y1 + y3 * y3;

  // Point som skal indeholde den beregnede position
  Point p;

  // Bruges til at kontrollere om ligningssystemet kan løses
  float denominator = A * E - B * D;

  // Hvis denominator er 0 kan positionen ikke beregnes
  if (denominator == 0) {
    p.x = 0;
    p.y = 0;
    return p;
  }

  // Beregner x- og y-koordinater
  p.x = (C * E - B * F) / denominator;
  p.y = (A * F - C * D) / denominator;

  // Returnerer den estimerede position
  return p;
}

// Funktion som kontrollerer om en enhed
// må sendes/printes igen.
//
// Formålet er at undgå spam fra samme device,
// så den samme mobilenhed ikke sendes til MQTT
// hele tiden.
bool shouldPrint(String id) {
  unsigned long now = millis(); // Gemmer nuværende tidspunkt i millisekunder

// Gennemløber alle registrerede enheder
  for (int i = 0; i < deviceCount; i++) {
    // Kontrollerer om enheden allerede findes
    if (devices[i].id == id) {
      // Tjekker om der er gået nok tid siden sidste registrering
      if (now - devices[i].lastSeen >= PRINT_INTERVAL) {
        // Opdaterer tidspunkt for sidste registrering
        devices[i].lastSeen = now;
        // Enheden må sendes igen
        return true;
      }
      // Hvis tidsintervallet ikke er overskredet,
      // ignoreres enheden midlertidigt
      return false;
    }
  }

  // Hvis enheden ikke allerede findes i listen,
  // oprettes en ny registrering.
  //
  // Systemet kan maksimalt gemme 50 enheder ad gangen.
  if (deviceCount < 50) {
    // Gemmer det anonymiserede device-id
    devices[deviceCount].id = id;
    // Gemmer tidspunktet hvor enheden blev registreret
    devices[deviceCount].lastSeen = now;
    // Øger antal registrerede enheder
    deviceCount++;
  }

  // Returnerer true fordi enheden er ny
  // og derfor må sendes til MQTT
  return true;
}

// Funktion som opretter forbindelse til WiFi-netværket.
//
// ESP32 forbindes til skolens IoT-netværk,
// så enheden kan sende data til MQTT-serveren.
void initWiFi() {
  // Sætter ESP32 i station mode.
  //
  // WIFI_STA betyder at ESP32 opfører sig som klient
  // og forbinder til et eksisterende WiFi-netværk.
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, wifiPassword); // Starter forbindelsen til WiFi med SSID og password

  Serial.print("Forbinder til WiFi"); // Skriver status til Serial Monitor

  while (WiFi.status() != WL_CONNECTED) { // Venter indtil ESP32 er forbundet til netværket
    Serial.print("."); // Printer punktummer mens forbindelsen oprettes
    delay(500); // Lille pause mellem hver statusopdatering
  }

  Serial.println(); // Linjeskift i Serial Monitor
  Serial.println("WiFi forbundet!"); // Bekræfter at WiFi-forbindelsen virker
  Serial.print("IP adresse: "); // Udskriver ESP32'ens lokale IP-adresse
  Serial.println(WiFi.localIP()); // Viser den tildelte IP-adresse fra netværket
}

// Funktion som genopretter forbindelsen til MQTT-serveren.
//
// Hvis ESP32 mister forbindelsen til MQTT,
// forsøger funktionen automatisk at forbinde igen.
//
// MQTT bruges til at sende positionsdata,
// RSSI-værdier og device-information til broker'en.
void reconnectMQTT() {
  while (!mqttClient.connected()) { // Kører så længe MQTT ikke er forbundet
    Serial.print("Forbinder til MQTT TLS..."); // Skriver forbindelsesstatus til Serial Monitor

    // Opretter et unikt client-id til MQTT-forbindelsen.
    //
    // MQTT kræver at hver klient har et unikt navn.
    // Derfor tilføjes et tilfældigt hex-tal til slut.
    String clientId = "ESP32-Position-user11-";
    clientId += String(random(0xffff), HEX);

    // Forsøger at oprette forbindelse til MQTT-serveren
    // med brugernavn og password.
    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("forbundet!"); // Forbindelsen lykkedes
    } else {
      // Hvis forbindelsen fejler,
      // udskrives fejlkoden til debugging.
      Serial.print("fejl, rc=");
      Serial.print(mqttClient.state());

      Serial.println(" prøver igen om 5 sekunder"); // Informerer om nyt forbindelsesforsøg
      delay(5000); // Venter 5 sekunder før næste forsøg
    }
  }
}

// Funktion som sender positionsdata til MQTT-serveren.
//
// Data sendes som JSON-format til et MQTT-topic,
// så andre systemer kan modtage og analysere dataene.
//
// Payload indeholder bl.a:
// - anonymiseret device-id
// - RSSI
// - afstand
// - node-id
// - position
void publishPosition(String payload) {
  // Kontrollerer om MQTT-forbindelsen stadig virker.
  // Hvis forbindelsen er mistet, forsøges der at genoprette den.
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop(); // Holder MQTT-forbindelsen aktiv

  // Opretter MQTT-topic dynamisk ud fra node-id.
  //
  // Eksempel:
  // /users/user11/position/ESP32_A
  String topic = String(mqtt_base_topic) + "/" + String(node_id);

  // Udskriver topic og payload til Serial Monitor
  // for debugging og overvågning.
  Serial.print("Sender MQTT til topic: ");
  Serial.println(topic);
  Serial.println(payload);

  // Sender JSON-data til MQTT-serveren.
  //
  // publish() returnerer true hvis beskeden blev sendt korrekt.
  bool sent = mqttClient.publish(topic.c_str(), payload.c_str());

  // Bekræfter om MQTT-beskeden blev sendt eller fejlede
  if (sent) {
    Serial.println("MQTT besked sendt");
  } else {
    Serial.println("MQTT besked fejlede");
  }
}

// Callback-funktion som kaldes hver gang ESP32
// registrerer en WiFi-pakke i promiscuous mode.
//
// Funktionen bruges til:
// - WiFi sniffing
// - RSSI-måling
// - afstandsberegning
// - positionsestimering
// - MQTT-dataopsamling
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
  // Konverterer den modtagne buffer til en WiFi-pakke
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buff;

  int rssi = pkt->rx_ctrl.rssi;   // Henter signalstyrken (RSSI) fra pakken
  unsigned long timestamp = millis(); // Gemmer tidspunktet for registreringen

  float d1 = estimateDistance(rssi); // Estimerer afstand ud fra RSSI

  // Simulerede afstande fra ekstra stationer.
  //
  // Bruges som prototype til trilateration.
  // I en rigtig løsning skulle disse værdier
  // komme fra andre ESP32-enheder.
  float d2 = d1 + 1.5;
  float d3 = d1 + 2.0;

  // Beregner estimeret position med trilateration
  Point position = trilaterate(
    0, 0, d1,
    5, 0, d2,
    0, 5, d3
  );

  uint8_t *payload = pkt->payload; // Henter payload fra WiFi-pakken
  uint8_t *mac = payload + 10; // Finder MAC-adressen i payload-dataene

  char macStr[18]; // Buffer til MAC-adresse som tekst
  macToString(mac, macStr); // Konverterer MAC-adressen til læsbart tekstformat

  String rawMac = String(macStr); // Gemmer MAC-adressen som String
  String hashedId = simpleHash(rawMac); // Anonymiserer MAC-adressen med hashing

  // Kontrollerer om enheden må sendes igen.
  // Hvis ikke returnerer funktionen med det samme.
  if (!shouldPrint(hashedId)) {
    return;
  }

  // Opretter JSON-data som skal sendes til MQTT.
  //
  // Indeholder:
  // - bruger
  // - node-id
  // - anonymiseret device-id
  // - tidspunkt
  // - RSSI
  // - afstand
  // - position
  // - antal registrerede devices
  String json =
    "{"
    "\"user\":\"user11\","
    "\"node\":\"" + String(node_id) + "\","
    "\"id\":\"" + hashedId + "\","
    "\"timestamp\":" + String(timestamp) + ","
    "\"rssi\":" + String(rssi) + ","
    "\"distance\":" + String(d1, 2) + ","
    "\"x\":" + String(position.x, 2) + ","
    "\"y\":" + String(position.y, 2) + ","
    "\"unique_devices\":" + String(deviceCount) +
    "}";

  Serial.println(json); // Udskriver JSON-data til Serial Monitor
  publishPosition(json); // Sender data til MQTT-serveren
}

// Setup-funktion som køres én gang når ESP32 starter.
//
// Funktionen initialiserer:
// - Serial Monitor
// - WiFi-forbindelse
// - MQTT-forbindelse
// - WiFi sniffing i promiscuous mode
void setup() {
  Serial.begin(115200); // Starter Serial Monitor med baudrate 115200
  delay(1000); // Lille pause så Serial Monitor kan nå at starte

  // Skriver statusmeddelelse til Serial Monitor
  Serial.println("WiFi sniffer med MQTT TLS startet - user11");

  // Opretter forbindelse til WiFi-netværket
  initWiFi();

  // Deaktiverer certifikatvalidering under test.
  //
  // Bruges fordi TLS-certifikatet gav problemer
  // under udvikling og test af MQTT-forbindelsen.
  espClient.setInsecure();

  mqttClient.setServer(mqtt_server, mqtt_port); // Angiver MQTT-server og portnummer
  reconnectMQTT(); // Opretter forbindelse til MQTT-brokeren

  // VIGTIGT:
  // IKKE kald WiFi.disconnect(false) her.
  // Det afbryder WiFi/MQTT forbindelsen.

  // Aktiverer promiscuous mode.
  //
  // Dette gør at ESP32 kan lytte på alle WiFi-pakker
  // i området og ikke kun pakker sendt til ESP32 selv.
  esp_wifi_set_promiscuous(true);

  // Registrerer callback-funktionen som skal håndtere
  // alle modtagne WiFi-pakker.
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

  Serial.println("Sniffer startet"); // Bekræfter at sniffing er startet
}

// Loop-funktion som kører konstant.
//
// Bruges til:
// - at holde MQTT-forbindelsen aktiv
// - vise statistik over registrerede enheder
void loop() {
  mqttClient.loop();

  static unsigned long lastCountPrint = 0; // Holder MQTT-forbindelsen aktiv

  if (millis() - lastCountPrint >= 10000) { // Kører hvert 10. sekund
    lastCountPrint = millis(); // Opdaterer tidspunkt for sidste udskrift

    // Udskriver antal registrerede enheder
    Serial.print("Antal unikke enheder set: ");
    Serial.println(deviceCount);
  }
}