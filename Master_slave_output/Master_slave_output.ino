// Master.ino
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

const char* AP_SSID = "ESP_MASTER";
const char* AP_PASS = "12345678";
const int UDP_PORT = 4210;
const int MAX_SLAVES = 20;

struct Slave {
  uint8_t id;
  IPAddress ip;        // NEW: store slave IP for sending commands
  int8_t s[4];
  unsigned long seen;
};

Slave slaves[MAX_SLAVES];
int slaveCount = 0;

WiFiUDP Udp;
ESP8266WebServer server(80);

const char page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP Master Dashboard</title>
<style>body {
  font-family: Arial, sans-serif;
  background: #f4f6f8;
  margin: 0;
  padding: 0;
  color: #222;
  text-align: center;
}

header {
  background: #2c3e50;
  color: #fff;
  padding: 1rem;
}

h1, h2 {
  margin: 0.5rem 0;
}

.master-controls {
  margin: 1rem auto;
  background: #fff;
  padding: 1rem;
  border-radius: 12px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
  width: 90%;
  max-width: 400px;
}

.gpio-buttons {
  display: flex;
  justify-content: center;
  gap: 1rem;
  margin-top: 10px;
}

.toggle-btn {
  padding: 10px 20px;
  border: none;
  border-radius: 8px;
  color: #fff;
  font-weight: bold;
  cursor: pointer;
  transition: background 0.3s ease, transform 0.2s ease;
}

.toggle-btn.off {
  background: #2ecc71;
}

.toggle-btn.on {
  background: #e74c3c;
}

.toggle-btn:hover {
  transform: scale(1.05);
}

.slave-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 1rem;
  padding: 1rem;
  max-width: 900px;
  margin: 0 auto;
}

.slave-card {
  background: #fff;
  border-radius: 12px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
  padding: 1rem;
  transition: transform 0.2s ease;
}

.slave-card:hover {
  transform: translateY(-4px);
}

.slave-header {
  font-weight: bold;
  margin-bottom: 8px;
}

.slave-status {
  font-size: 0.9em;
  color: #888;
  margin-bottom: 8px;
}

.slave-gpios {
  display: flex;
  justify-content: center;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.slave-btn {
  padding: 8px 16px;
  border: none;
  border-radius: 6px;
  color: #fff;
  font-weight: bold;
  cursor: pointer;
  transition: background 0.3s ease;
}

.slave-btn.on {
  background: #e74c3c;
}

.slave-btn.off {
  background: #2ecc71;
}
</style>
<script>const MASTER_IP = "192.168.4.1";

async function fetchSlaves() {
  try {
    const res = await fetch(`http://${MASTER_IP}/json`);
    const data = await res.json();
    displaySlaves(data);
  } catch (err) {
    console.error("Error fetching slave data:", err);
  }
}

function displaySlaves(slaves) {
  const container = document.getElementById("slaves");
  container.innerHTML = "";

  if (slaves.length === 0) {
    container.innerHTML = "<p>No slaves connected</p>";
    return;
  }

  slaves.forEach(slave => {
    const card = document.createElement("div");
    card.className = "slave-card";

    const header = document.createElement("div");
    header.className = "slave-header";
    header.textContent = `Slave #${slave.id}`;

    const status = document.createElement("div");
    status.className = "slave-status";
    status.textContent = `Last seen: ${(slave.seen / 1000).toFixed(1)}s ago`;

    const gpioContainer = document.createElement("div");
    gpioContainer.className = "slave-gpios";

    slave.s.forEach((val, i) => {
      const btn = document.createElement("button");
      btn.className = "slave-btn " + (val === 1 ? "on" : "off");
      btn.textContent = `GPIO${i}`;
      btn.onclick = () => toggleSlave(slave.id, i);
      gpioContainer.appendChild(btn);
    });

    card.appendChild(header);
    card.appendChild(status);
    card.appendChild(gpioContainer);
    container.appendChild(card);
  });
}

async function toggleSlave(id, pin) {
  try {
    await fetch(`http://${MASTER_IP}/toggle_slave?id=${id}&pin=${pin}`);
    setTimeout(fetchSlaves, 300);
  } catch (err) {
    console.error("Error toggling slave:", err);
  }
}

async function toggleMaster(pin) {
  const btn = document.getElementById(`gpio${pin}`);
  btn.classList.toggle("on");
  btn.classList.toggle("off");

  try {
    await fetch(`http://${MASTER_IP}/toggle?pin=${pin}`);
  } catch (err) {
    console.error("Error toggling master pin:", err);
  }
}

setInterval(fetchSlaves, 1000);
window.onload = fetchSlaves;
</script>
</head>
<body>
  <header>
    <h1>ESP Master Dashboard</h1>
  </header>

  <section class="master-controls">
    <h2>Master GPIO Controls</h2>
    <div class="gpio-buttons">
      <button id="gpio0" class="toggle-btn off" onclick="toggleMaster(0)">GPIO 0</button>
      <button id="gpio2" class="toggle-btn off" onclick="toggleMaster(2)">GPIO 2</button>
    </div>
  </section>

  <section>
    <h2>Connected Slaves</h2>
    <div id="slaves" class="slave-grid"></div>
  </section>
</body>
</html>


)=====";

int findSlaveIndex(uint8_t id) {
  for (int i = 0; i < slaveCount; ++i)
    if (slaves[i].id == id) return i;
  return -1;
}

void addOrUpdateSlave(uint8_t id, IPAddress ip, int s0, int s1, int s2, int s3) {
  int idx = findSlaveIndex(id);
  if (idx == -1) {
    if (slaveCount < MAX_SLAVES) {
      slaves[slaveCount].id = id;
      slaves[slaveCount].ip = ip;
      slaves[slaveCount].s[0] = s0;
      slaves[slaveCount].s[1] = s1;
      slaves[slaveCount].s[2] = s2;
      slaves[slaveCount].s[3] = s3;
      slaves[slaveCount].seen = millis();
      slaveCount++;
    }
  } else {
    slaves[idx].ip = ip;
    slaves[idx].s[0] = s0;
    slaves[idx].s[1] = s1;
    slaves[idx].s[2] = s2;
    slaves[idx].s[3] = s3;
    slaves[idx].seen = millis();
  }
}

void handleJson() {
  String out = "[";
  for (int i = 0; i < slaveCount; ++i) {
    out += "{";
    out += "\"id\":" + String(slaves[i].id) + ",";
    out += "\"seen\":" + String(millis() - slaves[i].seen) + ",";
    out += "\"s\":[" + String(slaves[i].s[0]) + "," + String(slaves[i].s[1]) + "," + String(slaves[i].s[2]) + "," + String(slaves[i].s[3]) + "]";
    out += "}";
    if (i < slaveCount - 1) out += ",";
  }
  out += "]";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", out);
}

void handleToggle() {
  if (!server.hasArg("pin")) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "text/plain", "Missing pin");
    return;
  }
  int pin = server.arg("pin").toInt();
  if (pin != 0 && pin != 2) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "text/plain", "Invalid pin");
    return;
  }
  int s = digitalRead(pin);
  digitalWrite(pin, !s);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// NEW: Toggle a slave¡¯s pin remotely
void handleToggleSlave() {
  if (!server.hasArg("id") || !server.hasArg("pin")) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "text/plain", "Missing args");
    return;
  }
  int id = server.arg("id").toInt();
  int pin = server.arg("pin").toInt();
  int idx = findSlaveIndex(id);
  if (idx == -1) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(404, "text/plain", "Slave not found");
    return;
  }
  char msg[16];
  snprintf(msg, sizeof(msg), "CMD,%d", pin);
  Udp.beginPacket(slaves[idx].ip, UDP_PORT);
  Udp.write(msg);
  Udp.endPacket();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(200);
  Serial.printf("AP started IP=%s\n", WiFi.softAPIP().toString().c_str());

  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  server.on("/", []() { server.send_P(200, "text/html", page); });
  server.on("/json", handleJson);
  server.on("/toggle", handleToggle);
  server.on("/toggle_slave", handleToggleSlave); // NEW
  server.begin();

  Udp.begin(UDP_PORT);
  Serial.printf("UDP listening on %d\n", UDP_PORT);
}

void loop() {
  server.handleClient();

  int len = Udp.parsePacket();
  if (len > 0) {
    char buf[64];
    int r = Udp.read(buf, sizeof(buf) - 1);
    if (r <= 0) return;
    buf[r] = 0;
    IPAddress ip = Udp.remoteIP();

    int id, d0, d1, d2, d3;
    int parsed = sscanf(buf, "%d,%d,%d,%d,%d", &id, &d0, &d1, &d2, &d3);
    if (parsed == 5 && id > 0) {
      addOrUpdateSlave((uint8_t)id, ip, d0, d1, d2, d3);
      Serial.printf("Pkt from %d [%s]: %d,%d,%d,%d\n", id, ip.toString().c_str(), d0, d1, d2, d3);
    }
  }
}
