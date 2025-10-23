// Slave.ino
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* AP_SSID = "ESP_MASTER";
const char* AP_PASS = "12345678";
const char* MASTER_IP = "192.168.4.1";
const int MASTER_PORT = 4210;
const unsigned long SEND_INTERVAL = 1000UL;

const int pins[4] = {0, 1, 2, 3};

WiFiUDP Udp;
unsigned long lastSend = 0;
uint8_t myId = 0;

uint8_t macIdFromLastByte() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  uint8_t b = mac[5];
  if (b == 0) b = 1;
  return b;
}

void connectToAP() {
  WiFi.begin(AP_SSID, AP_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) delay(200);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }
  myId = macIdFromLastByte();

  connectToAP();
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Joined AP, IP=%s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("Failed to join AP");

  Udp.begin(MASTER_PORT);
  lastSend = 0;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToAP();
    delay(200);
  }

  // listen for UDP command from master
  int len = Udp.parsePacket();
  if (len > 0) {
    char buf[32];
    int r = Udp.read(buf, sizeof(buf) - 1);
    if (r > 0) {
      buf[r] = 0;
      int pin;
      if (sscanf(buf, "CMD,%d", &pin) == 1 && pin >= 0 && pin < 4) {
        digitalWrite(pins[pin], !digitalRead(pins[pin]));
        Serial.printf("Toggled GPIO%d\n", pin);
      }
    }
  }

  unsigned long now = millis();
  if (now - lastSend >= SEND_INTERVAL) {
    lastSend = now;
    int d0 = digitalRead(pins[0]);
    int d1 = digitalRead(pins[1]);
    int d2 = digitalRead(pins[2]);
    int d3 = digitalRead(pins[3]);
    char msg[32];
    snprintf(msg, sizeof(msg), "%d,%d,%d,%d,%d", myId, d0, d1, d2, d3);
    Udp.beginPacket(MASTER_IP, MASTER_PORT);
    Udp.write((uint8_t*)msg, strlen(msg));
    Udp.endPacket();
  }
}