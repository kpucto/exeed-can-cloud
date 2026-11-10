
const VIEWER_HTML = `<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#080d17">
<title>EXEED CAN CLOUD</title>
<style>
:root {
  color-scheme:dark;
  --bg:#080d17;
  --panel:#111a29;
  --panel2:#0d1522;
  --line:#202d40;
  --text:#e9f0fa;
  --muted:#8190a6;
  --green:#36e0a0;
  --red:#ff6578;
  --blue:#65aaff;
  --orange:#ffbd69;
}
*{box-sizing:border-box}
body{
  margin:0;background:var(--bg);color:var(--text);
  font-family:Inter,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;
  min-height:100vh;
}
.container{max-width:1450px;margin:auto;padding:25px}
header{
  display:flex;justify-content:space-between;align-items:center;
  gap:15px;margin-bottom:25px;flex-wrap:wrap;
}
.brand{display:flex;align-items:center;gap:14px}
.logo{
  width:48px;height:48px;border-radius:14px;
  background:linear-gradient(145deg,#65aaff,#2867bd);
  display:grid;place-items:center;font-weight:900;font-size:20px;
  box-shadow:0 5px 25px #2867bd55;
}
h1{font-size:21px;margin:0;letter-spacing:1.5px}
.subtitle{font-size:12px;color:var(--muted);margin-top:5px}
.status{
  display:flex;align-items:center;gap:9px;
  background:var(--panel);border:1px solid var(--line);
  padding:10px 15px;border-radius:12px;font-size:13px;
}
.dot{width:9px;height:9px;border-radius:50%;background:var(--red)}
.dot.online{background:var(--green);box-shadow:0 0 12px var(--green)}
.test-banner{
  border:1px solid #765a2e;background:#2b2113;
  color:var(--orange);padding:12px 15px;border-radius:12px;
  font-size:12px;margin-bottom:20px;
}
.cards{
  display:grid;grid-template-columns:repeat(4,minmax(0,1fr));
  gap:14px;margin-bottom:20px;
}
.card{
  background:linear-gradient(145deg,var(--panel),var(--panel2));
  border:1px solid var(--line);border-radius:16px;padding:19px;
  min-width:0;
}
.card-label{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:1px}
.card-value{font-size:27px;font-weight:750;margin-top:12px;overflow-wrap:anywhere}
.card-note{font-size:11px;color:var(--muted);margin-top:7px}
.green{color:var(--green)}
.blue{color:var(--blue)}
.orange{color:var(--orange)}
.red{color:var(--red)}
.layout{
  display:grid;grid-template-columns:minmax(0,1fr) 290px;
  gap:16px;align-items:start;
}
.panel{
  background:var(--panel);border:1px solid var(--line);
  border-radius:16px;overflow:hidden;margin-bottom:16px;
}
.panel-head{
  padding:17px 19px;border-bottom:1px solid var(--line);
  display:flex;justify-content:space-between;align-items:center;
  gap:12px;flex-wrap:wrap;
}
.panel-title{font-size:14px;font-weight:700}
.panel-sub{font-size:11px;color:var(--muted);margin-top:4px}
.tools{display:flex;gap:8px;align-items:center}
input{
  background:#080f1b;border:1px solid var(--line);
  border-radius:9px;color:var(--text);padding:10px 12px;
  outline:none;width:160px;min-width:0;
}
input:focus{border-color:var(--blue)}
button{
  border:1px solid var(--line);background:#18263a;color:var(--text);
  padding:10px 13px;border-radius:9px;cursor:pointer;font-size:12px;
}
button:hover{border-color:var(--blue)}
.table-wrap{overflow-x:auto}
table{width:100%;border-collapse:collapse;white-space:nowrap;font-size:12px}
th{
  color:var(--muted);font-weight:600;text-align:left;
  font-size:10px;text-transform:uppercase;letter-spacing:.8px;
  padding:13px 17px;background:#0d1522;
}
td{padding:13px 17px;border-top:1px solid #1b293b}
tbody tr:hover{background:#18263a}
.can-id{color:var(--blue);font-weight:750;font-family:monospace}
.data{font-family:monospace;letter-spacing:1px;color:#d4e2f4}
.empty{padding:45px 15px;text-align:center;color:var(--muted);font-size:13px}
.side-box{padding:17px 18px}
.info-row{
  display:flex;justify-content:space-between;gap:12px;
  padding:12px 0;border-bottom:1px solid var(--line);font-size:12px;
}
.info-row:last-child{border:0}
.info-label{color:var(--muted)}
.info-value{text-align:right;overflow-wrap:anywhere}
.badge{
  display:inline-block;padding:5px 9px;border-radius:7px;
  background:#12372c;color:var(--green);font-size:10px;font-weight:800;
  letter-spacing:.7px;
}
.footer{text-align:center;color:#58677d;font-size:11px;padding:20px}
@media(max-width:900px){
  .cards{grid-template-columns:repeat(2,minmax(0,1fr))}
  .layout{grid-template-columns:1fr}
}
@media(max-width:520px){
  .container{padding:15px}
  header{margin-bottom:17px}
  h1{font-size:17px}
  .logo{width:42px;height:42px}
  .cards{gap:9px}
  .card{padding:14px}
  .card-value{font-size:23px}
  .panel-head{padding:14px}
  input{width:125px}
  th,td{padding:12px 13px}
}
</style>
</head>
<body>
<div class="container">
<header>
  <div class="brand">
    <div class="logo">EX</div>
    <div>
      <h1>EXEED CAN CLOUD</h1>
      <div class="subtitle">REMOTE VEHICLE DIAGNOSTICS</div>
    </div>
  </div>
  <div class="status">
    <span class="dot" id="wsDot"></span>
    <span id="wsStatus">Подключение...</span>
  </div>
</header>

<div class="test-banner">
  ⚠ ТЕСТОВЫЙ РЕЖИМ · АВТОРИЗАЦИЯ ОТКЛЮЧЕНА · CAN READ ONLY
</div>

<section class="cards">
  <div class="card">
    <div class="card-label">ESP32 устройство</div>
    <div class="card-value red" id="deviceStatus">OFFLINE</div>
    <div class="card-note" id="deviceNote">Ожидание устройства</div>
  </div>
  <div class="card">
    <div class="card-label">Получено CAN кадров</div>
    <div class="card-value blue" id="frameCount">0</div>
    <div class="card-note">За время работы сервера</div>
  </div>
  <div class="card">
    <div class="card-label">Последний кадр</div>
    <div class="card-value" id="lastFrame">--:--:--</div>
    <div class="card-note" id="lastFrameDate">Нет данных</div>
  </div>
  <div class="card">
    <div class="card-label">Режим CAN</div>
    <div class="card-value green">READ ONLY</div>
    <div class="card-note">Передача команд запрещена</div>
  </div>
</section>

<div class="layout">
  <main>
    <section class="panel">
      <div class="panel-head">
        <div>
          <div class="panel-title">CAN MONITOR</div>
          <div class="panel-sub">Поток CAN-кадров в реальном времени</div>
        </div>
        <div class="tools">
          <input id="search" placeholder="Поиск CAN ID...">
          <button id="clearBtn">Очистить</button>
        </div>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Время</th>
              <th>CAN ID</th>
              <th>DLC</th>
              <th>DATA</th>
            </tr>
          </thead>
          <tbody id="frames">
            <tr><td colspan="4" class="empty">Ожидание CAN-кадров...</td></tr>
          </tbody>
        </table>
      </div>
    </section>
  </main>

  <aside>
    <section class="panel">
      <div class="panel-head">
        <div>
          <div class="panel-title">СОСТОЯНИЕ СИСТЕМЫ</div>
          <div class="panel-sub">Cloud connection</div>
        </div>
        <span class="badge">ONLINE</span>
      </div>
      <div class="side-box">
        <div class="info-row">
          <span class="info-label">Cloud сервер</span>
          <span class="info-value green" id="cloudStatus">ONLINE</span>
        </div>
        <div class="info-row">
          <span class="info-label">Устройства</span>
          <span class="info-value" id="devices">0</span>
        </div>
        <div class="info-row">
          <span class="info-label">Просмотрщики</span>
          <span class="info-value" id="viewers">0</span>
        </div>
        <div class="info-row">
          <span class="info-label">Последняя связь</span>
          <span class="info-value" id="lastSeen">Нет данных</span>
        </div>
        <div class="info-row">
          <span class="info-label">Последняя ошибка</span>
          <span class="info-value" id="lastError">Нет</span>
        </div>
      </div>
    </section>

    <section class="panel">
      <div class="panel-head">
        <div>
          <div class="panel-title">ПОДКЛЮЧЕНИЕ</div>
          <div class="panel-sub">WebSocket viewer</div>
        </div>
      </div>
      <div class="side-box">
        <div class="info-row">
          <span class="info-label">Канал</span>
          <span class="info-value">/viewer</span>
        </div>
        <div class="info-row">
          <span class="info-label">Обновление</span>
          <span class="info-value">LIVE</span>
        </div>
        <div class="info-row">
          <span class="info-label">Буфер кадров</span>
          <span class="info-value">300</span>
        </div>
        <button id="reconnectBtn" style="width:100%;margin-top:12px">
          ↻ Переподключить
        </button>
      </div>
    </section>
  </aside>
</div>

<div class="footer">
  EXEED CAN CLOUD · READ ONLY · LIVE TELEMETRY
</div>
</div>

<script>
const $ = id => document.getElementById(id);
let socket = null;
let frames = [];
let totalReceived = 0;
let reconnectTimer = null;
let manuallyClosed = false;

function setConnection(online, text) {
  $("wsDot").className = online ? "dot online" : "dot";
  $("wsStatus").textContent = text;
}

function formatTime(value) {
  if (!value) return "--:--:--";
  const d = new Date(value);
  return d.toLocaleTimeString("ru-RU");
}

function formatDate(value) {
  if (!value) return "Нет данных";
  return new Date(value).toLocaleString("ru-RU");
}

function formatId(id) {
  if (typeof id === "number") return "0x" + id.toString(16).toUpperCase().padStart(3,"0");
  if (typeof id === "string") {
    if (id.startsWith("0x")) return id.toUpperCase();
    const n = Number(id);
    if (Number.isFinite(n)) return "0x" + n.toString(16).toUpperCase().padStart(3,"0");
  }
  return String(id ?? "UNKNOWN");
}

function renderFrames() {
  const query = $("search").value.trim().toLowerCase();
  const filtered = frames.filter(f => {
    return formatId(f.id).toLowerCase().includes(query);
  });
  const body = $("frames");

  if (!filtered.length) {
    body.innerHTML = '<tr><td colspan="4" class="empty">Нет CAN-кадров для отображения</td></tr>';
    return;
  }

  body.innerHTML = filtered.map(f => {
    const data = Array.isArray(f.data)
      ? f.data.map(x => Number(x).toString(16).toUpperCase().padStart(2,"0")).join(" ")
      : String(f.data ?? "");
    const dlc = Number.isFinite(Number(f.dlc)) ? f.dlc : (f.data?.length ?? 0);
    return '<tr>' +
      '<td>' + formatTime(f.receivedAt || f.ts) + '</td>' +
      '<td class="can-id">' + formatId(f.id) + '</td>' +
      '<td>' + dlc + '</td>' +
      '<td class="data">' + data + '</td>' +
      '</tr>';
  }).join("");
}

function addFrames(list) {
  if (!Array.isArray(list)) return;
  const now = Date.now();

  for (const frame of list) {
    if (!frame || typeof frame !== "object") continue;
    frames.unshift({...frame, receivedAt:now});
    totalReceived++;
  }

  if (frames.length > 300) frames.length = 300;
  $("frameCount").textContent = totalReceived.toLocaleString("ru-RU");

  if (list.length) {
    $("lastFrame").textContent = formatTime(now);
    $("lastFrameDate").textContent = formatDate(now);
  }
  renderFrames();
}

function connect() {
  manuallyClosed = false;
  clearTimeout(reconnectTimer);

  if (socket) {
    try { socket.close(); } catch {}
  }

  const protocol = location.protocol === "https:" ? "wss:" : "ws:";
  const wsUrl = protocol + "//" + location.host + "/viewer";

  setConnection(false, "Подключение...");
  socket = new WebSocket(wsUrl);

  socket.onopen = () => {
    setConnection(true, "WebSocket подключен");
  };

  socket.onmessage = event => {
    try {
      const packet = JSON.parse(event.data);
      if (packet.type === "frames") addFrames(packet.frames);
    } catch (e) {
      console.error("Invalid packet", e);
    }
  };

  socket.onerror = () => {
    setConnection(false, "Ошибка соединения");
  };

  socket.onclose = () => {
    setConnection(false, "Отключено");
    if (!manuallyClosed) {
      reconnectTimer = setTimeout(connect, 3000);
    }
  };
}

async function updateStatus() {
  try {
    const response = await fetch("/status", {cache:"no-store"});
    if (!response.ok) throw new Error("HTTP " + response.status);
    const s = await response.json();

    $("cloudStatus").textContent = s.status || "UNKNOWN";
    $("devices").textContent = s.devices ?? 0;
    $("viewers").textContent = s.viewers ?? 0;
    $("frameCount").textContent = (s.totalFrames ?? 0).toLocaleString("ru-RU");

    const online = Number(s.devices) > 0;
    $("deviceStatus").textContent = online ? "ONLINE" : "OFFLINE";
    $("deviceStatus").className = "card-value " + (online ? "green" : "red");
    $("deviceNote").textContent = online ? "ESP32 подключена" : "Ожидание устройства";

    $("lastSeen").textContent = s.lastSeen ? formatDate(s.lastSeen) : "Нет данных";

    if (s.lastSeen) {
      $("lastFrame").textContent = formatTime(s.lastSeen);
      $("lastFrameDate").textContent = formatDate(s.lastSeen);
    }

    $("lastError").textContent = s.lastError
      ? (s.lastError.message || s.lastError.type || "Ошибка")
      : "Нет";
  } catch (e) {
    $("cloudStatus").textContent = "ERROR";
    console.error("Status error", e);
  }
}

$("search").addEventListener("input", renderFrames);
$("clearBtn").addEventListener("click", () => {
  frames = [];
  totalReceived = 0;
  $("frameCount").textContent = "0";
  renderFrames();
});
$("reconnectBtn").addEventListener("click", connect);

window.addEventListener("online", connect);

connect();
updateStatus();
setInterval(updateStatus, 3000);
</script>
</body>
</html>`;

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const path = url.pathname;
    const isWebSocket =
      request.headers.get("Upgrade")?.toLowerCase() === "websocket";

    // Главная страница и health check
    if (path === "/" || path === "/health") {
      return Response.json({
        status: "online",
        service: "EXEED CAN CLOUD",
        mode: "READ ONLY",
        authentication: "DISABLED FOR TEST"
      });
    }

    // Веб-панель для обычного браузера
    if (path === "/viewer" && !isWebSocket) {
      return new Response(VIEWER_HTML, {
        headers: {
          "Content-Type": "text/html; charset=utf-8",
          "Cache-Control": "no-store",
          "X-Content-Type-Options": "nosniff"
        }
      });
    }

    let role;

    if (path === "/device") {
      role = "device";
    } else if (path === "/viewer") {
      role = "viewer";
    } else if (path === "/status") {
      role = "status";
    } else {
      return new Response("Not found", { status: 404 });
    }

    if (role !== "status" && !isWebSocket) {
      return new Response("WebSocket required", { status: 426 });
    }

    if (!env.CAN_HUB) {
      return Response.json(
        { error: "CAN_HUB binding unavailable" },
        { status: 500 }
      );
    }

    const id = env.CAN_HUB.idFromName("exeed-main");
    const stub = env.CAN_HUB.get(id);

    const internalUrl = new URL(request.url);
    internalUrl.pathname = role === "status" ? "/status" : "/connect";
    internalUrl.searchParams.set("role", role);

    return stub.fetch(new Request(internalUrl, request));
  }
};

export class CanHub {
  constructor(state, env) {
    this.state = state;
    this.env = env;

    this.totalFrames = 0;
    this.lastSeen = null;
    this.lastConnected = null;
    this.lastClose = null;
    this.lastError = null;
    this.lastMessage = null;
  }

  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === "/status") {
      return Response.json({
        status: "online",
        mode: "READ ONLY",
        devices: this.state.getWebSockets("device").length,
        viewers: this.state.getWebSockets("viewer").length,
        totalFrames: this.totalFrames,
        lastSeen: this.lastSeen,
        lastConnected: this.lastConnected,
        lastClose: this.lastClose,
        lastError: this.lastError,
        lastMessage: this.lastMessage
      }, {
        headers: {
          "Cache-Control": "no-store"
        }
      });
    }

    if (url.pathname !== "/connect") {
      return new Response("Not found", { status: 404 });
    }

    if (
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", { status: 426 });
    }

    const role = url.searchParams.get("role");

    if (role !== "device" && role !== "viewer") {
      return new Response("Invalid role", { status: 400 });
    }

    const pair = new WebSocketPair();
    const client = pair[0];
    const server = pair[1];

    server.serializeAttachment({
      role,
      connectedAt: Date.now()
    });

    this.state.acceptWebSocket(server, [role]);

    this.lastConnected = {
      role,
      time: Date.now()
    };

    this.lastClose = null;
    this.lastError = null;

    console.log("[WS] CONNECTED:", role);

    return new Response(null, {
      status: 101,
      webSocket: client
    });
  }

  webSocketMessage(ws, message) {
    const attachment = ws.deserializeAttachment();

    // Viewer имеет только право чтения
    if (attachment?.role !== "device") {
      console.log("[WS] Unauthorized viewer message");
      ws.close(1008, "Read only connection");
      return;
    }

    if (typeof message !== "string") {
      this.lastError = {
        type: "NON_TEXT_MESSAGE",
        time: Date.now()
      };

      ws.close(1003, "Text JSON only");
      return;
    }

    let packet;

    try {
      packet = JSON.parse(message);
    } catch {
      this.lastError = {
        type: "INVALID_JSON",
        time: Date.now()
      };

      ws.close(1007, "Invalid JSON");
      return;
    }

    if (
      packet?.type !== "frames" ||
      !Array.isArray(packet.frames) ||
      packet.frames.length > 500
    ) {
      this.lastError = {
        type: "INVALID_CAN_PACKET",
        time: Date.now()
      };

      ws.close(1008, "Invalid CAN packet");
      return;
    }

    const validFrames = packet.frames.filter(frame =>
      frame &&
      typeof frame === "object" &&
      Number.isFinite(Number(frame.id)) &&
      Array.isArray(frame.data) &&
      frame.data.length <= 8
    );

    this.totalFrames += validFrames.length;
    this.lastSeen = Date.now();

    this.lastMessage = {
      type: packet.type,
      frames: validFrames.length,
      time: Date.now()
    };

    const outgoing = JSON.stringify({
      type: "frames",
      frames: validFrames
    });

    for (const viewer of this.state.getWebSockets("viewer")) {
      try {
        viewer.send(outgoing);
      } catch (error) {
        console.log("[WS] BROADCAST ERROR:", String(error));

        try {
          viewer.close(1011, "Broadcast error");
        } catch {}
      }
    }
  }

  webSocketClose(ws, code, reason, wasClean) {
    const attachment = ws.deserializeAttachment();

    this.lastClose = {
      role: attachment?.role || "unknown",
      code,
      reason: String(reason || ""),
      wasClean: Boolean(wasClean),
      time: Date.now()
    };

    console.log("[WS] CLOSED:", JSON.stringify(this.lastClose));
  }

  webSocketError(ws, error) {
    const attachment = ws.deserializeAttachment();

    this.lastError = {
      role: attachment?.role || "unknown",
      message: String(error),
      time: Date.now()
    };

    console.log("[WS] ERROR:", JSON.stringify(this.lastError));
  }
}
