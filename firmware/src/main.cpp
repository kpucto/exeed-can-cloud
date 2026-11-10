#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsClient.h>
#include <Preferences.h>
#include <time.h>
#include <cstring>
#include "driver/twai.h"

// EXEED CAN ROUTER v2.0
// ESP32-S3 / 500 kbit/s / TWAI LISTEN ONLY.
// This firmware contains no CAN transmit calls.

static constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_4;
static constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_5;
static constexpr size_t FRAME_BUFFER_SIZE = 400;
static constexpr size_t ID_STATS_SIZE = 128;
static constexpr uint32_t CLOUD_SEND_INTERVAL_MS = 50;
static constexpr uint32_t STATUS_INTERVAL_MS = 5000;

static const char *AP_SSID = "EXEED_CAN_ROUTER";
static const char *AP_PASSWORD = "exeed123";
static const char *CLOUD_HOST = "exeed-can-cloud.kpucto.workers.dev";
static const uint16_t CLOUD_PORT = 443;
static const char *CLOUD_PATH = "/device";

static const char GTS_ROOT_R4[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)PEM";

WebServer server(80);
WebSocketsClient webSocket;
Preferences prefs;

struct CanFrame {
  uint32_t seq = 0, id = 0, ts = 0;
  uint8_t dlc = 0, data[8] = {};
  bool extended = false, remote = false;
};

struct IdStats {
  bool used = false;
  uint32_t id = 0, count = 0, firstSeen = 0, lastSeen = 0;
  uint32_t lastPeriod = 0, minPeriod = UINT32_MAX, maxPeriod = 0;
  uint8_t dlc = 0, lastData[8] = {};
  uint32_t changedBytes[8] = {};
};

CanFrame frameBuffer[FRAME_BUFFER_SIZE];
IdStats idStats[ID_STATS_SIZE];
uint32_t totalFrames = 0, cloudLastSentSeq = 0;
uint32_t rxErrors = 0, txErrors = 0, busErrors = 0, rxMissed = 0, rxOverrun = 0, txFailed = 0;
uint32_t lastStatusAt = 0, lastCloudSendAt = 0, lastProbeAt = 0, lastSerialFrameAt = 0;
uint32_t cloudSentBatches = 0, cloudSentFrames = 0;
bool canReady = false, cloudReady = false, cloudStarted = false;
bool ntpStarted = false, clockReady = false, probeSent = false;
String wifiSsid, wifiPassword, canState = "NOT_STARTED";

String hexId(uint32_t id, bool ext) {
  char b[12];
  snprintf(b, sizeof(b), ext ? "0x%08lX" : "0x%03lX", (unsigned long)id);
  return String(b);
}

String stateName(twai_state_t s) {
  switch (s) {
    case TWAI_STATE_STOPPED: return "STOPPED";
    case TWAI_STATE_RUNNING: return "RUNNING";
    case TWAI_STATE_BUS_OFF: return "BUS_OFF";
    case TWAI_STATE_RECOVERING: return "RECOVERING";
    default: return "UNKNOWN";
  }
}

IdStats *findIdStats(uint32_t id) {
  IdStats *freeSlot = nullptr;
  for (size_t i = 0; i < ID_STATS_SIZE; i++) {
    if (idStats[i].used && idStats[i].id == id) return &idStats[i];
    if (!idStats[i].used && !freeSlot) freeSlot = &idStats[i];
  }
  if (!freeSlot) {
    freeSlot = &idStats[0];
    for (size_t i = 1; i < ID_STATS_SIZE; i++)
      if (idStats[i].lastSeen < freeSlot->lastSeen) freeSlot = &idStats[i];
    *freeSlot = IdStats{};
  }
  freeSlot->used = true;
  freeSlot->id = id;
  freeSlot->firstSeen = millis();
  return freeSlot;
}

void saveFrame(const twai_message_t &msg) {
  const uint32_t now = millis();
  CanFrame &f = frameBuffer[totalFrames % FRAME_BUFFER_SIZE];
  f.seq = ++totalFrames;
  f.id = msg.identifier;
  f.ts = now;
  f.dlc = min((uint8_t)msg.data_length_code, (uint8_t)8);
  f.extended = msg.extd;
  f.remote = msg.rtr;
  for (uint8_t i = 0; i < 8; i++) f.data[i] = (i < f.dlc && !f.remote) ? msg.data[i] : 0;

  IdStats *s = findIdStats(f.id);
  if (s->count) {
    s->lastPeriod = now - s->lastSeen;
    s->minPeriod = min(s->minPeriod, s->lastPeriod);
    s->maxPeriod = max(s->maxPeriod, s->lastPeriod);
    for (uint8_t i = 0; i < f.dlc; i++)
      if (s->lastData[i] != f.data[i]) s->changedBytes[i]++;
  }
  s->count++;
  s->lastSeen = now;
  s->dlc = f.dlc;
  memcpy(s->lastData, f.data, 8);

  if (totalFrames <= 20 || now - lastSerialFrameAt >= 1000) {
    lastSerialFrameAt = now;
    Serial.printf("[CAN RX] #%lu ID=%s DLC=%u DATA=",
      (unsigned long)f.seq, hexId(f.id, f.extended).c_str(), f.dlc);
    for (uint8_t i = 0; i < f.dlc; i++) Serial.printf("%02X ", f.data[i]);
    Serial.printf("%s%s\n", f.extended ? " EXT" : " STD", f.remote ? " RTR" : "");
  }
}

void receiveCAN() {
  if (!canReady) return;
  twai_message_t msg = {};
  for (uint16_t i = 0; i < 250; i++) {
    if (twai_receive(&msg, 0) != ESP_OK) break;
    saveFrame(msg);
  }
}

void updateCANStatus() {
  if (!canReady) return;
  twai_status_info_t s = {};
  if (twai_get_status_info(&s) == ESP_OK) {
    canState = stateName(s.state);
    rxErrors = s.rx_error_counter;
    txErrors = s.tx_error_counter;
    busErrors = s.bus_error_count;
    rxMissed = s.rx_missed_count;
    rxOverrun = s.rx_overrun_count;
    txFailed = s.tx_failed_count;
  }
}

bool initializeCAN() {
  Serial.println("[CAN] TWAI install: 500 kbit/s, LISTEN ONLY");
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_LISTEN_ONLY);
  g.rx_queue_len = 1000;
  g.tx_queue_len = 0;
  g.alerts_enabled = TWAI_ALERT_ALL;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  esp_err_t e = twai_driver_install(&g, &t, &f);
  if (e != ESP_OK) {
    Serial.printf("[CAN ERROR] install: %s\n", esp_err_to_name(e));
    return false;
  }
  e = twai_start();
  if (e != ESP_OK) {
    Serial.printf("[CAN ERROR] start: %s\n", esp_err_to_name(e));
    twai_driver_uninstall();
    return false;
  }
  Serial.println("[CAN] RUNNING. TX queue disabled; no CAN transmit.");
  return true;
}

void loadCredentials() {
  prefs.begin("exeed-can", true);
  wifiSsid = prefs.getString("ssid", "");
  wifiPassword = prefs.getString("pass", "");
  prefs.end();
}

void startWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  bool ap = WiFi.softAP(AP_SSID, AP_PASSWORD, 6, false, 4);
  Serial.printf("[WIFI] Setup AP %s; IP=%s\n", ap ? "READY" : "FAILED", WiFi.softAPIP().toString().c_str());
  loadCredentials();
  if (wifiSsid.length()) {
    Serial.printf("[WIFI] Connecting to saved network: %s\n", wifiSsid.c_str());
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  } else {
    Serial.println("[WIFI] No saved network. Configure at http://192.168.4.1");
  }
}

void updateClock() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!ntpStarted) {
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    ntpStarted = true;
    Serial.println("[TIME] NTP started");
  }
  if (!clockReady && time(nullptr) > 1760000000) {
    clockReady = true;
    Serial.println("[TIME] Clock synchronized");
  }
}

const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>EXEED CAN ROUTER</title><style>
*{box-sizing:border-box}body{margin:0;background:#0b1220;color:#e8eef8;font:14px Arial;padding:15px}main{max-width:1250px;margin:auto}
.card{background:#151f30;border:1px solid #29384d;border-radius:12px;padding:14px;margin:10px 0}
h1{font-size:20px}.muted{color:#91a1b8}.stats{display:grid;grid-template-columns:repeat(4,1fr);gap:9px}
.big{font-size:25px;font-weight:bold;margin-top:8px}.good{color:#43dfa5}.bad{color:#ff7485}
input,button{background:#0b1525;color:#e8eef8;border:1px solid #34465e;border-radius:7px;padding:10px}
button{cursor:pointer}table{width:100%;border-collapse:collapse;font-size:12px;white-space:nowrap}
th,td{text-align:left;padding:9px;border-bottom:1px solid #29384d}th{color:#91a1b8}.wrap{overflow:auto;max-height:420px}.id{color:#76b5ff;font-family:monospace}
form{display:flex;gap:8px;flex-wrap:wrap}form input{min-width:180px;flex:1}
@media(max-width:650px){.stats{grid-template-columns:repeat(2,1fr)}.big{font-size:21px}}
</style></head><body><main>
<h1>EXEED CAN ROUTER v2.0</h1><p class="muted">ESP32-S3 · CAN 500 kbit/s · HARDWARE LISTEN ONLY</p>
<div class="card"><b>Настройка Wi-Fi</b><p class="muted">Подключись к точке EXEED_CAN_ROUTER и введи SSID/пароль автомобильного модема.</p>
<form method="POST" action="/wifi"><input name="ssid" placeholder="Wi-Fi SSID" required><input name="pass" type="password" placeholder="Wi-Fi пароль"><button>Сохранить и подключить</button></form></div>
<section class="stats">
<div class="card"><div class="muted">CAN state</div><div class="big" id="can">...</div></div>
<div class="card"><div class="muted">Всего кадров</div><div class="big" id="frames">0</div></div>
<div class="card"><div class="muted">Уникальных ID</div><div class="big" id="ids">0</div></div>
<div class="card"><div class="muted">Cloud WSS</div><div class="big" id="cloud">OFFLINE</div></div></section>
<div class="card"><b>Ошибки и соединение</b><p id="errors" class="muted">Ожидание...</p><p id="wifiState" class="muted"></p></div>
<div class="card"><b>Последние CAN-кадры</b><div class="wrap"><table><thead><tr><th>Время, ms</th><th>ID</th><th>Тип</th><th>DLC</th><th>DATA</th></tr></thead><tbody id="log"><tr><td colspan="5">Ожидание...</td></tr></tbody></table></div></div>
<div class="card"><b>CAN ID inventory</b><div class="wrap"><table><thead><tr><th>ID</th><th>Кадров</th><th>Период, ms</th><th>Мин/макс, ms</th><th>DLC</th><th>Последние данные</th></tr></thead><tbody id="inventory"></tbody></table></div></div>
</main><script>
let seq=0;const $=id=>document.getElementById(id);
function hx(n){return "0x"+Number(n).toString(16).toUpperCase().padStart(3,"0")}
async function poll(){try{const r=await fetch("/api/data?since="+seq,{cache:"no-store"}),d=await r.json();
$("can").textContent=d.canState;$("can").className="big "+(d.canState==="RUNNING"?"good":"bad");
$("frames").textContent=d.frames;$("ids").textContent=d.uniqueIds;$("cloud").textContent=d.cloud?"ONLINE":"OFFLINE";$("cloud").className="big "+(d.cloud?"good":"bad");
$("errors").textContent="RX "+d.rxErrors+" · TX "+d.txErrors+" · BUS "+d.busErrors+" · MISSED "+d.rxMissed+" · OVERRUN "+d.rxOverrun+" · TX FAILED "+d.txFailed+" · TWAI "+d.canState;
$("wifiState").textContent="Wi-Fi "+(d.sta?"CONNECTED":"OFFLINE")+" · Cloud sent "+d.cloudSentFrames+" frames / "+d.cloudSentBatches+" batches";
const log=$("log");for(const f of d.framesList){if(f.seq<=seq)continue;const tr=document.createElement("tr");[f.ts,hx(f.id),f.type,f.dlc,f.data].forEach(v=>{const td=document.createElement("td");td.textContent=v;tr.appendChild(td)});log.prepend(tr);while(log.children.length>80)log.lastChild.remove();seq=Math.max(seq,f.seq)}
$("inventory").innerHTML=d.inventory.map(x=>"<tr><td class='id'>"+hx(x.id)+"</td><td>"+x.count+"</td><td>"+x.period+"</td><td>"+x.min+"/"+x.max+"</td><td>"+x.dlc+"</td><td>"+x.data+"</td></tr>").join("");
}catch(e){console.error(e)}}setInterval(poll,500);poll();
</script></body></html>
)HTML";

void handleRoot() { server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML); }

void handleWiFiSave() {
  String ssid = server.arg("ssid"); String pass = server.arg("pass"); ssid.trim();
  if (!ssid.length()) { server.send(400, "text/plain", "SSID required"); return; }
  prefs.begin("exeed-can", false); prefs.putString("ssid", ssid); prefs.putString("pass", pass); prefs.end();
  server.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><p>Wi-Fi сохранён. ESP32 перезапускается...</p>");
  delay(700); ESP.restart();
}

void handleApiData() {
  updateCANStatus();
  uint32_t since = server.hasArg("since") ? strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
  String out; out.reserve(24000);
  uint32_t uniqueIds = 0; for (const auto &s : idStats) if (s.used) uniqueIds++;
  out = "{\"canState\":\"" + canState + "\",\"frames\":" + String(totalFrames) +
    ",\"uniqueIds\":" + String(uniqueIds) +
    ",\"sta\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") +
    ",\"cloud\":" + String(cloudReady ? "true" : "false") +
    ",\"rxErrors\":" + String(rxErrors) + ",\"txErrors\":" + String(txErrors) +
    ",\"busErrors\":" + String(busErrors) + ",\"rxMissed\":" + String(rxMissed) +
    ",\"rxOverrun\":" + String(rxOverrun) + ",\"txFailed\":" + String(txFailed) +
    ",\"cloudSentFrames\":" + String(cloudSentFrames) +
    ",\"cloudSentBatches\":" + String(cloudSentBatches) + ",\"framesList\":[";
  bool first = true;
  uint32_t oldest = totalFrames > FRAME_BUFFER_SIZE ? totalFrames - FRAME_BUFFER_SIZE + 1 : 1;
  for (uint32_t seq = oldest; seq <= totalFrames; seq++) {
    if (seq <= since) continue;
    const CanFrame &f = frameBuffer[(seq - 1) % FRAME_BUFFER_SIZE];
    if (f.seq != seq) continue;
    if (!first) out += ","; first = false;
    out += "{\"seq\":" + String(f.seq) + ",\"ts\":" + String(f.ts) +
      ",\"id\":" + String(f.id) + ",\"dlc\":" + String(f.dlc) +
      ",\"type\":\"" + String(f.extended ? "EXT" : "STD") + "\",\"data\":\"";
    for (uint8_t i = 0; i < f.dlc; i++) { if (i) out += " "; if (f.data[i] < 16) out += "0"; out += String(f.data[i], HEX); }
    out += "\"}";
  }
  out += "],\"inventory\":["; first = true;
  for (const auto &s : idStats) {
    if (!s.used) continue;
    if (!first) out += ","; first = false;
    uint32_t minP = s.minPeriod == UINT32_MAX ? 0 : s.minPeriod;
    out += "{\"id\":" + String(s.id) + ",\"count\":" + String(s.count) +
      ",\"period\":" + String(s.lastPeriod) + ",\"min\":" + String(minP) +
      ",\"max\":" + String(s.maxPeriod) + ",\"dlc\":" + String(s.dlc) + ",\"data\":\"";
    for (uint8_t i = 0; i < s.dlc; i++) { if (i) out += " "; if (s.lastData[i] < 16) out += "0"; out += String(s.lastData[i], HEX); }
    out += "\"}";
  }
  out += "]}";
  server.send(200, "application/json", out);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi", HTTP_POST, handleWiFiSave);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/favicon.ico", HTTP_GET, [](){ server.send(204); });
  server.begin();
  Serial.println("[WEB] Dashboard: http://192.168.4.1");
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: cloudReady = true; probeSent = false; Serial.println("[WSS] Connected to Cloudflare /device"); break;
    case WStype_DISCONNECTED: cloudReady = false; probeSent = false; Serial.println("[WSS] Disconnected; reconnect enabled"); break;
    case WStype_ERROR: cloudReady = false; Serial.println("[WSS] Socket error"); break;
    case WStype_TEXT: Serial.printf("[WSS] Server: %.*s\n", (int)length, (const char*)payload); break;
    default: break;
  }
}

void startCloudIfReady() {
  if (cloudStarted || WiFi.status() != WL_CONNECTED || !clockReady) return;
  Serial.println("[CLOUD] Starting TLS WebSocket...");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);
  webSocket.beginSslWithCA(CLOUD_HOST, CLOUD_PORT, CLOUD_PATH, GTS_ROOT_R4);
  cloudStarted = true;
}

void sendCloudProbe() {
  if (!cloudReady || !webSocket.isConnected()) return;
  if (probeSent && millis() - lastProbeAt < 15000) return;
  if (webSocket.sendTXT("{\"type\":\"frames\",\"frames\":[]}")) {
    probeSent = true; lastProbeAt = millis();
    Serial.println("[CLOUD] Link heartbeat sent; CAN frame count in probe=0");
  }
}

void sendCloudFrames() {
  if (!cloudReady || !webSocket.isConnected() || !totalFrames) return;
  if (millis() - lastCloudSendAt < CLOUD_SEND_INTERVAL_MS) return;
  lastCloudSendAt = millis();
  uint32_t oldest = totalFrames > FRAME_BUFFER_SIZE ? totalFrames - FRAME_BUFFER_SIZE + 1 : 1;
  uint32_t nextSeq = max(cloudLastSentSeq + 1, oldest);
  if (nextSeq > totalFrames) return;

  String packet; packet.reserve(5000); packet = "{\"type\":\"frames\",\"frames\":[";
  bool first = true; uint32_t sentThrough = cloudLastSentSeq; uint16_t count = 0;
  for (uint32_t seq = nextSeq; seq <= totalFrames && count < 25; seq++) {
    const CanFrame &f = frameBuffer[(seq - 1) % FRAME_BUFFER_SIZE];
    if (f.seq != seq) continue;
    if (!first) packet += ","; first = false;
    packet += "{\"seq\":" + String(f.seq) + ",\"ts\":" + String(f.ts) +
      ",\"id\":" + String(f.id) + ",\"dlc\":" + String(f.dlc) +
      ",\"ext\":" + String(f.extended ? "true" : "false") +
      ",\"rtr\":" + String(f.remote ? "true" : "false") + ",\"data\":[";
    for (uint8_t i = 0; i < f.dlc; i++) { if (i) packet += ","; packet += String(f.data[i]); }
    packet += "]}"; sentThrough = seq; count++;
  }
  packet += "]}";
  if (count && webSocket.sendTXT(packet)) {
    cloudLastSentSeq = sentThrough; cloudSentFrames += count; cloudSentBatches++;
    Serial.printf("[CLOUD] Sent %u frames; total=%lu\n", count, (unsigned long)cloudSentFrames);
  } else if (count) Serial.println("[CLOUD ERROR] sendTXT failed");
}

void printStatus() {
  updateCANStatus();
  uint32_t ids = 0; for (const auto &s : idStats) if (s.used) ids++;
  Serial.println("\n========== EXEED CAN ROUTER v2.0 ==========");
  Serial.printf("CAN=%s frames=%lu uniqueIDs=%lu\n", canState.c_str(), (unsigned long)totalFrames, (unsigned long)ids);
  Serial.printf("TWAI RX=%lu TX=%lu BUS=%lu MISSED=%lu OVERRUN=%lu TX_FAILED=%lu\n",
    (unsigned long)rxErrors, (unsigned long)txErrors, (unsigned long)busErrors,
    (unsigned long)rxMissed, (unsigned long)rxOverrun, (unsigned long)txFailed);
  Serial.printf("Wi-Fi=%s IP=%s Cloud=%s sentFrames=%lu\n",
    WiFi.status() == WL_CONNECTED ? "CONNECTED" : "OFFLINE",
    WiFi.localIP().toString().c_str(), cloudReady ? "CONNECTED" : "OFFLINE",
    (unsigned long)cloudSentFrames);
  Serial.println("MODE=LISTEN ONLY; CAN TRANSMIT=DISABLED");
  Serial.println("============================================");
}

void setup() {
  Serial.begin(115200); delay(1500);
  Serial.println("\nEXEED CAN ROUTER v2.0 / ESP32-S3");
  Serial.println("CAN 500 kbit/s / TWAI LISTEN ONLY / Cloudflare WSS");
  canReady = initializeCAN();
  startWiFi();
  startWebServer();
}

void loop() {
  server.handleClient();
  receiveCAN();
  if (WiFi.status() == WL_CONNECTED) {
    updateClock();
    startCloudIfReady();
  } else {
    clockReady = false;
    ntpStarted = false;
  }
  if (cloudStarted) {
    webSocket.loop();
    sendCloudProbe();
    sendCloudFrames();
  }
  if (millis() - lastStatusAt >= STATUS_INTERVAL_MS) {
    lastStatusAt = millis();
    printStatus();
  }
  delay(1);
}
