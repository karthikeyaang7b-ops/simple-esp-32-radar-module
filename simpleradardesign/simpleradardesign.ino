#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ESP32Servo.h>
#include <DHT.h>

// --- Network Credentials ---
const char* ssid = "<wifi name>";
const char* password = "<password>";

// --- Pin Definitions (Left-Side Only) ---
#define DHTPIN 2
#define DHTTYPE DHT11
#define SERVO_PIN 15
#define IR_PIN 4
#define BUZZER_PIN 18
#define LDR_PIN 5
#define LED_RED_PIN 19 // Connected WITH a resistor!

// --- Object Initializations ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Servo radarServo;
DHT dht(DHTPIN, DHTTYPE);

// --- Radar Global variables ---
int sweepTargetAngle = 180; // User adjustable sweep width (e.g., 45, 90, 180)
int sweepSpeedDelay = 30;   // User adjustable delay in ms (lower = faster)
int currentAngle = 0;
bool incrementing = true;
unsigned long lastMoveTime = 0;
unsigned long lastDHTTime = 0;
String systemState = "ACTIVE"; // ACTIVE, SLEEP, LOCKED

// HTML/CSS/JS Source for Tactical Web Interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>TACTICAL RADAR OPERATING SYSTEM</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { background-color: #030a05; color: #33ff33; font-family: 'Courier New', monospace; margin: 20px; text-shadow: 0 0 5px #33ff33; }
        .grid-container { display: grid; grid-template-columns: 2fr 1fr; gap: 20px; }
        .panel { border: 2px solid #33ff33; padding: 15px; background: rgba(5, 20, 5, 0.8); box-shadow: inset 0 0 10px #33ff33; }
        h2 { border-bottom: 1px dashed #33ff33; padding-bottom: 5px; margin-top: 0; }
        .status-box { font-size: 1.2em; font-weight: bold; padding: 5px; margin-bottom: 10px; border: 1px solid #33ff33; text-align: center; }
        .alert { background-color: #ff0000; color: #000; text-shadow: none; animation: blink 1s infinite; }
        input, select, button { background: #000; color: #33ff33; border: 1px solid #33ff33; padding: 5px; font-family: 'Courier New', monospace; }
        button:hover { background: #33ff33; color: #000; cursor: pointer; }
        @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0; } 100% { opacity: 1; } }
    </style>
</head>
<body>
    <h1>[ DEFCON SCANNER v4.1 ]</h1>
    <div class="grid-container">
        <!-- Left Side: Telemetry & Controls -->
        <div class="panel">
            <h2>RADAR TELEMETRY FEED</h2>
            <div id="sysStatus" class="status-box">SYSTEM STATE: LOADING...</div>
            <p>CURRENT SWEEP ANGLE: <span id="angleValue">0</span>&deg;</p>
            <p>SECTOR TARGET LOCK: <span id="targetValue">CLEAR</span></p>
            <hr style="border-color:#33ff33;">
            <h3>TACTICAL CONTROLS</h3>
            <label>SWEEP SECTOR BOUNDARY: </label>
            <select id="angleSelect" onchange="updateControls()">
                <option value="45">45 Degrees Sector</option>
                <option value="90">90 Degrees Sector</option>
                <option value="180" selected>180 Degrees Full Sweep</option>
            </select>
            <br><br>
            <label>SWEEP MOTOR SPEED: </label>
            <input type="range" id="speedSelect" min="10" max="100" value="30" onchange="updateControls()">
        </div>
        
        <!-- Right Side: Climate & Comms -->
        <div class="panel">
            <h2>ENVIRON FEED (DHT11)</h2>
            <p>AMBIENT TEMP: <span id="tempValue">0</span> &deg;C</p>
            <p>HUMIDITY INDEX: <span id="humValue">0</span>%</p>
            <p>LIGHT VALUE: <span id="lightValue">0</span></p>
        </div>
    </div>

    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        window.addEventListener('load', initWebSocket);

        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onmessage = onMessage;
        }

        function onMessage(event) {
            var data = JSON.parse(event.data);
            if(data.type === "telemetry") {
                document.getElementById("angleValue").innerHTML = data.angle;
                var statusBox = document.getElementById("sysStatus");
                statusBox.innerHTML = "SYSTEM STATE: " + data.state;
                
                if(data.state === "LOCKED") {
                    statusBox.className = "status-box alert";
                    document.getElementById("targetValue").innerHTML = "!!! TARGET ACQUIRED !!!";
                } else {
                    statusBox.className = "status-box";
                    document.getElementById("targetValue").innerHTML = "CLEAR";
                }
            } else if (data.type === "env") {
                document.getElementById("tempValue").innerHTML = data.temp;
                document.getElementById("humValue").innerHTML = data.hum;
                document.getElementById("lightValue").innerHTML = data.light;
            }
        }

        function updateControls() {
            var angle = document.getElementById("angleSelect").value;
            var speed = document.getElementById("speedSelect").value;
            var command = { angle: parseInt(angle), speed: parseInt(speed) };
            websocket.send(JSON.stringify(command));
        }
    </script>
</body>
</html>
)rawliteral";

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->opcode == WS_TEXT) {
    data[len] = 0;
    // Simple command parsing from WebUI controls
    String message = (char*)data;
    if(message.indexOf("angle") >= 0) {
       // Super basic text sniffing to extract control integers quickly
       int angleIdx = message.indexOf("angle") + 7;
       int speedIdx = message.indexOf("speed") + 7;
       sweepTargetAngle = message.substring(angleIdx, message.indexOf(",", angleIdx)).toInt();
       sweepSpeedDelay = message.substring(speedIdx, message.indexOf("}", speedIdx)).toInt();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected\n", client->id());
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  
  radarServo.attach(SERVO_PIN);
  dht.begin();

  // Connect to Local Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nAuthenticated. IP Address: ");
  Serial.println(WiFi.localIP());

  // Attach WebSocket Events
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Serve Dashboard HTML Page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  server.begin();
}

void loop() {
  ws.cleanupClients(); 
  unsigned long currentTime = millis();

  // 1. PROCESS ENVIRONMENTAL METRICS (Every 2 Seconds)
  if (currentTime - lastDHTTime >= 2000) {
    lastDHTTime = currentTime;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // Bypassing the LDR sensor by forcing a bright ambient value
    int light = 800; 

    if (isnan(t)) t = 0.0;
    if (isnan(h)) h = 0.0;

    // Force system to stay awake 24/7
    systemState = "ACTIVE"; 

    String envJson = "{\"type\":\"env\",\"temp\":" + String(t) + ",\"hum\":" + String(h) + ",\"light\":" + String(light) + "}";
    ws.textAll(envJson);
  } // <-- Closes environmental block

  // 2. PROCESS RADAR SECTOR DATA (Speed Controlled Sweep)
  if (systemState != "SLEEP" && (currentTime - lastMoveTime >= sweepSpeedDelay)) {
    lastMoveTime = currentTime;

    // Check IR sensor status
    int targetSpotted = digitalRead(IR_PIN);

    if (targetSpotted == LOW) {
      // Threat Track Engaged
      systemState = "LOCKED";
      digitalWrite(LED_RED_PIN, HIGH);
      tone(BUZZER_PIN, 1500, 100);       
    } else {
      // Normal Recon Sweep
      systemState = "ACTIVE";
      digitalWrite(LED_RED_PIN, LOW);
      noTone(BUZZER_PIN);

      // Advance the sweep mechanically
      radarServo.write(currentAngle);

      if (incrementing) {
        currentAngle += 2;
        if (currentAngle >= sweepTargetAngle) incrementing = false;
      } else {
        currentAngle -= 2;
        if (currentAngle <= 0) incrementing = true;
      }
    } // <-- Closes targetSpotted ELSE block

    // Direct, structural payload send to ensure UI renders
    String telemetryJson = "{\"type\":\"telemetry\",\"angle\":" + String(currentAngle) + ",\"state\":\"" + systemState + "\"}";
    ws.textAll(telemetryJson);
  } // <-- Closes radar sector data block
} // <-- Closes the entire void loop() function