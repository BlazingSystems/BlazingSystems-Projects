/*
  ESP8266 Wi-Fi AP Thermostat
  --------------------------------
  Features:
  - ESP8266 creates its own Wi-Fi Access Point
  - Local web thermostat UI with 4 tabs:
      1. Main
      2. Settings
      3. Calibration
      4. System
  - DS18B20 temperature probe
  - Relay control
  - Automatic thermostat control with hysteresis
  - Minimum relay ON/OFF delay / anti-short-cycle protection
  - Manual ON/OFF mode
  - Temperature calibration offset
  - Persistent settings in EEPROM
  - No Internet/cloud required

  Libraries:
  - ESP8266 core
  - OneWire
  - DallasTemperature

  Install:
  Arduino Library Manager:
    "OneWire"
    "DallasTemperature"

  Default wiring:
    DS18B20 DATA -> GPIO4 (D2)
    DS18B20 VCC  -> 3.3V
    DS18B20 GND  -> GND
    4.7k resistor between DATA and 3.3V

    Relay IN -> GPIO5 (D1)
    Relay VCC/GND -> appropriate relay supply

  IMPORTANT:
    Relay output is configurable as active LOW below.
    Mains voltage must be isolated and wired using properly rated
    hardware/enclosures by a qualified person.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// -------------------- PIN CONFIG --------------------

#define TEMP_PIN       4       // GPIO4 / D2
#define RELAY_PIN      5       // GPIO5 / D1

// Most ESP8266 relay modules are active LOW.
#define RELAY_ACTIVE_LOW true

// -------------------- WIFI AP -----------------------

const char* AP_SSID = "ESP8266-Thermostat";
char AP_PASS[24] = "";  // generated per device on boot

ESP8266WebServer server(80);

// -------------------- SENSOR -------------------------

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

float rawTemperature = NAN;
float temperatureC = NAN;
float calibrationOffset = 0.0;

// -------------------- SETTINGS -----------------------

struct Settings {
  uint32_t magic;

  float setpoint;
  float hysteresis;

  unsigned long minOnTime;
  unsigned long minOffTime;

  unsigned long sensorInterval;

  bool autoMode;
  bool relayManual;

  float calibration;

  uint8_t reserved[16];
};

Settings settings;

const uint32_t SETTINGS_MAGIC = 0x54484552; // "THER"

// -------------------- RUNTIME ------------------------

bool relayState = false;

unsigned long relayChangedAt = 0;
unsigned long lastSensorRead = 0;
unsigned long lastSave = 0;

bool sensorOK = false;

String lastReason = "Startup";

// -------------------- EEPROM -------------------------

void setDefaults() {
  settings.magic = SETTINGS_MAGIC;

  settings.setpoint = 25.0;
  settings.hysteresis = 0.5;

  // Minimum relay ON/OFF time:
  settings.minOnTime = 60000UL;    // 1 minute
  settings.minOffTime = 60000UL;   // 1 minute

  settings.sensorInterval = 2000UL; // 2 sec

  settings.autoMode = true;
  settings.relayManual = false;

  settings.calibration = 0.0;

  memset(settings.reserved, 0, sizeof(settings.reserved));
}

void loadSettings() {
  EEPROM.begin(sizeof(Settings));

  EEPROM.get(0, settings);

  if (settings.magic != SETTINGS_MAGIC ||
      isnan(settings.setpoint) ||
      settings.setpoint < -40 ||
      settings.setpoint > 100) {

    setDefaults();
    EEPROM.put(0, settings);
    EEPROM.commit();
  }

  calibrationOffset = settings.calibration;
}

void saveSettings() {
  settings.magic = SETTINGS_MAGIC;
  settings.calibration = calibrationOffset;

  EEPROM.put(0, settings);
  EEPROM.commit();

  lastSave = millis();
}

// -------------------- RELAY --------------------------

void writeRelay(bool on) {
  relayState = on;

  bool output = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(RELAY_PIN, output ? HIGH : LOW);

  relayChangedAt = millis();
}

unsigned long relayAge() {
  return millis() - relayChangedAt;
}

bool canTurnOn() {
  return !relayState && relayAge() >= settings.minOffTime;
}

bool canTurnOff() {
  return relayState && relayAge() >= settings.minOnTime;
}

void requestRelay(bool on, String reason) {

  if (on == relayState) return;

  if (on) {
    if (canTurnOn()) {
      writeRelay(true);
      lastReason = reason;
    }
  }
  else {
    if (canTurnOff()) {
      writeRelay(false);
      lastReason = reason;
    }
  }
}

// -------------------- SENSOR -------------------------

void readTemperature() {

  unsigned long now = millis();

  if (now - lastSensorRead < settings.sensorInterval)
    return;

  lastSensorRead = now;

  sensors.requestTemperatures();

  float t = sensors.getTempCByIndex(0);

  if (t == DEVICE_DISCONNECTED_C || t < -55 || t > 125) {
    sensorOK = false;
    temperatureC = NAN;
    return;
  }

  sensorOK = true;

  rawTemperature = t;
  temperatureC = rawTemperature + calibrationOffset;
}

// -------------------- THERMOSTAT ---------------------

void thermostatControl() {

  if (!sensorOK) {
    // Fail-safe: turn output OFF when sensor fails.
    requestRelay(false, "Sensor fault");
    return;
  }

  // Manual mode
  if (!settings.autoMode) {

    requestRelay(settings.relayManual,
                 settings.relayManual ? "Manual ON" : "Manual OFF");

    return;
  }

  // Automatic mode
  //
  // This behaves as a COOLING thermostat:
  //
  // Temperature >= setpoint + hysteresis -> ON
  // Temperature <= setpoint - hysteresis -> OFF

  float upper = settings.setpoint + settings.hysteresis;
  float lower = settings.setpoint - settings.hysteresis;

  if (!relayState && temperatureC >= upper) {
    requestRelay(true, "Auto: temperature high");
  }

  if (relayState && temperatureC <= lower) {
    requestRelay(false, "Auto: temperature reached");
  }
}

// -------------------- HTML ----------------------------

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP8266 Thermostat</title>

<style>
*{box-sizing:border-box}
body{
  margin:0;
  background:#101418;
  color:#eee;
  font-family:Arial,sans-serif;
}
.header{
  padding:18px;
  background:#181e24;
  border-bottom:1px solid #303840;
}
.header h1{margin:0;font-size:22px}
.header small{color:#9aa5ad}

.tabs{
  display:flex;
  background:#181e24;
  overflow-x:auto;
}
.tab{
  flex:1;
  min-width:110px;
  padding:14px 10px;
  border:0;
  background:#181e24;
  color:#aaa;
  font-size:15px;
}
.tab.active{
  background:#252e36;
  color:#fff;
  border-bottom:3px solid #4da3ff;
}

.page{
  display:none;
  padding:15px;
  max-width:800px;
  margin:auto;
}
.page.active{display:block}

.card{
  background:#181e24;
  border:1px solid #303840;
  border-radius:14px;
  padding:18px;
  margin-bottom:15px;
}

.bigtemp{
  font-size:58px;
  text-align:center;
  font-weight:bold;
  margin:20px 0 5px;
}
.center{text-align:center}
.status{
  display:inline-block;
  padding:8px 14px;
  border-radius:30px;
  background:#333;
  margin:10px 0;
}
.on{background:#126b39}
.off{background:#5c2424}

.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:12px;
}
@media(max-width:550px){
  .grid{grid-template-columns:1fr}
}

label{
  display:block;
  margin-bottom:6px;
  color:#aeb8c0;
  font-size:13px;
}
input,select{
  width:100%;
  padding:12px;
  border-radius:8px;
  border:1px solid #404a53;
  background:#0f1317;
  color:#fff;
  font-size:16px;
}
button.action{
  width:100%;
  padding:13px;
  border:0;
  border-radius:9px;
  background:#2d8cff;
  color:white;
  font-size:16px;
  margin-top:8px;
}
button.danger{background:#9d3535}
button.green{background:#167347}

.value{
  font-size:24px;
  font-weight:bold;
}
.row{
  display:flex;
  justify-content:space-between;
  padding:9px 0;
  border-bottom:1px solid #2c343b;
}
.note{
  color:#9da7ae;
  font-size:13px;
  line-height:1.5;
}
.ok{color:#5ee39a}
.bad{color:#ff7373}
</style>
</head>

<body>

<div class="header">
  <h1>ESP8266 Thermostat</h1>
  <small>Local Wi-Fi controller</small>
</div>

<div class="tabs">
  <button class="tab active" onclick="tab('main',this)">Main</button>
  <button class="tab" onclick="tab('settings',this)">Settings</button>
  <button class="tab" onclick="tab('calibration',this)">Calibration</button>
  <button class="tab" onclick="tab('system',this)">System</button>
</div>

<!-- MAIN -->

<section id="main" class="page active">

<div class="card">

  <div class="center">ACTUAL TEMPERATURE</div>

  <div class="bigtemp" id="temp">--.-°C</div>

  <div class="center">
    <span id="sensor" class="status">Sensor...</span>
  </div>

</div>

<div class="grid">

  <div class="card">
    <label>SETPOINT</label>
    <div class="value" id="setpoint">--</div>
  </div>

  <div class="card">
    <label>HYSTERESIS</label>
    <div class="value" id="hysteresis">--</div>
  </div>

</div>

<div class="card">

  <div class="center">
    <label>RELAY</label>
    <div id="relay" class="status">---</div>
  </div>

  <div class="row">
    <span>Mode</span>
    <b id="mode">---</b>
  </div>

  <div class="row">
    <span>Reason</span>
    <b id="reason">---</b>
  </div>

  <button class="action green" onclick="manual(true)">FORCE ON</button>
  <button class="action danger" onclick="manual(false)">FORCE OFF</button>
  <button class="action" onclick="autoMode()">RETURN TO AUTO</button>

</div>

</section>


<!-- SETTINGS -->

<section id="settings" class="page">

<div class="card">

<h2>Thermostat Settings</h2>

<div class="grid">

<div>
<label>Setpoint °C</label>
<input id="s_setpoint" type="number" step="0.1">
</div>

<div>
<label>Hysteresis °C</label>
<input id="s_hysteresis" type="number" step="0.1">
</div>

<div>
<label>Minimum ON time (seconds)</label>
<input id="s_minon" type="number">
</div>

<div>
<label>Minimum OFF time (seconds)</label>
<input id="s_minoff" type="number">
</div>

<div>
<label>Sensor reading interval (seconds)</label>
<input id="s_interval" type="number">
</div>

</div>

<button class="action" onclick="saveSettings()">SAVE SETTINGS</button>

<p class="note">
For cooling: the relay turns ON when temperature reaches
Setpoint + Hysteresis and turns OFF at Setpoint - Hysteresis.
Minimum ON/OFF times protect compressors and reduce relay cycling.
</p>

</div>

<div class="card">

<h2>Manual Control</h2>

<p class="note">
Manual mode overrides automatic thermostat control until AUTO is selected again.
</p>

<button class="action green" onclick="manual(true)">Manual ON</button>
<button class="action danger" onclick="manual(false)">Manual OFF</button>
<button class="action" onclick="autoMode()">AUTO</button>

</div>

</section>


<!-- CALIBRATION -->

<section id="calibration" class="page">

<div class="card">

<h2>Temperature Calibration</h2>

<div class="row">
  <span>Raw sensor</span>
  <b id="raw">--</b>
</div>

<div class="row">
  <span>Correction</span>
  <b id="offset">--</b>
</div>

<div class="row">
  <span>Displayed</span>
  <b id="caltemp">--</b>
</div>

<label>Calibration offset °C</label>
<input id="cal" type="number" step="0.1">

<button class="action" onclick="saveCalibration()">SAVE CALIBRATION</button>

<p class="note">
Example: if a trusted thermometer reads 25.0°C while the probe reads
24.3°C, enter +0.7°C.
</p>

</div>

</section>


<!-- SYSTEM -->

<section id="system" class="page">

<div class="card">

<h2>System Information</h2>

<div class="row">
  <span>Wi-Fi AP</span>
  <b id="ssid">---</b>
</div>

<div class="row">
  <span>IP address</span>
  <b id="ip">---</b>
</div>

<div class="row">
  <span>ESP8266 uptime</span>
  <b id="uptime">---</b>
</div>

<div class="row">
  <span>Sensor</span>
  <b id="sensor2">---</b>
</div>

<div class="row">
  <span>Relay</span>
  <b id="relay2">---</b>
</div>

</div>

<div class="card">

<h2>Thermostat Safety</h2>

<p class="note">
If the temperature probe becomes disconnected, the firmware turns the
relay OFF automatically.
</p>

<p class="note">
The minimum relay ON/OFF timers remain active even when using automatic
control. This helps prevent rapid cycling.
</p>

</div>

</section>


<script>

function tab(id,btn){

  document.querySelectorAll('.page')
    .forEach(x=>x.classList.remove('active'));

  document.querySelectorAll('.tab')
    .forEach(x=>x.classList.remove('active'));

  document.getElementById(id).classList.add('active');

  btn.classList.add('active');
}


function getStatus(){

 fetch('/api/status')
 .then(r=>r.json())
 .then(d=>{

   document.getElementById('temp').innerHTML =
     d.tempValid ? d.temp.toFixed(1)+'°C' : '--.-°C';

   document.getElementById('setpoint').innerHTML =
     d.setpoint.toFixed(1)+'°C';

   document.getElementById('hysteresis').innerHTML =
     d.hysteresis.toFixed(1)+'°C';

   let sensor=document.getElementById('sensor');

   sensor.innerHTML=d.sensor?'SENSOR OK':'SENSOR ERROR';
   sensor.className='status '+(d.sensor?'on':'off');

   let relay=document.getElementById('relay');

   relay.innerHTML=d.relay?'RELAY ON':'RELAY OFF';
   relay.className='status '+(d.relay?'on':'off');

   document.getElementById('mode').innerHTML =
     d.auto?'AUTO':'MANUAL';

   document.getElementById('reason').innerHTML=d.reason;

   document.getElementById('raw').innerHTML =
     d.rawValid ? d.raw.toFixed(2)+'°C' : '--';

   document.getElementById('offset').innerHTML =
     d.offset.toFixed(2)+'°C';

   document.getElementById('caltemp').innerHTML =
     d.tempValid ? d.temp.toFixed(2)+'°C':'--';

   document.getElementById('ssid').innerHTML=d.ssid;
   document.getElementById('ip').innerHTML=d.ip;
   document.getElementById('uptime').innerHTML=d.uptime;

   document.getElementById('sensor2').innerHTML =
     d.sensor?'OK':'ERROR';

   document.getElementById('relay2').innerHTML =
     d.relay?'ON':'OFF';

   document.getElementById('s_setpoint').value=d.setpoint;
   document.getElementById('s_hysteresis').value=d.hysteresis;
   document.getElementById('s_minon').value=d.minOn;
   document.getElementById('s_minoff').value=d.minOff;
   document.getElementById('s_interval').value=d.interval;
   document.getElementById('cal').value=d.offset;

 });

}


function saveSettings(){

 let data={
   setpoint:parseFloat(document.getElementById('s_setpoint').value),
   hysteresis:parseFloat(document.getElementById('s_hysteresis').value),
   minOn:parseInt(document.getElementById('s_minon').value),
   minOff:parseInt(document.getElementById('s_minoff').value),
   interval:parseInt(document.getElementById('s_interval').value)
 };

 fetch('/api/settings',{
   method:'POST',
   headers:{'Content-Type':'application/json'},
   body:JSON.stringify(data)
 })
 .then(()=>alert('Settings saved'));

}


function saveCalibration(){

 let offset=parseFloat(document.getElementById('cal').value);

 fetch('/api/calibration',{
   method:'POST',
   headers:{'Content-Type':'application/json'},
   body:JSON.stringify({offset:offset})
 })
 .then(()=>alert('Calibration saved'));

}


function manual(state){

 fetch('/api/manual?state='+(state?'1':'0'))
 .then(()=>getStatus());

}


function autoMode(){

 fetch('/api/auto')
 .then(()=>getStatus());

}


getStatus();

setInterval(getStatus,2000);

</script>

</body>
</html>
)rawliteral";

// -------------------- JSON ---------------------------

String uptimeString() {

  unsigned long sec = millis() / 1000;

  unsigned long days = sec / 86400;
  sec %= 86400;

  unsigned long hrs = sec / 3600;
  sec %= 3600;

  unsigned long mins = sec / 60;
  sec %= 60;

  char buf[40];

  snprintf(buf, sizeof(buf),
           "%lu d %02lu:%02lu:%02lu",
           days, hrs, mins, sec);

  return String(buf);
}

String jsonStatus() {

  String j = "{";

  j += "\"tempValid\":";
  j += sensorOK ? "true" : "false";

  j += ",\"temp\":";
  j += String(temperatureC, 2);

  j += ",\"rawValid\":";
  j += sensorOK ? "true" : "false";

  j += ",\"raw\":";
  j += String(rawTemperature, 2);

  j += ",\"sensor\":";
  j += sensorOK ? "true" : "false";

  j += ",\"relay\":";
  j += relayState ? "true" : "false";

  j += ",\"auto\":";
  j += settings.autoMode ? "true" : "false";

  j += ",\"setpoint\":";
  j += String(settings.setpoint, 2);

  j += ",\"hysteresis\":";
  j += String(settings.hysteresis, 2);

  j += ",\"offset\":";
  j += String(calibrationOffset, 2);

  j += ",\"minOn\":";
  j += String(settings.minOnTime / 1000UL);

  j += ",\"minOff\":";
  j += String(settings.minOffTime / 1000UL);

  j += ",\"interval\":";
  j += String(settings.sensorInterval / 1000UL);

  j += ",\"ssid\":\"";
  j += AP_SSID;
  j += "\"";

  j += ",\"ip\":\"";
  j += WiFi.softAPIP().toString();
  j += "\"";

  j += ",\"uptime\":\"";
  j += uptimeString();
  j += "\"";

  j += ",\"reason\":\"";
  j += lastReason;
  j += "\"";

  j += "}";

  return j;
}

// -------------------- WEB HANDLERS -------------------

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  server.send(200, "application/json", jsonStatus());
}

void handleSettings() {

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing JSON");
    return;
  }

  String body = server.arg("plain");

  float setpoint = extractFloat(body, "setpoint", settings.setpoint);
  float hysteresis = extractFloat(body, "hysteresis", settings.hysteresis);

  long minOn = extractLong(body, "minOn", settings.minOnTime / 1000UL);
  long minOff = extractLong(body, "minOff", settings.minOffTime / 1000UL);
  long interval = extractLong(body, "interval", settings.sensorInterval / 1000UL);

  if (setpoint < -40) setpoint = -40;
  if (setpoint > 100) setpoint = 100;

  if (hysteresis < 0.1) hysteresis = 0.1;
  if (hysteresis > 20) hysteresis = 20;

  if (minOn < 0) minOn = 0;
  if (minOff < 0) minOff = 0;

  if (interval < 1) interval = 1;
  if (interval > 60) interval = 60;

  settings.setpoint = setpoint;
  settings.hysteresis = hysteresis;

  settings.minOnTime = (unsigned long)minOn * 1000UL;
  settings.minOffTime = (unsigned long)minOff * 1000UL;

  settings.sensorInterval = (unsigned long)interval * 1000UL;

  saveSettings();

  server.send(200, "text/plain", "OK");
}

void handleCalibration() {

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing JSON");
    return;
  }

  String body = server.arg("plain");

  float offset = extractFloat(body, "offset", calibrationOffset);

  if (offset < -20) offset = -20;
  if (offset > 20) offset = 20;

  calibrationOffset = offset;
  settings.calibration = offset;

  saveSettings();

  // Recalculate displayed temperature immediately.
  if (sensorOK)
    temperatureC = rawTemperature + calibrationOffset;

  server.send(200, "text/plain", "OK");
}

void handleManual() {

  if (!server.hasArg("state")) {
    server.send(400, "text/plain", "Missing state");
    return;
  }

  settings.autoMode = false;
  settings.relayManual = server.arg("state") == "1";

  thermostatControl();

  saveSettings();

  server.send(200, "text/plain", "OK");
}

void handleAuto() {

  settings.autoMode = true;

  thermostatControl();

  saveSettings();

  server.send(200, "text/plain", "OK");
}

// -------------------- SIMPLE JSON PARSER -------------

float extractFloat(String body, String key, float fallback) {

  String search = "\"" + key + "\"";

  int p = body.indexOf(search);

  if (p < 0)
    return fallback;

  p = body.indexOf(':', p);

  if (p < 0)
    return fallback;

  p++;

  while (p < (int)body.length() &&
         (body[p] == ' ' || body[p] == '"'))
    p++;

  int end = p;

  while (end < (int)body.length() &&
         String("0123456789.-+eE").indexOf(body[end]) >= 0)
    end++;

  return body.substring(p, end).toFloat();
}

long extractLong(String body, String key, long fallback) {

  return (long)extractFloat(body, key, (float)fallback);
}

// -------------------- SETUP --------------------------

void setup() {

  Serial.begin(115200);
  delay(100);

  pinMode(RELAY_PIN, OUTPUT);

  // Start safely OFF.
  bool offOutput = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(RELAY_PIN, offOutput);

  relayState = false;
  relayChangedAt = millis();

  loadSettings();

  sensors.begin();
  sensors.setResolution(12);

  // Wi-Fi Access Point
  // Avoid one universal public default password across every compiled device.
  snprintf(AP_PASS, sizeof(AP_PASS), "BlazeTherm-%06lX", (unsigned long)ESP.getChipId());
  WiFi.mode(WIFI_AP);

  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP8266 THERMOSTAT");
  Serial.println("================================");
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP Password: ");
  Serial.println(AP_PASS);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on("/api/calibration", HTTP_POST, handleCalibration);
  server.on("/api/manual", HTTP_GET, handleManual);
  server.on("/api/auto", HTTP_GET, handleAuto);

  server.begin();

  Serial.println("Web server started.");

  // Initial sensor reading
  sensors.requestTemperatures();

  float t = sensors.getTempCByIndex(0);

  if (t != DEVICE_DISCONNECTED_C &&
      t >= -55 &&
      t <= 125) {

    sensorOK = true;
    rawTemperature = t;
    temperatureC = rawTemperature + calibrationOffset;
  }

  lastSensorRead = millis();
}

// -------------------- LOOP ----------------------------

void loop() {

  server.handleClient();

  readTemperature();

  thermostatControl();

  yield();
}