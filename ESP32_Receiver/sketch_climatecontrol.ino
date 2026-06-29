#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS 15
#define CAN_INT 4

MCP_CAN CAN(CAN_CS);
WebServer server(80);

uint8_t tempVal = 0;
uint8_t humVal  = 0;
uint8_t fanVal  = 0;
uint8_t acVal   = 0;
uint8_t occVal  = 0;

unsigned long lastCANTime = 0;

const char* ssid = "SmartClimate_Dashboard";
const char* password = "12345678";

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Climate</title>
<style>
body{
  margin:0;
  font-family:Arial,sans-serif;
  background:#fff6e8;
  text-align:center;
  color:#555;
}
h1{
  margin-top:55px;
  color:#ef5b00;
  font-size:42px;
}
h3{
  margin-top:-18px;
  font-weight:400;
}
.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:22px;
  padding:25px;
}
.card{
  background:white;
  border-radius:22px;
  padding:28px 10px;
  box-shadow:0 8px 20px rgba(0,0,0,0.12);
}
.label{
  font-size:20px;
  margin-bottom:15px;
}
.value{
  color:#ef5b00;
  font-size:42px;
  font-weight:bold;
}
.big{
  margin:0 25px;
  background:white;
  border-radius:22px;
  padding:30px 10px;
  box-shadow:0 8px 20px rgba(0,0,0,0.12);
}
.status{
  margin-top:30px;
  font-size:24px;
}
</style>
</head>

<body>

<h1>Smart Climate</h1>
<h3>CAN Bus Mobile Dashboard</h3>

<div class="grid">
  <div class="card">
    <div class="label">Temperature</div>
    <div class="value" id="temp">0 C</div>
  </div>

  <div class="card">
    <div class="label">Humidity</div>
    <div class="value" id="hum">0 %</div>
  </div>

  <div class="card">
    <div class="label">Fan Speed</div>
    <div class="value" id="fan">0 %</div>
  </div>

  <div class="card">
    <div class="label">AC Mode</div>
    <div class="value" id="ac">OFF</div>
  </div>
</div>

<div class="big">
  <div class="label">Occupancy</div>
  <div class="value" id="occ">EMPTY</div>
</div>

<div class="status" id="status">No recent CAN data</div>

<script>
function updateData(){
  fetch('/data')
  .then(response => response.json())
  .then(data => {
    document.getElementById("temp").innerHTML = data.temp + " C";
    document.getElementById("hum").innerHTML  = data.hum + " %";
    document.getElementById("fan").innerHTML  = data.fan + " %";
    document.getElementById("ac").innerHTML   = data.ac ? "ON" : "OFF";
    document.getElementById("occ").innerHTML  = data.occ ? "OCCUPIED" : "EMPTY";
    document.getElementById("status").innerHTML = data.recent ? "Receiving CAN data" : "No recent CAN data";
  });
}

setInterval(updateData, 500);
updateData();
</script>

</body>
</html>
)rawliteral";

void handleRoot()
{
  server.send_P(200, "text/html", MAIN_page);
}

void handleData()
{
  bool recent = (millis() - lastCANTime < 2000);

  String json = "{";
  json += "\"temp\":" + String(tempVal) + ",";
  json += "\"hum\":" + String(humVal) + ",";
  json += "\"fan\":" + String(fanVal) + ",";
  json += "\"ac\":" + String(acVal) + ",";
  json += "\"occ\":" + String(occVal) + ",";
  json += "\"recent\":" + String(recent ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting Smart Climate Dashboard...");
  Serial.println("ESP32 CS = GPIO15");
  Serial.println("CAN Speed = 125 kbps");
  Serial.println("MCP Crystal = 8 MHz");

  SPI.begin(18, 19, 23, CAN_CS);

  if (CAN.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK)
  {
    Serial.println("MCP2515 Init OK");
  }
  else
  {
    Serial.println("MCP2515 Init FAIL");
    while (1);
  }

  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);

  WiFi.softAP(ssid, password);

  Serial.print("Dashboard IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("Web server started");
  Serial.println("Waiting for CAN messages...");
}

void loop()
{
  server.handleClient();

  while (!digitalRead(CAN_INT))
  {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    CAN.readMsgBuf(&rxId, &len, rxBuf);

    if (len == 0)
    {
      break;
    }

    Serial.print("ID: 0x");
    Serial.print(rxId, HEX);
    Serial.print(" LEN: ");
    Serial.print(len);
    Serial.print(" Data: ");

    for (byte i = 0; i < len; i++)
    {
      Serial.print(rxBuf[i]);
      Serial.print(" ");
    }

    Serial.println();

    if (rxId == 0x123 && len == 5 && rxBuf[0] > 0)
    {
      tempVal = rxBuf[0];
      humVal  = rxBuf[1];
      fanVal  = rxBuf[2];
      acVal   = rxBuf[3];
      occVal  = rxBuf[4];

      lastCANTime = millis();

      Serial.println("Dashboard updated");
    }
  }
}