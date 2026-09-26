#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Definitions
#define RELAY_PIN 26
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   2
#define OLED_CS   5
#define OLED_RESET 4

Adafruit_SSD1306 display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

const char* ssid = "SmartClassroom";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int peopleCount = 0;
bool relayState = false;
bool manualOverride = false;

unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 5000;

void applyRelay(bool on) {
  relayState = on;
  digitalWrite(RELAY_PIN, on ? LOW : HIGH);
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("SMART CLASSROOM");
  display.drawLine(0, 10, 128, 10, WHITE);
  display.setCursor(0, 15);
  display.print("People: ");
  display.println(peopleCount);
  display.setCursor(0, 30);
  display.print("Status: ");
  display.println(relayState ? "OCCUPIED" : "EMPTY");
  display.setCursor(0, 45);
  display.print("Supply: ");
  display.println(relayState ? "ON" : "OFF");
  if (manualOverride) {
    display.setCursor(0, 55);
    display.println("(Manual Override)");
  }
  display.display();
}

String buildStatusJson() {
  return "{\"people\":" + String(peopleCount) +
         ",\"relay\":" + String(relayState ? "true" : "false") +
         ",\"status\":\"" + String(relayState ? "OCCUPIED" : "EMPTY") + "\"" +
         ",\"manualOverride\":" + String(manualOverride ? "true" : "false") + "}";
}

void sendStatus() {
  ws.textAll(buildStatusJson());
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(buildStatusJson());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT) {
      String msg = String((char*)data).substring(0, len);
      if (msg == "MANUAL_ON") {
        manualOverride = true;
        applyRelay(true);
        updateOLED();
        sendStatus();
      } else if (msg == "MANUAL_OFF") {
        manualOverride = true;
        applyRelay(false);
        updateOLED();
        sendStatus();
      }
    }
  }
}

const char index_html[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Smart Classroom</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body { font-family: Arial; text-align: center; background: #1a1a1a; color: white; padding: 20px; }
    h1 { color: #44aaff; }
    .card { background: #2a2a2a; border-radius: 10px; padding: 20px; margin: 10px auto; max-width: 400px; }
    .value { font-size: 2em; font-weight: bold; }
    .occupied { color: #44ff44; }
    .empty { color: #ff4444; }
    .btn { border: none; padding: 15px 30px; font-size: 1em; border-radius: 8px; cursor: pointer; margin: 5px; }
    .btn-on { background: #44ff44; color: black; }
    .btn-off { background: #ff4444; color: white; }
    .btn-auto { background: #44aaff; color: black; margin-top: 8px; }
    .conn { font-size: 0.8em; margin-top: 10px; color: #888; }
  </style>
</head>
<body>
  <h1>Smart Classroom System</h1>
  <div class='card'>
    <p>People Detected</p>
    <div class='value' id='people'>--</div>
  </div>
  <div class='card'>
    <p>Room Status</p>
    <div class='value' id='status'>--</div>
  </div>
  <div class='card'>
    <p>Power Supply</p>
    <div class='value' id='relay'>--</div>
  </div>
  <div class='card'>
    <p>Manual Override</p>
    <button class='btn btn-on' onclick='send("MANUAL_ON")'>Force ON</button>
    <button class='btn btn-off' onclick='send("MANUAL_OFF")'>Force OFF</button>
    <br>
    <button class='btn btn-auto' onclick='clearOverride()' id='autoBtn' style='display:none;'>Resume Auto</button>
    <div id='overrideNote' style='color:#ffaa44; margin-top:8px; display:none;'>Manual override active</div>
  </div>
  <div class='conn' id='connStatus'>Connecting...</div>
  <script>
    var socket;
    function connect() {
      socket = new WebSocket('ws://' + location.hostname + '/ws');
      socket.onopen = function() {
        document.getElementById('connStatus').innerHTML = 'Connected';
      };
      socket.onclose = function() {
        document.getElementById('connStatus').innerHTML = 'Disconnected - retrying...';
        setTimeout(connect, 2000);
      };
      socket.onerror = function() { socket.close(); };
      socket.onmessage = function(event) {
        var data = JSON.parse(event.data);
        document.getElementById('people').innerHTML = data.people;
        var statusEl = document.getElementById('status');
        statusEl.innerHTML = data.status;
        statusEl.className = 'value ' + (data.relay ? 'occupied' : 'empty');
        document.getElementById('relay').innerHTML = data.relay ? 'ON' : 'OFF';
        var autoBtn = document.getElementById('autoBtn');
        var note = document.getElementById('overrideNote');
        if (data.manualOverride) {
          autoBtn.style.display = 'inline-block';
          note.style.display = 'block';
        } else {
          autoBtn.style.display = 'none';
          note.style.display = 'none';
        }
      };
    }
    connect();
    function send(msg) {
      if (msg == "MANUAL_ON") { fetch('/manual_on'); }
      else if (msg == "MANUAL_OFF") { fetch('/manual_off'); }
    }
    function clearOverride() { fetch('/manual_clear'); }
  </script>
</body>
</html>
)";

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  display.begin(SSD1306_SWITCHCAPVCC, 0);
  display.clearDisplay();
  display.display();

  WiFi.softAP(ssid, password);
  Serial.print("Hotspot IP: ");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/relay_on", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("count")) {
      peopleCount = request->getParam("count")->value().toInt();
    } else {
      peopleCount = 1;
    }
    if (!manualOverride) {
      applyRelay(true);
      updateOLED();
      sendStatus();
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/relay_off", HTTP_GET, [](AsyncWebServerRequest* request) {
    peopleCount = 0;
    if (!manualOverride) {
      applyRelay(false);
      updateOLED();
      sendStatus();
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/manual_off", HTTP_GET, [](AsyncWebServerRequest* request) {
    manualOverride = true;
    applyRelay(false);
    updateOLED();
    sendStatus();
    request->send(200, "text/plain", "Manual OFF");
  });

  server.on("/manual_on", HTTP_GET, [](AsyncWebServerRequest* request) {
    manualOverride = true;
    applyRelay(true);
    updateOLED();
    sendStatus();
    request->send(200, "text/plain", "Manual ON");
  });

  server.on("/manual_clear", HTTP_GET, [](AsyncWebServerRequest* request) {
    manualOverride = false;
    updateOLED();
    sendStatus();
    request->send(200, "text/plain", "Override cleared");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
  updateOLED();
}

void loop() {
  ws.cleanupClients();

  unsigned long now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeat = now;
    sendStatus();
  }
}