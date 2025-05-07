#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "Arnoy";         // Your WiFi SSID
const char* password = "uxoricide"; // Your WiFi Password

ESP8266WebServer server(80);
String sonarData = "";

void setup() {
  Serial.begin(9600);  // Communication with Mega

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Routes
  server.on("/", handleRoot);
  server.on("/cmd", HTTP_GET, handleCommand);
  server.on("/sonar", HTTP_GET, []() {
    server.send(200, "text/plain", sonarData);
  });

  server.begin();
  Serial.println("Server started.");
}

void loop() {
  server.handleClient();

  // Read sonar data from Mega
  while (Serial.available()) {
    sonarData = Serial.readStringUntil('\n');
    sonarData.trim();
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <title>RC Car Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 40px; }
    h1 { font-size: 24px; }
  </style>
</head>
<body>
  <h1>RC Car Control</h1>
  <p><b>Controls:</b> W/A/S/D or Arrows = Move | Space = Stop<br>
  B = Blade On | O = Blade Off | 1 = Auto | 0 = Manual</p>
  <h3>Sonar Data:</h3>
  <pre id="sonar">Loading...</pre>

  <script>
    let pressedKeys = {};

    document.addEventListener('keydown', function(e) {
      if (pressedKeys[e.code]) return;  // Prevent repeat
      pressedKeys[e.code] = true;

      let key = e.key.toLowerCase();
      if (e.code === 'Space') key = 'space';
      sendKey(key);
    });

    document.addEventListener('keyup', function(e) {
      delete pressedKeys[e.code];

      const moveKeys = ['KeyW','KeyA','KeyS','KeyD','ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Space'];
      if (moveKeys.includes(e.code)) {
        sendKey('space');  // Stop when key is released
      }
    });

    function sendKey(key) {
      fetch("/cmd?key=" + key)
        .catch(err => console.error("Command send failed:", err));
    }

    function fetchSonar() {
      fetch("/sonar")
        .then(res => res.text())
        .then(data => {
          document.getElementById('sonar').innerText = data;
        });
    }

    setInterval(fetchSonar, 100); // 500ms refresh
  </script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleCommand() {
  String key = server.arg("key");
  String command = "";

  if (key == "w" || key == "arrowup") command = "FWD";
  else if (key == "s" || key == "arrowdown") command = "BWD";
  else if (key == "a" || key == "arrowleft") command = "LFT";
  else if (key == "d" || key == "arrowright") command = "RGT";
  else if (key == "space") command = "STP";
  else if (key == "b") command = "BON";
  else if (key == "o") command = "BOF";
  else if (key == "1") command = "AUTO";
  else if (key == "0") command = "MANU";

  if (command != "") {
    Serial.println(command);  // Send to Mega
    Serial.flush();
  }

  server.send(200, "text/plain", "Sent: " + command);
}