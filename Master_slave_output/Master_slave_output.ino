#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <FirebaseESP8266.h>
#include <vector>

// ============================
// 🔧 Configuration
// ============================
#define WIFI_SSID "Kunz"
#define WIFI_PASSWORD "123456781"
#define FIREBASE_HOST "esp-dashboard-50f8b-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH ""

// Firebase and Web Server Objects
FirebaseData fbdo;
ESP8266WebServer server(80);

// ============================
// 🌐 HTML Page
// ============================
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8" />
  <title>ESP8266 Dashboard</title>
  <style>
    body { font-family: Arial; text-align: center; background: #121212; color: #fff; }
    h1 { color: #0ff; }
    button {
      background: #0ff; border: none; padding: 15px 25px; margin: 10px;
      font-size: 18px; border-radius: 8px; cursor: pointer; color: #000;
    }
    button:hover { background: #00aaff; }
  </style>
</head>
<body>
  <h1>ESP8266 Remote Control</h1>
  <button onclick="sendCommand('ON')">TURN ON</button>
  <button onclick="sendCommand('OFF')">TURN OFF</button>
  <h3 id="status">Status: Unknown</h3>

  <script>
    function sendCommand(cmd) {
      fetch("/command?state=" + cmd)
      .then(r => r.text())
      .then(t => document.getElementById("status").innerText = "Status: " + t);
    }
  </script>
</body>
</html>
)rawliteral";

// ============================
// ⚙️ Setup
// ============================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase setup
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Web routes
  server.on("/", []() {
    server.send_P(200, "text/html", HTML_PAGE);
  });

  server.on("/command", []() {
    if (server.hasArg("state")) {
      String state = server.arg("state");
      Serial.println("Received command: " + state);

      if (state == "ON") {
        Firebase.setString(fbdo, "/device/state", "ON");
      } else {
        Firebase.setString(fbdo, "/device/state", "OFF");
      }

      server.send(200, "text/plain", state);
    } else {
      server.send(400, "text/plain", "Missing state parameter");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

// ============================
// 🔁 Loop
// ============================
void loop() {
  server.handleClient();
}
