
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

const DECODER_HTML = "<!DOCTYPE html>\n<html lang=\"ru\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><meta name=\"theme-color\" content=\"#080d17\"><title>EXEED CAN Decoder Lab</title>\n<style>\n:root{color-scheme:dark;--bg:#080d17;--panel:#111a29;--panel2:#0d1522;--line:#202d40;--text:#e9f0fa;--muted:#8190a6;--green:#36e0a0;--red:#ff6578;--blue:#65aaff;--orange:#ffbd69}\n*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font-family:Inter,-apple-system,BlinkMacSystemFont,\"Segoe UI\",sans-serif;min-height:100vh}.wrap{max-width:1600px;margin:auto;padding:22px}.top{display:flex;align-items:center;justify-content:space-between;gap:14px;flex-wrap:wrap;margin-bottom:18px}.brand{display:flex;align-items:center;gap:13px}.logo{width:45px;height:45px;border-radius:13px;background:linear-gradient(145deg,#65aaff,#2867bd);display:grid;place-items:center;font-weight:900}.sub{color:var(--muted);font-size:12px;margin-top:4px}h1{font-size:20px;margin:0;letter-spacing:1px}.nav{display:flex;gap:8px;flex-wrap:wrap}.nav a,button,.filebtn{border:1px solid var(--line);background:#18263a;color:var(--text);padding:9px 12px;border-radius:9px;text-decoration:none;font-size:12px;cursor:pointer}.nav a.active{border-color:var(--blue);color:var(--blue)}button:hover,.filebtn:hover{border-color:var(--blue)}.banner{border:1px solid #765a2e;background:#2b2113;color:var(--orange);padding:10px 13px;border-radius:10px;font-size:12px;margin-bottom:16px}.stats{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));gap:12px;margin-bottom:16px}.card,.panel{background:linear-gradient(145deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:14px;min-width:0}.card{padding:15px}.label{font-size:10px;color:var(--muted);letter-spacing:1px;text-transform:uppercase}.value{font-size:25px;font-weight:750;margin-top:9px}.hint{font-size:11px;color:var(--muted);margin-top:5px}.layout{display:grid;grid-template-columns:minmax(0,1fr) 330px;gap:14px;align-items:start}.panel{overflow:hidden;margin-bottom:14px}.ph{padding:14px 16px;border-bottom:1px solid var(--line);display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap}.pt{font-size:13px;font-weight:700}.tools{display:flex;gap:7px;flex-wrap:wrap;align-items:center}.body{padding:14px 16px}input,select{background:#080f1b;border:1px solid var(--line);border-radius:8px;color:var(--text);padding:9px 10px;min-width:0}input[type=file]{display:none}input[type=search]{width:180px}table{width:100%;border-collapse:collapse;font-size:12px;white-space:nowrap}th{text-align:left;color:var(--muted);font-size:10px;text-transform:uppercase;letter-spacing:.7px;background:#0d1522;padding:11px}td{padding:10px 11px;border-top:1px solid #1b293b}tbody tr:hover{background:#18263a}.scroll{overflow:auto;max-height:460px}.id{font-family:monospace;color:var(--blue);font-weight:700}.hex{font-family:monospace;letter-spacing:1px}.muted{color:var(--muted)}.pill{display:inline-block;padding:4px 7px;border-radius:6px;background:#12372c;color:var(--green);font-size:10px}.empty{text-align:center;padding:30px;color:var(--muted)}.drop{border:1px dashed #3a5270;border-radius:12px;padding:17px;text-align:center;color:var(--muted);font-size:12px}.drop.drag{border-color:var(--blue);background:#14253a}.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}.bits{display:grid;grid-template-columns:repeat(8,1fr);gap:4px;margin-top:10px}.bit{background:#17263a;border:1px solid var(--line);border-radius:5px;text-align:center;padding:7px 2px;font:11px monospace}.bit.one{background:#174c3b;color:#75f0bf;border-color:#267b5d}.bytebar{display:grid;grid-template-columns:repeat(8,minmax(0,1fr));gap:6px}.byte{background:#0b1421;border:1px solid var(--line);border-radius:8px;padding:8px;text-align:center;font-family:monospace}.byte strong{display:block;color:var(--blue);font-size:14px}.byte small{color:var(--muted)}canvas{width:100%;height:180px;background:#0a1220;border:1px solid var(--line);border-radius:9px}.status{font-size:12px;color:var(--muted)}.good{color:var(--green)}.warn{color:var(--orange)}.danger{color:var(--red)}.row{display:flex;justify-content:space-between;gap:10px;padding:9px 0;border-bottom:1px solid var(--line);font-size:12px}.row:last-child{border:0}.foot{text-align:center;color:#58677d;font-size:10px;padding:12px}.hide{display:none!important}\n@media(max-width:1050px){.layout{grid-template-columns:1fr}.stats{grid-template-columns:repeat(3,1fr)}}@media(max-width:600px){.wrap{padding:12px}.stats{grid-template-columns:repeat(2,1fr)}.grid2{grid-template-columns:1fr}h1{font-size:16px}.value{font-size:22px}input[type=search]{width:135px}}\n</style></head><body><div class=\"wrap\">\n<header class=\"top\"><div class=\"brand\"><div class=\"logo\">EX</div><div><h1>CAN DECODER LAB</h1><div class=\"sub\">EXEED TXL · CAN ANALYSIS WORKBENCH</div></div></div><nav class=\"nav\"><a href=\"/viewer\">LIVE MONITOR</a><a class=\"active\" href=\"/decoder\">DECODER LAB</a></nav></header>\n<div class=\"banner\">⚠ READ ONLY · Анализ логов и приём CAN кадров. Передача команд в автомобиль отсутствует.</div>\n<section class=\"stats\">\n<div class=\"card\"><div class=\"label\">Кадров в наборе</div><div class=\"value\" id=\"sFrames\">0</div><div class=\"hint\">Текущий набор данных</div></div>\n<div class=\"card\"><div class=\"label\">Уникальных CAN ID</div><div class=\"value\" id=\"sIds\">0</div><div class=\"hint\">Идентификаторы</div></div>\n<div class=\"card\"><div class=\"label\">Средняя частота</div><div class=\"value\" id=\"sRate\">0 Hz</div><div class=\"hint\">По временным меткам</div></div>\n<div class=\"card\"><div class=\"label\">Последний кадр</div><div class=\"value\" id=\"sLast\">—</div><div class=\"hint\">Время / timestamp</div></div>\n<div class=\"card\"><div class=\"label\">Источник</div><div class=\"value\" id=\"sSource\" style=\"font-size:17px\">Нет данных</div><div class=\"hint\" id=\"sMode\">Загрузите лог или включите Live</div></div>\n</section>\n<div class=\"layout\"><main>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">ИМПОРТ ЛОГА</div><div class=\"sub\">CSV · ASC · TRC · candump · текстовые дампы</div></div><div class=\"tools\"><label class=\"filebtn\" for=\"fileInput\">＋ Загрузить файл</label><input id=\"fileInput\" type=\"file\" accept=\".csv,.asc,.trc,.log,.txt\"><button id=\"exportBtn\">Экспорт CSV</button><button id=\"clearBtn\">Очистить</button></div></div><div class=\"body\"><div class=\"drop\" id=\"drop\">Перетащите сюда CAN-лог или нажмите «Загрузить файл».<br><span class=\"muted\">Исходный файл обрабатывается в браузере и не отправляется на сервер.</span></div><div class=\"tools\" style=\"margin-top:12px\"><input id=\"filter\" type=\"search\" placeholder=\"Фильтр CAN ID\"><select id=\"sort\"><option value=\"count\">Сортировать: частота</option><option value=\"id\">Сортировать: CAN ID</option><option value=\"rate\">Сортировать: Hz</option></select><button id=\"pauseBtn\">Ⅱ Пауза</button><button id=\"liveBtn\">● Live: выкл.</button></div></div></section>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">CAN ID INVENTORY</div><div class=\"sub\">Количество, периодичность, DLC и последнее значение</div></div><span class=\"status\" id=\"inventoryCount\">0 IDs</span></div><div class=\"scroll\"><table><thead><tr><th>CAN ID</th><th>Кадры</th><th>Частота</th><th>Период</th><th>DLC</th><th>Последние DATA</th></tr></thead><tbody id=\"inventory\"><tr><td colspan=\"6\" class=\"empty\">Загрузите лог или подключите Live</td></tr></tbody></table></div></section>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">FRAME INSPECTOR</div><div class=\"sub\">Выберите ID в таблице, чтобы изучить байты и биты</div></div><span class=\"pill\" id=\"selectedId\">ID НЕ ВЫБРАН</span></div><div class=\"body\"><div class=\"grid2\"><div><div class=\"label\">Последний кадр</div><div id=\"frameMeta\" class=\"sub\">Нет выбранного кадра</div><div class=\"bytebar\" id=\"byteView\" style=\"margin-top:12px\"><div class=\"empty\" style=\"grid-column:1/-1\">Нет данных</div></div></div><div><div class=\"label\">Биты последнего кадра</div><div id=\"bitView\" class=\"bits\"></div><div class=\"sub\" style=\"margin-top:8px\">Биты отображаются от 7 к 0 в каждом байте. Это сырые данные, не расшифровка сигнала.</div></div></div></div></section>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">SIGNAL EXPLORER</div><div class=\"sub\">Распределение значений байтов выбранного CAN ID</div></div><span class=\"status\" id=\"signalHint\">Выберите CAN ID</span></div><div class=\"body\"><canvas id=\"plot\" width=\"1000\" height=\"180\"></canvas><div class=\"sub\" style=\"margin-top:9px\">График показывает сырые значения байтов по последовательным кадрам. Масштабирование, знаковость и физические единицы пока не назначаются.</div></div></section>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">СРАВНЕНИЕ ЛОГОВ</div><div class=\"sub\">Сравните два снимка до и после действия в автомобиле</div></div><div class=\"tools\"><button id=\"snapA\">Снимок A</button><button id=\"snapB\">Снимок B</button><button id=\"compareBtn\">Сравнить A / B</button></div></div><div class=\"body\"><div class=\"grid2\"><div class=\"card\"><div class=\"label\">Snapshot A</div><div class=\"value\" id=\"snapACount\">—</div><div class=\"hint\" id=\"snapAInfo\">Не сохранён</div></div><div class=\"card\"><div class=\"label\">Snapshot B</div><div class=\"value\" id=\"snapBCount\">—</div><div class=\"hint\" id=\"snapBInfo\">Не сохранён</div></div></div><div id=\"compareResult\" class=\"sub\" style=\"margin-top:12px\">Снимите A и B, затем запустите сравнение.</div></div></section>\n</main><aside>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">LIVE CONNECTION</div><div class=\"sub\">Существующий канал /viewer</div></div><span class=\"pill\" id=\"liveStatus\">OFFLINE</span></div><div class=\"body\"><div class=\"row\"><span class=\"muted\">WebSocket</span><b id=\"wsState\">Отключён</b></div><div class=\"row\"><span class=\"muted\">Поступило live</span><b id=\"liveFrames\">0</b></div><div class=\"row\"><span class=\"muted\">Режим</span><b class=\"good\">READ ONLY</b></div><div class=\"sub\" style=\"margin-top:10px\">Live использует тот же viewer endpoint. Второе соединение с ESP32 не создаётся.</div></div></section>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">РАСПРЕДЕЛЕНИЕ ID</div><div class=\"sub\">Самые активные CAN ID</div></div></div><div class=\"body\" id=\"topIds\"><div class=\"empty\">Нет данных</div></div></section>\n<section class=\"panel\"><div class=\"ph\"><div><div class=\"pt\">АНАЛИЗАТОР</div><div class=\"sub\">Текущий набор</div></div></div><div class=\"body\"><div class=\"row\"><span class=\"muted\">Первый timestamp</span><b id=\"firstTs\">—</b></div><div class=\"row\"><span class=\"muted\">Последний timestamp</span><b id=\"lastTs\">—</b></div><div class=\"row\"><span class=\"muted\">Диапазон DLC</span><b id=\"dlcRange\">—</b></div><div class=\"row\"><span class=\"muted\">Формат</span><b id=\"formatName\">—</b></div></div></section>\n</aside></div><div class=\"foot\">EXEED CAN DECODER LAB · NO CAN TRANSMIT · UNKNOWN SIGNALS ARE NOT GUESSED</div></div>\n<script>\nconst $=id=>document.getElementById(id);let frames=[],selected=null,paused=false,liveOn=false,socket=null,liveCount=0,snapshots={A:null,B:null},sourceName=\"Нет данных\",formatName=\"—\";\nconst normId=v=>{if(typeof v===\"string\"&&v.toLowerCase().startsWith(\"0x\"))return parseInt(v,16);return Number(v)};\nconst hex=n=>Number(n).toString(16).toUpperCase().padStart(2,\"0\");\nfunction addFrame(f){if(!f||!Number.isFinite(Number(f.id))||!Array.isArray(f.data))return;const d=f.data.slice(0,8).map(x=>typeof x===\"string\"&&/^[0-9a-f]{2}$/i.test(x)?parseInt(x,16):Number(x)).filter(x=>Number.isFinite(x)).map(x=>x&255);frames.push({id:normId(f.id),data:d,dlc:Number.isFinite(Number(f.dlc))?Number(f.dlc):d.length,ts:Number.isFinite(Number(f.ts))?Number(f.ts):Date.now()/1000});if(frames.length>100000)frames.shift()}\nfunction parseLine(line){let m;const s=line.trim();if(!s)return null;\nm=s.match(/^\\(?(\\d+\\.\\d+)\\)?\\s+(?:\\w+\\s+)?([0-9A-Fa-f]{3,8})#([0-9A-Fa-f]*)/);if(m)return{id:parseInt(m[2],16),data:(m[3].match(/.{1,2}/g)||[]).map(x=>parseInt(x,16)),ts:Number(m[1])};\nm=s.match(/^\\s*(\\d+\\.\\d+)\\s+\\d+\\s+([0-9A-Fa-f]{3,8})\\s+(?:Rx|Tx|D)\\s+(\\d)\\s+((?:[0-9A-Fa-f]{2}\\s*)*)/i);if(m)return{id:parseInt(m[2],16),dlc:Number(m[3]),data:(m[4].trim().match(/[0-9A-Fa-f]{2}/g)||[]).map(x=>parseInt(x,16)),ts:Number(m[1])};\nm=s.match(/^\\s*(\\d+(?:\\.\\d+)?)\\s*,\\s*(0x[0-9a-f]+|\\d+)\\s*,\\s*(?:\"?)([0-9a-f ]{2,24})(?:\"?)\\s*$/i);if(m)return{id:normId(m[2]),data:(m[3].trim().match(/[0-9a-f]{2}/gi)||[]).map(x=>parseInt(x,16)),ts:Number(m[1])};\nreturn null}\nfunction parseText(text){const lines=text.replace(/\\r/g,\"\").split(\"\\n\"),out=[];let header=null;for(const line of lines){if(!line.trim())continue;const p=parseLine(line);if(p){out.push(p);continue}if(line.includes(\",\")&&/id/i.test(line)&&/data|byte/i.test(line)){header=line.toLowerCase().split(\",\").map(x=>x.trim());continue}if(header){const a=line.split(\",\").map(x=>x.trim());const ix=header.findIndex(x=>x===\"id\"||x.includes(\"can_id\")),dx=header.findIndex(x=>x===\"data\"||x.includes(\"byte\")),tx=header.findIndex(x=>x.includes(\"time\")||x===\"ts\"),lc=header.findIndex(x=>x===\"dlc\");if(ix>=0&&dx>=0&&a[ix]){const dat=(a[dx].replace(/0x/gi,\"\").match(/[0-9a-f]{2}/gi)||[]).map(x=>parseInt(x,16));out.push({id:normId(a[ix]),data:dat,dlc:lc>=0?Number(a[lc]):dat.length,ts:tx>=0?Number(a[tx]):out.length})}}}return out}\nfunction stats(){const ids=new Map();for(const f of frames){let x=ids.get(f.id);if(!x){x={id:f.id,count:0,first:f.ts,last:f.ts,dlcs:new Set(),lastFrame:f,periods:[]};ids.set(f.id,x)}else{x.periods.push(f.ts-x.last);x.last=f.ts;x.lastFrame=f}x.count++;x.dlcs.add(f.dlc)}\nconst span=frames.length>1?Math.max(.001,frames[frames.length-1].ts-frames[0].ts):0;return{ids,rate:span?frames.length/span:0,span}}\nfunction fmtTs(t){if(!Number.isFinite(t))return\"—\";return new Date(t*1000).toLocaleTimeString(\"ru-RU\")}\nfunction render(){const st=stats();$(\"sFrames\").textContent=frames.length.toLocaleString(\"ru-RU\");$(\"sIds\").textContent=st.ids.size;$(\"sRate\").textContent=st.rate.toFixed(1)+\" Hz\";$(\"sLast\").textContent=frames.length?fmtTs(frames[frames.length-1].ts):\"—\";$(\"sSource\").textContent=sourceName;$(\"sMode\").textContent=liveOn?\"Live stream\":formatName;$(\"inventoryCount\").textContent=st.ids.size+\" IDs\";$(\"firstTs\").textContent=frames.length?fmtTs(frames[0].ts):\"—\";$(\"lastTs\").textContent=frames.length?fmtTs(frames[frames.length-1].ts):\"—\";const dlcs=[...new Set(frames.map(f=>f.dlc))].sort();$(\"dlcRange\").textContent=dlcs.length?dlcs.join(\", \"):\"—\";$(\"formatName\").textContent=formatName;\nconst q=$(\"filter\").value.trim().toLowerCase(),sort=$(\"sort\").value;let rows=[...st.ids.values()].filter(x=>(\"0x\"+x.id.toString(16)).includes(q)||String(x.id).includes(q));rows.sort((a,b)=>sort===\"id\"?a.id-b.id:sort===\"rate\"?(b.count/Math.max(.001,b.last-b.first))-(a.count/Math.max(.001,a.last-a.first)):b.count-a.count);\n$(\"inventory\").innerHTML=rows.length?rows.map(x=>{const p=x.periods.length?x.periods.reduce((a,b)=>a+b,0)/x.periods.length*1000:0,rate=x.count>1?x.count/Math.max(.001,x.last-x.first):0;return '<tr data-id=\"'+x.id+'\"><td class=\"id\">0x'+x.id.toString(16).toUpperCase().padStart(3,\"0\")+'</td><td>'+x.count+'</td><td>'+rate.toFixed(2)+' Hz</td><td>'+(p?p.toFixed(2)+\" ms\":\"—\")+'</td><td>'+[...x.dlcs].join(\"/\")+'</td><td class=\"hex\">'+x.lastFrame.data.map(hex).join(\" \")+'</td></tr>'}).join(\"\"):'<tr><td colspan=\"6\" class=\"empty\">Нет совпадений</td></tr>';\ndocument.querySelectorAll(\"#inventory tr[data-id]\").forEach(tr=>tr.addEventListener(\"click\",()=>selectId(Number(tr.dataset.id))));\n$(\"topIds\").innerHTML=[...st.ids.values()].sort((a,b)=>b.count-a.count).slice(0,8).map(x=>'<div class=\"row\"><span class=\"id\">0x'+x.id.toString(16).toUpperCase()+'</span><b>'+x.count+'</b></div>').join(\"\")||'<div class=\"empty\">Нет данных</div>';if(selected!==null)drawSelected()}\nfunction selectId(id){selected=id;const fs=frames.filter(f=>f.id===id);const f=fs[fs.length-1];if(!f)return;$(\"selectedId\").textContent=\"0x\"+id.toString(16).toUpperCase();$(\"frameMeta\").textContent=\"Время: \"+fmtTs(f.ts)+\" · DLC: \"+f.dlc+\" · кадров: \"+fs.length;$(\"byteView\").innerHTML=Array.from({length:8},(_,i)=>'<div class=\"byte\"><small>B'+i+'</small><strong>'+(f.data[i]===undefined?\"—\":hex(f.data[i]))+'</strong><small>'+(f.data[i]===undefined?\"\":f.data[i])+'</small></div>').join(\"\");$(\"bitView\").innerHTML=f.data.map((v,i)=>Array.from({length:8},(_,b)=>'<div class=\"bit '+((v>>(7-b))&1?'one':'')+'\" title=\"Byte '+i+', bit '+(7-b)+'\">'+((v>>(7-b))&1)+'</div>').join(\"\")).join(\"\");$(\"signalHint\").textContent=fs.length+\" frames · raw bytes\";drawPlot(fs)}\nfunction drawSelected(){if(selected!==null)selectId(selected)}\nfunction drawPlot(fs){const c=$(\"plot\"),ctx=c.getContext(\"2d\"),w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.strokeStyle=\"#26374d\";ctx.lineWidth=1;for(let y=0;y<=4;y++){const yy=10+y*(h-25)/4;ctx.beginPath();ctx.moveTo(38,yy);ctx.lineTo(w-8,yy);ctx.stroke()}if(!fs||!fs.length)return;const max=Math.min(fs.length,180),slice=fs.slice(-max),colors=[\"#65aaff\",\"#36e0a0\",\"#ffbd69\",\"#ff6578\",\"#bd91ff\",\"#56d5d5\",\"#e3eaf4\",\"#a6b4c8\"];for(let b=0;b<8;b++){ctx.beginPath();ctx.strokeStyle=colors[b];let started=false;slice.forEach((f,i)=>{if(f.data[b]===undefined)return;const x=38+i*(w-48)/Math.max(1,max-1),y=h-15-f.data[b]/255*(h-35);if(!started){ctx.moveTo(x,y);started=true}else ctx.lineTo(x,y)});ctx.stroke()}ctx.fillStyle=\"#8190a6\";ctx.font=\"11px sans-serif\";ctx.fillText(\"255\",5,18);ctx.fillText(\"0\",18,h-14)}\nfunction importText(txt,name){const parsed=parseText(txt);if(!parsed.length){alert(\"Не удалось распознать кадры. Проверьте формат CSV/ASC/TRC/candump.\");return}frames=[];parsed.forEach(addFrame);sourceName=name;formatName=(name.split(\".\").pop()||\"text\").toUpperCase()+\" · \"+parsed.length+\" frames\";selected=null;render()}\n$(\"fileInput\").addEventListener(\"change\",async e=>{const f=e.target.files[0];if(f)importText(await f.text(),f.name);e.target.value=\"\"});$(\"drop\").addEventListener(\"dragover\",e=>{e.preventDefault();$(\"drop\").classList.add(\"drag\")});$(\"drop\").addEventListener(\"dragleave\",()=>$(\"drop\").classList.remove(\"drag\"));$(\"drop\").addEventListener(\"drop\",async e=>{e.preventDefault();$(\"drop\").classList.remove(\"drag\");const f=e.dataTransfer.files[0];if(f)importText(await f.text(),f.name)});\n$(\"filter\").addEventListener(\"input\",render);$(\"sort\").addEventListener(\"change\",render);$(\"clearBtn\").onclick=()=>{frames=[];selected=null;sourceName=\"Нет данных\";formatName=\"—\";render()};$(\"pauseBtn\").onclick=()=>{paused=!paused;$(\"pauseBtn\").textContent=paused?\"▶ Продолжить\":\"Ⅱ Пауза\"};$(\"exportBtn\").onclick=()=>{const csv=[\"timestamp,id,dlc,data\",...frames.map(f=>[f.ts,\"0x\"+f.id.toString(16),f.dlc,'\"'+f.data.map(hex).join(\" \")+'\"'].join(\",\"))].join(\"\\n\");const a=document.createElement(\"a\");a.href=URL.createObjectURL(new Blob([csv],{type:\"text/csv\"}));a.download=\"exeed-can-export.csv\";a.click();URL.revokeObjectURL(a.href)};\n$(\"snapA\").onclick=()=>saveSnap(\"A\");$(\"snapB\").onclick=()=>saveSnap(\"B\");function saveSnap(k){snapshots[k]=frames.map(f=>({id:f.id,data:f.data.slice(),dlc:f.dlc}));$(\"snap\"+k+\"Count\").textContent=snapshots[k].length;$(\"snap\"+k+\"Info\").textContent=new Date().toLocaleTimeString(\"ru-RU\")+\" · \"+new Set(snapshots[k].map(f=>f.id)).size+\" IDs\"}\n$(\"compareBtn\").onclick=()=>{const a=snapshots.A,b=snapshots.B;if(!a||!b){$(\"compareResult\").textContent=\"Сначала сохраните оба снимка.\";return}const map=x=>{const m=new Map();x.forEach(f=>m.set(f.id,f.data));return m},ma=map(a),mb=map(b),ids=new Set([...ma.keys(),...mb.keys()]),changed=[];ids.forEach(id=>{const x=ma.get(id),y=mb.get(id);if(!x||!y||x.join(\",\")!==y.join(\",\"))changed.push({id,x,y})});$(\"compareResult\").innerHTML=changed.length?'<b>'+changed.length+' изменившихся ID</b><div class=\"scroll\" style=\"margin-top:8px\"><table><thead><tr><th>ID</th><th>Snapshot A</th><th>Snapshot B</th></tr></thead><tbody>'+changed.map(x=>'<tr><td class=\"id\">0x'+x.id.toString(16).toUpperCase()+'</td><td class=\"hex\">'+(x.x||[]).map(hex).join(\" \")+'</td><td class=\"hex\">'+(x.y||[]).map(hex).join(\" \")+'</td></tr>').join(\"\")+'</tbody></table></div>':'Сырые значения совпадают.'};\nfunction connectLive(){if(socket){try{socket.close()}catch{}socket=null}if(!liveOn){$(\"wsState\").textContent=\"Отключён\";$(\"liveStatus\").textContent=\"OFFLINE\";return}const proto=location.protocol===\"https:\"?\"wss:\":\"ws:\";socket=new WebSocket(proto+\"//\"+location.host+\"/viewer\");$(\"wsState\").textContent=\"Подключение…\";socket.onopen=()=>{$(\"wsState\").textContent=\"Подключён\";$(\"liveStatus\").textContent=\"ONLINE\"};socket.onmessage=e=>{if(paused)return;try{const p=JSON.parse(e.data);if(p.type===\"frames\"&&Array.isArray(p.frames)){p.frames.forEach(addFrame);liveCount+=p.frames.length;$(\"liveFrames\").textContent=liveCount;sourceName=\"LIVE /viewer\";formatName=\"Live CAN\";render()}}catch(err){console.error(err)}};socket.onclose=()=>{$(\"wsState\").textContent=\"Отключён\";$(\"liveStatus\").textContent=\"OFFLINE\";if(liveOn)setTimeout(()=>{if(liveOn)connectLive()},2500)};socket.onerror=()=>{$(\"wsState\").textContent=\"Ошибка\"}}\n$(\"liveBtn\").onclick=()=>{liveOn=!liveOn;$(\"liveBtn\").textContent=liveOn?\"● Live: вкл.\":\"● Live: выкл.\";connectLive()};\nrender();\n</script></body></html>";

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

    // CAN Decoder Lab: отдельная read-only страница анализа логов
    if (path === "/decoder" && !isWebSocket) {
      return new Response(DECODER_HTML, {
        headers: {
          "Content-Type": "text/html; charset=utf-8",
          "Cache-Control": "no-store",
          "X-Content-Type-Options": "nosniff"
        }
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
