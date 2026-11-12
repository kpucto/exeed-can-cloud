#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cstring>
#include "driver/twai.h"

// EXEED CAN LAB v3.2: local-only passive CAN capture.
// No cloud, no STA, no filesystem logging, no CAN transmit or diagnostics.
static constexpr gpio_num_t CAN_TX_PIN=GPIO_NUM_4, CAN_RX_PIN=GPIO_NUM_5;
static constexpr size_t FRAME_BUFFER_SIZE=512, ID_STATS_SIZE=128;
static constexpr uint16_t API_FRAME_LIMIT=160;
static const char *AP_SSID="EXEED_CAN_LAB", *AP_PASSWORD="exeed123";
WebServer server(80);

struct CanFrame { uint32_t seq=0,id=0,ts=0; uint8_t dlc=0,data[8]={}; bool ext=false,rtr=false; };
struct IdStats {
  bool used=false,ext=false; uint32_t id=0,count=0,firstSeen=0,lastSeen=0,lastPeriod=0,minPeriod=UINT32_MAX,maxPeriod=0;
  uint32_t changedBytes[8]={}; uint8_t dlc=0,lastData[8]={};
};
static CanFrame frames[FRAME_BUFFER_SIZE];
static IdStats ids[ID_STATS_SIZE];
static uint32_t totalFrames=0,rxErrors=0,txErrors=0,busErrors=0,rxMissed=0,rxOverrun=0,txFailed=0,lastStatus=0;
static bool canReady=false; static String canState="NOT_STARTED";

static String stateName(twai_state_t s){switch(s){case TWAI_STATE_STOPPED:return "STOPPED";case TWAI_STATE_RUNNING:return "RUNNING";case TWAI_STATE_BUS_OFF:return "BUS_OFF";case TWAI_STATE_RECOVERING:return "RECOVERING";default:return "UNKNOWN";}}
static IdStats* findId(uint32_t id,bool ext){
  IdStats *slot=nullptr;
  for(size_t i=0;i<ID_STATS_SIZE;i++){if(ids[i].used&&ids[i].id==id&&ids[i].ext==ext)return &ids[i];if(!ids[i].used&&!slot)slot=&ids[i];}
  if(!slot){slot=&ids[0];for(size_t i=1;i<ID_STATS_SIZE;i++)if((int32_t)(ids[i].lastSeen-slot->lastSeen)<0)slot=&ids[i];*slot=IdStats{};}
  slot->used=true;slot->id=id;slot->ext=ext;slot->firstSeen=millis();return slot;
}
static void saveFrame(const twai_message_t&m){
  uint32_t now=millis();CanFrame&f=frames[totalFrames%FRAME_BUFFER_SIZE];f.seq=++totalFrames;f.id=m.identifier;f.ts=now;f.dlc=min((uint8_t)m.data_length_code,(uint8_t)8);f.ext=m.extd;f.rtr=m.rtr;
  for(uint8_t i=0;i<8;i++)f.data[i]=(i<f.dlc&&!f.rtr)?m.data[i]:0;
  IdStats*s=findId(f.id,f.ext);if(s->count){s->lastPeriod=now-s->lastSeen;s->minPeriod=min(s->minPeriod,s->lastPeriod);s->maxPeriod=max(s->maxPeriod,s->lastPeriod);for(uint8_t i=0;i<f.dlc;i++)if(s->lastData[i]!=f.data[i])s->changedBytes[i]++;}
  s->count++;s->lastSeen=now;s->dlc=f.dlc;memcpy(s->lastData,f.data,8);
}
static void receiveCAN(){if(!canReady)return;twai_message_t m={};for(uint16_t i=0;i<250;i++){if(twai_receive(&m,0)!=ESP_OK)break;saveFrame(m);}}
static void updateCANStatus(){if(!canReady)return;twai_status_info_t s={};if(twai_get_status_info(&s)==ESP_OK){canState=stateName(s.state);rxErrors=s.rx_error_counter;txErrors=s.tx_error_counter;busErrors=s.bus_error_count;rxMissed=s.rx_missed_count;rxOverrun=s.rx_overrun_count;txFailed=s.tx_failed_count;}}
static bool initializeCAN(){
  twai_general_config_t g=TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN,CAN_RX_PIN,TWAI_MODE_LISTEN_ONLY);g.rx_queue_len=1000;g.tx_queue_len=0;g.alerts_enabled=TWAI_ALERT_ALL;
  twai_timing_config_t t=TWAI_TIMING_CONFIG_500KBITS();twai_filter_config_t f=TWAI_FILTER_CONFIG_ACCEPT_ALL();
  esp_err_t e=twai_driver_install(&g,&t,&f);if(e!=ESP_OK){Serial.printf("[CAN] install: %s\n",esp_err_to_name(e));return false;}
  e=twai_start();if(e!=ESP_OK){Serial.printf("[CAN] start: %s\n",esp_err_to_name(e));twai_driver_uninstall();return false;}
  Serial.println("[CAN] LISTEN ONLY active; TX queue disabled");return true;
}

static const char HTML[] PROGMEM=R"HTML(
<!doctype html><html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><meta name="theme-color" content="#0a1020"><title>EXEED CAN LAB</title>
<style>
:root{color-scheme:dark;--bg:#0a1020;--p:#121d30;--p2:#0e1728;--line:#263750;--txt:#eaf1fc;--muted:#91a3bd;--blue:#73b7ff;--green:#42dfa5;--red:#ff7185;--amber:#ffc36e}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--txt);font:14px -apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif}main{max-width:1500px;margin:auto;padding:14px}
header,.bar,.tabs,.stats,.toolbar{display:flex;gap:9px;flex-wrap:wrap;align-items:center;justify-content:space-between}h1{font-size:21px;margin:0}.muted,.hint{color:var(--muted)}.hint{line-height:1.45;font-size:12px}
.card{background:var(--p);border:1px solid var(--line);border-radius:12px;padding:13px;margin:10px 0}.stats{display:grid;grid-template-columns:repeat(4,1fr)}
.metric{font-size:24px;font-weight:750;margin-top:7px}.green{color:var(--green)}.red{color:var(--red)}
input,select,button{background:var(--p2);color:var(--txt);border:1px solid #354965;border-radius:8px;padding:10px;font:inherit;min-height:39px}button{cursor:pointer;font-weight:650}button.primary,.active{background:#174d7e;border-color:#438bc8}
.panel{display:none}.panel.active{display:block}.tablewrap{overflow:auto;max-height:430px;border:1px solid var(--line);border-radius:8px}table{width:100%;border-collapse:collapse;white-space:nowrap;font:12px ui-monospace,monospace}th,td{text-align:left;padding:8px;border-bottom:1px solid var(--line)}th{position:sticky;top:0;background:#17243a;color:var(--muted)}td.id{color:var(--blue)}.pill{border:1px solid var(--line);border-radius:20px;padding:4px 9px}.event{border-left:3px solid var(--amber);padding:7px 10px;margin:6px 0;background:#101a2b}.heat{display:grid;grid-template-columns:repeat(8,minmax(28px,1fr));gap:6px}.bit{padding:9px 3px;text-align:center;border:1px solid var(--line);border-radius:8px;background:var(--p2)}.bit b{display:block;font-size:17px}.bit small{color:var(--muted);font-size:10px}.signalgrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.chartbox{width:100%;height:230px;background:#0b1424;border:1px solid var(--line);border-radius:9px;display:block}.bytegrid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px}.bytecell{background:var(--p2);border:1px solid var(--line);border-radius:8px;padding:8px;min-width:0}.bytecell b{display:block;color:var(--blue);font:700 18px ui-monospace,monospace}.bytecell small{display:block;color:var(--muted);font-size:10px;overflow-wrap:anywhere}.dbrow{display:flex;gap:8px;align-items:center;flex-wrap:wrap;padding:8px;border-bottom:1px solid var(--line)}.dbrow span{flex:1;min-width:130px}.good{color:var(--green)}.warn{color:var(--amber)}@media(max-width:700px){.signalgrid{grid-template-columns:1fr}}
@media(max-width:700px){main{padding:9px}.stats{grid-template-columns:repeat(2,1fr)}.metric{font-size:20px}h1{font-size:18px}}
</style></head><body><main>
<header><div><h1>EXEED CAN LAB <span class="pill">v3.6 LOCAL</span></h1><div class="hint">ESP32-S3 · 500 kbit/s · пассивное чтение · без интернета</div></div><span class="pill" id="link">Соединение…</span></header>
<section class="stats">
<div class="card"><div class="muted">CAN</div><div class="metric" id="can">…</div></div><div class="card"><div class="muted">Всего кадров</div><div class="metric" id="total">0</div></div>
<div class="card"><div class="muted">Уникальных ID</div><div class="metric" id="unique">0</div></div><div class="card"><div class="muted">Кадров в браузере</div><div class="metric" id="client">0</div></div></section>
<div class="card"><div id="errors" class="bar">Ожидание статистики…</div><p class="hint">RAM ESP32: 512 кадров. Браузер iPad: до 12 000 кадров текущей сессии. На флеш платы ничего не записывается.</p><div class="bar hint"><span>Потеряно в буфере платы: <b id="lost">0</b></span><span>Потеряно при опросе: <b id="gap">0</b></span><span>Скорость: <b id="rate">0</b> кадр/с</span></div></div>
<nav class="tabs"><button class="tab active" data-tab="live">CAN монитор</button><button class="tab" data-tab="ids">ID анализатор</button><button class="tab" data-tab="signals">Сигналы</button><button class="tab" data-tab="lab">Эксперименты</button><button class="tab" data-tab="analytics">Графики и декодер</button><button class="tab" data-tab="database">База сигналов</button><button class="tab" data-tab="sessions">Сессии</button><button class="tab" data-tab="export">Экспорт</button></nav>
<section class="panel active" id="p-live"><div class="card"><div class="toolbar"><input id="fid" placeholder="Фильтр ID, например 0x123" style="flex:1"><input id="fdata" placeholder="Поиск DATA" style="flex:1"><select id="ftype"><option value="ALL">Все кадры</option><option>STD</option><option>EXT</option></select><button id="pause">Пауза отображения</button><button id="clearview">Очистить экран</button></div><p class="hint">Пауза останавливает только отображение, сбор сессии продолжается.</p><div class="tablewrap"><table><thead><tr><th>Seq</th><th>Uptime ms</th><th>ID</th><th>Тип</th><th>DLC</th><th>DATA</th></tr></thead><tbody id="log"><tr><td colspan="6">Ожидание кадров…</td></tr></tbody></table></div></div></section>
<section class="panel" id="p-ids"><div class="card"><div class="toolbar"><input id="idq" placeholder="Поиск ID/метки" style="flex:1"><select id="sort"><option value="count">По количеству</option><option value="period">По периоду</option><option value="changes">По изменяемости</option><option value="id">По ID</option></select></div><div class="tablewrap"><table><thead><tr><th>ID</th><th>Тип</th><th>Кадров</th><th>Период</th><th>Min/Max</th><th>Байты изменялись</th><th>Последние данные</th><th>Метка (редактируется)</th></tr></thead><tbody id="inventory"></tbody></table></div><p class="hint">Нажми ID, чтобы отфильтровать монитор. Названия ID сохраняются только в локальном браузере iPad.</p></div></section>
<section class="panel" id="p-signals">
<div class="card"><b>Анализатор сигналов и битов</b><p class="hint">Выбери CAN ID и байт. Анализ строится по кадрам текущей сессии в браузере. Помогает находить биты, связанные с изменением органов управления климатом.</p><div class="toolbar"><select id="sigid" style="flex:2"><option value="">Нет ID</option></select><select id="sigbyte"><option value="0">Байт 0</option><option value="1">Байт 1</option><option value="2">Байт 2</option><option value="3">Байт 3</option><option value="4">Байт 4</option><option value="5">Байт 5</option><option value="6">Байт 6</option><option value="7">Байт 7</option></select><button id="sigrefresh">Обновить анализ</button></div><div class="signalgrid"><div><div class="muted">Статистика выбранного байта</div><div id="sigsummary" class="hint">Выбери ID с полученными кадрами.</div></div><div><div class="muted">Распределение значений</div><div id="sighist" class="hint">Нет данных.</div></div></div><p class="muted">Активность битов</p><div id="sigbits" class="heat"></div><p class="hint">«Переходы» показывает, сколько раз бит менял состояние между соседними кадрами этого ID. Это статистика, а не готовая расшифровка сигнала.</p></div>
<div class="card"><b>Периодичность выбранного ID</b><div id="sigperiod" class="hint">Нет данных.</div></div>
</section>
<section class="panel" id="p-lab">
<div class="card"><b>Метки действий</b><p class="hint">Ставь метку непосредственно перед или после изменения одной настройки: A/C, температура, вентилятор, направление обдува, обогрев.</p><div class="toolbar"><input id="eventname" placeholder="Например: Климат включён" style="flex:1"><button class="primary" id="mark">Добавить метку</button></div><div id="events"></div></div>
<div class="card"><b>Сравнение A/B</b><p class="hint">A фиксируй перед действием, B после него. Сравнение анализирует окно A→B и окно B→текущий момент.</p><div class="toolbar"><button id="seta">Зафиксировать A</button><button id="setb">Зафиксировать B</button><button class="primary" id="compare">Сравнить</button><button id="resetab">Сбросить</button></div><p class="hint" id="abstatus">Точки не установлены.</p><div class="tablewrap"><table><thead><tr><th>ID</th><th>Кадров A/B</th><th>DATA A</th><th>DATA B</th><th>Изменения</th></tr></thead><tbody id="ab"></tbody></table></div></div>
<div class="card"><b>Автоматический поиск сигналов</b><p class="hint">Нажми «База», подожди 3–5 секунд, измени ровно одну настройку климата, сразу нажми «Действие» и снова подожди 3–5 секунд. Анализ сравнит распределение битов до и после действия. Нужны минимум 3 кадра с каждой стороны.</p><div class="toolbar"><input id="autolabel" placeholder="Действие: A/C включён, температура +1…" style="flex:1"><button id="autobase">База</button><button id="autoaction">Действие</button><button class="primary" id="autoanalyze">Анализировать</button><button id="autoreset">Сброс</button></div><p class="hint" id="autostatus">Точки не установлены.</p><div class="tablewrap"><table><thead><tr><th>CAN ID</th><th>Байт/бит</th><th>До</th><th>После</th><th>Δ, п.п.</th><th>Кадры до/после</th><th>Оценка</th></tr></thead><tbody id="autocandidates"><tr><td colspan="7">Ожидание эксперимента.</td></tr></tbody></table></div><p class="hint">Это поиск корреляций, не расшифровка протокола. Повтори действие несколько раз и проверь обратное изменение. Передача CAN отключена.</p></div>
<div class="card"><button id="clearsession">Очистить сессию браузера</button> <span class="hint">Очищает только данные текущей вкладки.</span></div></section>
<section class="panel" id="p-analytics"><div class="card"><b>График выбранного сигнала</b><p class="hint">История последних значений выбранного байта. График строится по кадрам текущей сессии в браузере.</p><div class="toolbar"><select id="chartid" style="flex:2"><option value="">Нет ID</option></select><select id="chartbyte"><option value="0">Байт 0</option><option value="1">Байт 1</option><option value="2">Байт 2</option><option value="3">Байт 3</option><option value="4">Байт 4</option><option value="5">Байт 5</option><option value="6">Байт 6</option><option value="7">Байт 7</option></select><button id="drawchart">Построить</button></div><canvas id="signalchart" class="chartbox" width="1200" height="300"></canvas><p class="hint" id="chartinfo">Выбери ID и байт.</p></div><div class="card"><b>Декодер последнего кадра</b><p class="hint">Показывает HEX, DEC, BIN и состояние отдельных битов. Это сырые данные, смысл сигнала нужно подтвердить экспериментами.</p><div id="decodehead" class="hint">Нет кадров.</div><div id="decodebytes" class="bytegrid"></div></div><div class="card"><b>Поиск повторяющихся последовательностей ID</b><p class="hint">Ищет повторяющиеся цепочки CAN ID в соседних кадрах. Полезно для поиска типичных последовательностей обмена, но не определяет их назначение.</p><div class="toolbar"><label>Длина цепочки <select id="seqLen"><option>3</option><option selected>4</option><option>5</option><option>6</option><option>7</option><option>8</option></select></label><button class="primary" id="seqscan">Найти повторения</button></div><div class="tablewrap"><table><thead><tr><th>Цепочка ID</th><th>Повторов</th><th>Средний интервал</th><th>Пример seq</th></tr></thead><tbody id="seqresults"><tr><td colspan="4">Нажми «Найти повторения».</td></tr></tbody></table></div></div></section>
<section class="panel" id="p-database"><div class="card"><b>Локальная база сигналов</b><p class="hint">Сохраняй проверенные гипотезы: CAN ID, байт, бит и своё описание. База хранится в браузере iPad, а не на плате. Не называй сигнал подтверждённым, пока не проверил повторными экспериментами.</p><div class="toolbar"><select id="dbid" style="flex:2"><option value="">Выбери CAN ID</option></select><select id="dbbyte"><option value="0">Байт 0</option><option value="1">Байт 1</option><option value="2">Байт 2</option><option value="3">Байт 3</option><option value="4">Байт 4</option><option value="5">Байт 5</option><option value="6">Байт 6</option><option value="7">Байт 7</option></select><select id="dbbit"><option value="-1">Весь байт</option><option value="0">Бит 0</option><option value="1">Бит 1</option><option value="2">Бит 2</option><option value="3">Бит 3</option><option value="4">Бит 4</option><option value="5">Бит 5</option><option value="6">Бит 6</option><option value="7">Бит 7</option></select></div><div class="toolbar"><input id="dbname" placeholder="Название сигнала, например AC_REQUEST" style="flex:2"><input id="dbnote" placeholder="Условия / заметки" style="flex:3"><button class="primary" id="dbsave">Сохранить сигнал</button></div><div id="dbitems"></div><div class="toolbar"><button id="dbexport">Экспорт базы JSON</button><button id="dbimportbtn">Импорт базы JSON</button><input type="file" id="dbfile" accept="application/json,.json" hidden></div></div></section>
<section class="panel" id="p-sessions"><div class="card"><b>Сохранение и восстановление сессий</b><p class="hint">Сессия включает кадры, метки, инвентарь ID и точки эксперимента. Сохраняй JSON-файл на iPad. После перезагрузки страницы можно загрузить его обратно. Большие записи занимают место в памяти браузера.</p><div class="toolbar"><button class="primary" id="savesession">Сохранить текущую сессию</button><button id="loadsessionbtn">Загрузить сессию JSON</button><input type="file" id="sessionfile" accept="application/json,.json" hidden></div><p class="hint" id="sessionstatus">Текущих кадров: 0.</p><p class="hint warn">При загрузке сессии текущие кадры в браузере будут заменены. Сначала сохрани их, если они нужны.</p></div></section>
<section class="panel" id="p-export"><div class="card"><b>Экспорт на iPad</b><p class="hint">Файл формируется на iPad из данных текущей сессии. Ничего не скачивается автоматически и не сохраняется на ESP32.</p><p>Кадров: <b id="ecount">0</b> · Меток: <b id="evcount">0</b></p><div class="toolbar"><button class="primary" id="csv">Скачать CSV</button><button id="json">Скачать JSON</button></div></div></section>
<p class="hint">READ ONLY · TWAI LISTEN ONLY · CAN TX disabled · CAN FD не поддерживается.</p></main>
<script>
"use strict";
const $=id=>document.getElementById(id),S={seq:0,serverSeq:0,frames:[],events:[],inventory:[],paused:false,A:null,B:null,autoBase:null,autoAction:null,gap:0,labels:JSON.parse(localStorage.getItem("exeedCanLabels")||"{}"),rate:0,rateAt:Date.now(),rateCount:0};
const MAX=12000,pad=(n,w=2)=>Number(n).toString(16).toUpperCase().padStart(w,"0"),key=x=>(x.ext?"EXT":"STD")+":0x"+pad(x.id,x.ext?8:3),data=f=>f.data.map(v=>pad(v)).join(" ");
function switchTab(t){document.querySelectorAll(".tab").forEach(b=>b.classList.toggle("active",b.dataset.tab===t));document.querySelectorAll(".panel").forEach(p=>p.classList.toggle("active",p.id==="p-"+t));}
document.querySelectorAll(".tab").forEach(b=>b.onclick=()=>switchTab(b.dataset.tab));
function parseHex(s){s=s.trim().toLowerCase().replace(/^0x/,"");return s?(/^[0-9a-f]+$/.test(s)?parseInt(s,16):NaN):null;}
function addFrame(f){S.frames.push(f);if(S.frames.length>MAX){const n=S.frames.length-MAX;S.frames.splice(0,n);if(S.A)S.A.index=Math.max(0,S.A.index-n);if(S.B)S.B.index=Math.max(0,S.B.index-n);S.events.forEach(e=>e.index=Math.max(0,e.index-n));}}
function counts(){$("client").textContent=S.frames.length.toLocaleString();$("ecount").textContent=S.frames.length;$("evcount").textContent=S.events.length;if($("sessionstatus"))$("sessionstatus").textContent="Текущих кадров: "+S.frames.length+" · меток: "+S.events.length+".";}
function drawLog(){if(S.paused)return;const q=parseHex($("fid").value),d=$("fdata").value.replace(/\s/g,"").toUpperCase(),t=$("ftype").value;const list=S.frames.slice(-250).reverse().filter(f=>(q===null||(!Number.isNaN(q)&&f.id===q))&&(t==="ALL"||f.ext===(t==="EXT"))&&(!d||data(f).replace(/\s/g,"").includes(d)));const b=$("log");b.textContent="";for(const f of list){const tr=document.createElement("tr");[f.seq,f.ts,key(f),f.ext?"EXT":"STD",f.dlc,data(f)+(f.rtr?" RTR":"")].forEach((v,i)=>{const td=document.createElement("td");td.textContent=v;if(i===2)td.className="id";tr.appendChild(td)});b.appendChild(tr);}if(!list.length)b.innerHTML="<tr><td colspan='6'>Нет кадров по фильтру</td></tr>";}
function drawInventory(){const q=$("idq").value.toLowerCase(),sort=$("sort").value;let a=S.inventory.filter(x=>key(x).toLowerCase().includes(q)||(S.labels[key(x)]||"").toLowerCase().includes(q));a.sort((x,y)=>sort==="id"?x.id-y.id:sort==="period"?x.period-y.period:sort==="changes"?y.changed-x.changed:y.count-x.count);const b=$("inventory");b.textContent="";for(const x of a){const tr=document.createElement("tr"),k=key(x),vals=[k,x.ext?"EXT":"STD",x.count,x.period+" ms",x.min+"/"+x.max+" ms",x.changedBytes.join(", "),x.data.map(v=>pad(v)).join(" ")];vals.forEach((v,i)=>{const td=document.createElement("td");td.textContent=v;if(i===0){td.className="id";td.onclick=()=>{$("fid").value="0x"+pad(x.id,x.ext?8:3);switchTab("live");drawLog();};}tr.appendChild(td);});const td=document.createElement("td");td.contentEditable="true";td.textContent=S.labels[k]||"";td.onblur=()=>{S.labels[k]=td.textContent.trim();localStorage.setItem("exeedCanLabels",JSON.stringify(S.labels));};tr.appendChild(td);b.appendChild(tr);}}
function drawSignals(){const sel=$("sigid"),old=sel.value,items=S.inventory.slice().sort((a,b)=>a.id-b.id);sel.textContent="";items.forEach(x=>{const o=document.createElement("option");o.value=(x.ext?"E":"S")+":"+x.id;o.textContent=key(x)+" · "+x.count+" кадров";sel.appendChild(o);});if(items.some(x=>(x.ext?"E":"S")+":"+x.id===old))sel.value=old;if(!sel.value&&items.length)sel.value=(items[0].ext?"E":"S")+":"+items[0].id;const parts=sel.value.split(":");if(parts.length!==2){$("sigsummary").textContent="Пока нет CAN ID.";$("sighist").textContent="Нет данных.";$("sigbits").textContent="";$("sigperiod").textContent="Нет данных.";return;}const ext=parts[0]==="E",id=Number(parts[1]),bi=Number($("sigbyte").value),arr=S.frames.filter(f=>f.id===id&&f.ext===ext&&f.dlc>bi),vals=arr.map(f=>f.data[bi]);if(!vals.length){$("sigsummary").textContent="В текущей сессии нет кадров с выбранным байтом.";$("sighist").textContent="Нет данных.";$("sigbits").textContent="";return;}let ones=Array(8).fill(0),toggles=Array(8).fill(0);for(let n=0;n<vals.length;n++){for(let b=0;b<8;b++){const v=(vals[n]>>b)&1;ones[b]+=v;if(n&&v!==((vals[n-1]>>b)&1))toggles[b]++;}}const uniq=new Map();vals.forEach(v=>uniq.set(v,(uniq.get(v)||0)+1));const sorted=[...uniq.entries()].sort((a,b)=>b[1]-a[1]);const min=Math.min(...vals),max=Math.max(...vals),last=vals[vals.length-1];$("sigsummary").innerHTML="Кадров: <b>"+vals.length+"</b><br>Мин/макс: <b>0x"+pad(min)+" / 0x"+pad(max)+"</b><br>Уникальных значений: <b>"+uniq.size+"</b><br>Последнее значение: <b>0x"+pad(last)+" ("+last+")</b><br>Изменений байта: <b>"+vals.filter((v,i)=>i&&v!==vals[i-1]).length+"</b>";$("sighist").textContent=sorted.slice(0,16).map(([v,n])=>"0x"+pad(v)+" : "+n+" ("+(100*n/vals.length).toFixed(1)+"%)").join(" · ");const bits=$("sigbits");bits.textContent="";for(let b=7;b>=0;b--){const d=document.createElement("div");d.className="bit";d.innerHTML="<b>"+b+"</b><small>1: "+(100*ones[b]/vals.length).toFixed(1)+"%</small><small>Δ: "+toggles[b]+"</small>";bits.appendChild(d);}const periods=[];for(let i=1;i<arr.length;i++)periods.push(arr[i].ts-arr[i-1].ts);periods.sort((a,b)=>a-b);const median=periods.length?periods[Math.floor(periods.length/2)]:0;$("sigperiod").textContent="Кадров: "+arr.length+" · Медианный интервал: "+median+" мс · Min: "+(periods[0]??0)+" мс · Max: "+(periods[periods.length-1]??0)+" мс";}
$("sigid").onchange=drawSignals;$("sigbyte").onchange=drawSignals;$("sigrefresh").onclick=drawSignals;
function autoPoint(){return {seq:S.seq,wall:new Date().toISOString()};}
function autoStatus(){const b=S.autoBase,a=S.autoAction;$("autostatus").textContent=(b?"База: seq "+b.seq+" ("+new Date(b.wall).toLocaleTimeString()+")":"База не установлена")+(a?" · Действие: seq "+a.seq+" ("+new Date(a.wall).toLocaleTimeString()+")":" · Действие не отмечено");}
function autoReset(){S.autoBase=S.autoAction=null;autoStatus();$("autocandidates").innerHTML="<tr><td colspan='7'>Эксперимент сброшен.</td></tr>";}
function autoAnalyze(){
 if(!S.autoBase||!S.autoAction||S.autoAction.seq<=S.autoBase.seq){$("autostatus").textContent="Сначала зафиксируй Базу, затем Действие.";return;}
 const before=S.frames.filter(f=>f.seq>S.autoBase.seq&&f.seq<=S.autoAction.seq),after=S.frames.filter(f=>f.seq>S.autoAction.seq),groups=new Map();
 for(const f of before.concat(after)){const k=(f.ext?"E":"S")+":"+f.id;let g=groups.get(k);if(!g){g={id:f.id,ext:f.ext,before:[],after:[]};groups.set(k,g);}if(f.seq<=S.autoAction.seq)g.before.push(f);else g.after.push(f);}
 const rows=[];
 for(const g of groups.values()){
  if(g.before.length<3||g.after.length<3)continue;
  for(let bi=0;bi<8;bi++){
   const bv=g.before.filter(f=>f.dlc>bi).map(f=>f.data[bi]),av=g.after.filter(f=>f.dlc>bi).map(f=>f.data[bi]);
   if(bv.length<3||av.length<3)continue;
   for(let bit=0;bit<8;bit++){
    const bp=100*bv.reduce((n,v)=>n+((v>>bit)&1),0)/bv.length,ap=100*av.reduce((n,v)=>n+((v>>bit)&1),0)/av.length,delta=ap-bp;
    if(Math.abs(delta)<15)continue;
    const score=Math.abs(delta)*Math.sqrt(Math.min(bv.length,av.length));
    rows.push({g,bi,bit,bp,ap,delta,score,bn:bv.length,an:av.length});
   }
  }
 }
 rows.sort((a,b)=>b.score-a.score);const body=$("autocandidates");body.textContent="";
 const top=rows.slice(0,100);
 top.forEach(x=>{const tr=document.createElement("tr"),vals=[key(x.g), "B"+x.bi+" / bit "+x.bit,x.bp.toFixed(1)+"%",x.ap.toFixed(1)+"%",(x.delta>0?"+":"")+x.delta.toFixed(1),x.bn+" / "+x.an,x.score.toFixed(1)];vals.forEach(v=>{const td=document.createElement("td");td.textContent=v;tr.appendChild(td);});body.appendChild(tr);});
 if(!top.length)body.innerHTML="<tr><td colspan='7'>Нет выраженных изменений. Проверь, что кадры поступают, увеличь время записи или повтори одно действие.</td></tr>";
 const label=$("autolabel").value.trim()||"действие";
 const trimmed=S.frames.length&&S.frames[0].seq>S.autoBase.seq+1;$("autostatus").textContent="«"+label+"» · до: "+before.length+" кадров · после: "+after.length+" кадров · кандидатов: "+rows.length+" · показано: "+top.length+(trimmed?" · ВНИМАНИЕ: начало эксперимента частично вытеснено из буфера браузера.":"")+".";
}
$("autobase").onclick=()=>{S.autoBase=autoPoint();S.autoAction=null;autoStatus();$("autocandidates").innerHTML="<tr><td colspan='7'>База записана. Выполни одно действие с климатом.</td></tr>";};
$("autoaction").onclick=()=>{if(!S.autoBase){$("autostatus").textContent="Сначала нажми «База».";return;}S.autoAction=autoPoint();autoStatus();$("autocandidates").innerHTML="<tr><td colspan='7'>Действие записано. Подожди 3–5 секунд и запускай анализ.</td></tr>";};
$("autoanalyze").onclick=autoAnalyze;$("autoreset").onclick=autoReset;
function refreshSelectors(){
 const ids=S.inventory.slice().sort((a,b)=>a.id-b.id);
 ["chartid","dbid"].forEach(id=>{
  const el=$(id),old=el.value;el.textContent="";
  const first=document.createElement("option");first.value="";first.textContent="Выбери CAN ID";el.appendChild(first);
  ids.forEach(x=>{const o=document.createElement("option");o.value=(x.ext?"E":"S")+":"+x.id;o.textContent=key(x)+" · "+x.count+" кадров";el.appendChild(o);});
  if(ids.some(x=>(x.ext?"E":"S")+":"+x.id===old))el.value=old;
 });
}
function drawChart(){
 const c=$("signalchart"),ctx=c.getContext("2d"),parts=$("chartid").value.split(":"),bi=Number($("chartbyte").value);
 ctx.clearRect(0,0,c.width,c.height);ctx.fillStyle="#0b1424";ctx.fillRect(0,0,c.width,c.height);
 if(parts.length!==2){$("chartinfo").textContent="Выбери CAN ID.";return;}
 const ext=parts[0]==="E",id=Number(parts[1]),arr=S.frames.filter(f=>f.id===id&&f.ext===ext&&f.dlc>bi).slice(-500);
 if(!arr.length){$("chartinfo").textContent="Нет данных для выбранного байта.";return;}
 const padL=48,padR=18,padT=18,padB=32,w=c.width-padL-padR,h=c.height-padT-padB;
 ctx.strokeStyle="#263750";ctx.lineWidth=1;ctx.fillStyle="#91a3bd";ctx.font="14px sans-serif";
 for(let i=0;i<=4;i++){const y=padT+h*i/4;ctx.beginPath();ctx.moveTo(padL,y);ctx.lineTo(padL+w,y);ctx.stroke();ctx.fillText(String(Math.round(255-i*255/4)),7,y+5);}
 const first=arr[0].ts,last=arr[arr.length-1].ts,span=Math.max(1,last-first);
 ctx.strokeStyle="#73b7ff";ctx.lineWidth=2;ctx.beginPath();
 arr.forEach((f,i)=>{const x=padL+(f.ts-first)/span*w,y=padT+h-(f.data[bi]/255)*h;if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);});ctx.stroke();
 ctx.fillStyle="#91a3bd";ctx.fillText("0x"+pad(id,ext?8:3)+" · byte "+bi+" · "+arr.length+" samples",padL,c.height-8);
 $("chartinfo").textContent="Последнее: "+arr[arr.length-1].data[bi]+" (0x"+pad(arr[arr.length-1].data[bi])+") · минимум: "+Math.min(...arr.map(f=>f.data[bi]))+" · максимум: "+Math.max(...arr.map(f=>f.data[bi]))+" · период: "+(last-first)+" мс.";
}
function drawDecoder(){
 const f=S.frames[S.frames.length-1];if(!f){$("decodehead").textContent="Нет кадров.";return;}
 $("decodehead").textContent="Seq "+f.seq+" · "+key(f)+" · DLC "+f.dlc+" · "+f.ts+" ms";
 const b=$("decodebytes");b.textContent="";
 for(let i=0;i<f.dlc;i++){const v=f.data[i],d=document.createElement("div");d.className="bytecell";const title=document.createElement("b");title.textContent="B"+i+" = 0x"+pad(v);const dec=document.createElement("small");dec.textContent="DEC "+v;const bin=document.createElement("small");bin.textContent="BIN "+v.toString(2).padStart(8,"0");const bits=document.createElement("small");bits.textContent="b7…b0: "+Array.from({length:8},(_,n)=>(v>>(7-n))&1).join("");d.append(title,dec,bin,bits);b.appendChild(d);}
}
function scanSequences(){
 const n=Number($("seqLen").value),a=S.frames,found=new Map();
 if(a.length<n){$("seqresults").innerHTML="<tr><td colspan='4'>Недостаточно кадров.</td></tr>";return;}
 for(let i=0;i<=a.length-n;i++){
  const part=a.slice(i,i+n),sig=part.map(f=>(f.ext?"E":"S")+":"+f.id.toString(16)+"/"+f.dlc).join("|");
  let g=found.get(sig);if(!g){g={sig,count:0,intervals:[],first:part[0].seq,lastStart:part[0].seq};found.set(sig,g);}
  if(g.count>0)g.intervals.push(part[0].ts-(g.lastTs||part[0].ts));
  g.count++;g.lastTs=part[0].ts;g.lastStart=part[0].seq;
 }
 const rows=[...found.values()].filter(x=>x.count>1).sort((x,y)=>y.count-x.count).slice(0,40),body=$("seqresults");body.textContent="";
 rows.forEach(x=>{const tr=document.createElement("tr"),avg=x.intervals.length?Math.round(x.intervals.reduce((a,b)=>a+b,0)/x.intervals.length):0,vals=[x.sig.split("|").map(z=>z.replace(/^[ES]:/,"")).join(" → "),x.count,avg+" ms",x.first];vals.forEach(v=>{const td=document.createElement("td");td.textContent=v;tr.appendChild(td);});body.appendChild(tr);});
 if(!rows.length)body.innerHTML="<tr><td colspan='4'>Повторяющихся цепочек пока не найдено.</td></tr>";
}
function loadDatabase(){try{const a=JSON.parse(localStorage.getItem("exeedCanSignalDB")||"[]");return Array.isArray(a)?a:[];}catch(e){return [];}}
function renderDatabase(){
 const body=$("dbitems"),a=loadDatabase();body.textContent="";
 if(!a.length){body.textContent="База пока пуста.";return;}
 a.forEach((x,i)=>{const row=document.createElement("div");row.className="dbrow";const label=document.createElement("span");label.textContent=x.name+" · "+x.id+" · byte "+x.byte+" · "+(x.bit<0?"весь байт":"bit "+x.bit)+" · "+(x.note||"без заметки");const del=document.createElement("button");del.textContent="Удалить";del.onclick=()=>{const z=loadDatabase();z.splice(i,1);localStorage.setItem("exeedCanSignalDB",JSON.stringify(z));renderDatabase();};row.append(label,del);body.appendChild(row);});
}
$("dbsave").onclick=()=>{
 const id=$("dbid").value;if(!id){$("sessionstatus").textContent="Сначала выбери CAN ID в разделе базы.";return;}
 const parts=id.split(":"),item={id:key({ext:parts[0]==="E",id:Number(parts[1])}),ext:parts[0]==="E",canId:Number(parts[1]),byte:Number($("dbbyte").value),bit:Number($("dbbit").value),name:$("dbname").value.trim()||"Без названия",note:$("dbnote").value.trim(),savedAt:new Date().toISOString()};
 const a=loadDatabase();a.push(item);localStorage.setItem("exeedCanSignalDB",JSON.stringify(a));$("dbname").value="";$("dbnote").value="";renderDatabase();
};
$("dbexport").onclick=()=>download("exeed-can-signal-database.json","application/json",JSON.stringify(loadDatabase(),null,2));
$("dbimportbtn").onclick=()=>$("dbfile").click();
$("dbfile").onchange=()=>{const file=$("dbfile").files[0];if(!file)return;const reader=new FileReader();reader.onload=()=>{try{const a=JSON.parse(reader.result);if(!Array.isArray(a))throw Error("Ожидается массив записей");localStorage.setItem("exeedCanSignalDB",JSON.stringify(a));renderDatabase();}catch(e){alert("Не удалось импортировать базу: "+e.message);}};reader.readAsText(file);};
$("savesession").onclick=()=>download("exeed-can-session-"+new Date().toISOString().replace(/[:.]/g,"-")+".json","application/json",JSON.stringify({format:"EXEED_CAN_LAB_SESSION",version:"3.6",createdAt:new Date().toISOString(),frames:S.frames,events:S.events,inventory:S.inventory,pointA:S.A,pointB:S.B,autoBase:S.autoBase,autoAction:S.autoAction,labels:S.labels},null,2));
$("loadsessionbtn").onclick=()=>$("sessionfile").click();
$("sessionfile").onchange=()=>{
 const file=$("sessionfile").files[0];if(!file)return;const reader=new FileReader();
 reader.onload=()=>{try{const d=JSON.parse(reader.result);if(!d||!Array.isArray(d.frames))throw Error("В файле нет массива frames");if(!confirm("Заменить текущую сессию загруженными "+d.frames.length+" кадрами?"))return;
 S.frames=d.frames.slice(-MAX);S.seq=S.frames.length?Math.max(...S.frames.map(f=>Number(f.seq)||0)):0;S.serverSeq=0;S.events=Array.isArray(d.events)?d.events:[];S.inventory=Array.isArray(d.inventory)?d.inventory:[];S.A=d.pointA||null;S.B=d.pointB||null;S.autoBase=d.autoBase||null;S.autoAction=d.autoAction||null;S.labels=d.labels||S.labels;localStorage.setItem("exeedCanLabels",JSON.stringify(S.labels));counts();drawEvents();drawInventory();drawLog();drawSignals();refreshSelectors();drawChart();drawDecoder();autoStatus();$("sessionstatus").textContent="Загружено "+S.frames.length+" кадров из "+file.name+". Новые кадры с платы продолжат добавляться с текущим seq.";switchTab("sessions");
 }catch(e){alert("Ошибка загрузки сессии: "+e.message);}};reader.readAsText(file);
};
$("chartid").onchange=drawChart;$("chartbyte").onchange=drawChart;$("drawchart").onclick=drawChart;$("seqscan").onclick=scanSequences;
function drawEvents(){const b=$("events");b.textContent="";S.events.forEach(e=>{const d=document.createElement("div");d.className="event";d.textContent=new Date(e.wall).toLocaleTimeString()+" · "+e.label+" · seq "+e.seq;b.appendChild(d);});}
function mark(label){if(!label)return;S.events.push({label,wall:new Date().toISOString(),seq:S.seq,index:S.frames.length});drawEvents();counts();}
function point(){return {index:S.frames.length,seq:S.seq,wall:new Date().toISOString()};}
function summarize(start,end){const m=new Map();for(const f of S.frames.slice(start,end)){const k=key(f);let x=m.get(k);if(!x){x={id:f.id,ext:f.ext,count:0,last:null};m.set(k,x);}x.count++;x.last=f;}return m;}
function compare(){if(!S.A||!S.B){$("abstatus").textContent="Установи точки A и B.";return;}const a=summarize(S.A.index,S.B.index),b=summarize(S.B.index,S.frames.length),keys=new Set([...a.keys(),...b.keys()]),rows=[];for(const k of keys){const x=a.get(k),y=b.get(k),r=y||x,changes=[];for(let i=0;i<8;i++){const av=x?.last?.data[i],bv=y?.last?.data[i];if(av!==bv&&(av!==undefined||bv!==undefined))changes.push("B"+i);}if(!x||!y||changes.length)rows.push({r,x,y,changes});}rows.sort((x,y)=>y.changes.length-x.changes.length||(y.y?.count||0)-(x.y?.count||0));$("ab").textContent="";rows.forEach(z=>{const tr=document.createElement("tr");[key(z.r),(z.x?.count||0)+" / "+(z.y?.count||0),z.x?data(z.x.last):"нет",z.y?data(z.y.last):"нет",z.changes.join(", ")||"ID появился/исчез"].forEach(v=>{const td=document.createElement("td");td.textContent=v;tr.appendChild(td)});$("ab").appendChild(tr);});$("abstatus").textContent="A: seq "+S.A.seq+" · B: seq "+S.B.seq+" · строк: "+rows.length;}
function download(name,type,content){const u=URL.createObjectURL(new Blob([content],{type})),a=document.createElement("a");a.href=u;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(u),1500);}
function exportRows(){return S.frames.map(f=>({seq:f.seq,uptime_ms:f.ts,id:"0x"+pad(f.id,f.ext?8:3),format:f.ext?"EXT":"STD",dlc:f.dlc,data:data(f),remote:f.rtr}));}
$("csv").onclick=()=>{const h=["seq","uptime_ms","id","format","dlc","data","remote"],q=v=>'"'+String(v??"").replace(/"/g,'""')+'"',rows=exportRows(),csv=[h.join(","),...rows.map(r=>h.map(k=>q(r[k])).join(","))].join("\r\n");download("exeed-can-"+new Date().toISOString().replace(/[:.]/g,"-")+".csv","text/csv;charset=utf-8","\uFEFF"+csv);};
$("json").onclick=()=>download("exeed-can-"+new Date().toISOString().replace(/[:.]/g,"-")+".json","application/json",JSON.stringify({createdAt:new Date().toISOString(),frames:exportRows(),events:S.events,pointA:S.A,pointB:S.B,inventory:S.inventory},null,2));
$("pause").onclick=()=>{S.paused=!S.paused;$("pause").textContent=S.paused?"Продолжить":"Пауза отображения";if(!S.paused)drawLog();};
$("clearview").onclick=()=>$("log").textContent="";["fid","fdata"].forEach(id=>$(id).oninput=drawLog);$("ftype").onchange=drawLog;$("idq").oninput=drawInventory;$("sort").onchange=drawInventory;
$("mark").onclick=()=>{mark($("eventname").value.trim());$("eventname").value="";};$("seta").onclick=()=>{S.A=point();$("abstatus").textContent="Точка A установлена, seq "+S.A.seq;};$("setb").onclick=()=>{S.B=point();$("abstatus").textContent="Точка B установлена, seq "+S.B.seq;};$("compare").onclick=compare;
$("resetab").onclick=()=>{S.A=S.B=null;$("ab").textContent="";$("abstatus").textContent="Точки сброшены.";};
$("clearsession").onclick=()=>{if(confirm("Очистить текущую сессию в браузере?")){S.frames=[];S.events=[];S.A=S.B=null;S.autoBase=S.autoAction=null;S.seq=0;S.gap=0;counts();drawEvents();drawLog();drawSignals();drawDecoder();drawChart();$("ab").textContent="";autoStatus();}};
async function poll(){try{const r=await fetch("/api/data?since="+S.serverSeq,{cache:"no-store"});if(!r.ok)throw Error("HTTP "+r.status);const d=await r.json();$("link").textContent="ESP32 · "+location.host;$("link").className="pill green";$("can").textContent=d.canState;$("can").className="metric "+(d.canState==="RUNNING"?"green":"red");$("total").textContent=d.totalFrames.toLocaleString();$("unique").textContent=d.uniqueIds;$("errors").textContent="RX "+d.rxErrors+" · TX "+d.txErrors+" · BUS "+d.busErrors+" · MISSED "+d.rxMissed+" · OVERRUN "+d.rxOverrun+" · TX FAILED "+d.txFailed;$("lost").textContent=d.boardLost.toLocaleString();S.gap+=d.gap;$("gap").textContent=S.gap.toLocaleString();d.framesList.forEach(f=>{S.serverSeq=Math.max(S.serverSeq,f.seq);const localFrame=Object.assign({},f,{deviceSeq:f.seq,seq:++S.seq});addFrame(localFrame);});S.inventory=d.inventory;const now=Date.now();if(now-S.rateAt>=1000){S.rate=(S.frames.length-S.rateCount)*1000/(now-S.rateAt);S.rateCount=S.frames.length;S.rateAt=now;}$("rate").textContent=S.rate.toFixed(1);counts();drawInventory();if(document.getElementById("p-signals").classList.contains("active"))drawSignals();if(document.getElementById("p-analytics").classList.contains("active")){refreshSelectors();drawChart();drawDecoder();}if(document.getElementById("p-database").classList.contains("active"))refreshSelectors();if(!S.paused)drawLog();}catch(e){$("link").textContent="Нет связи с ESP32";$("link").className="pill red";}}
renderDatabase();counts();setInterval(poll,180);poll();
</script></body></html>
)HTML";

static void handleRoot(){server.send_P(200,"text/html; charset=utf-8",HTML);}
static void handleApi(){
  updateCANStatus();uint32_t since=server.hasArg("since")?strtoul(server.arg("since").c_str(),nullptr,10):0;
  uint32_t oldest=totalFrames>FRAME_BUFFER_SIZE?totalFrames-FRAME_BUFFER_SIZE+1:(totalFrames?1:0);
  uint32_t overwritten=(oldest&&since+1<oldest)?oldest-since-1:0,start=since+1,end=totalFrames;
  if(oldest&&start<oldest)start=oldest;
  if(end>=start&&end-start+1>API_FRAME_LIMIT)start=end-API_FRAME_LIMIT+1;
  uint32_t gap=start>since+1?start-since-1:0;
  uint32_t unique=0;for(const auto&s:ids)if(s.used)unique++;
  String out;out.reserve(26000);
  out="{\"canState\":\""+canState+"\",\"totalFrames\":"+String(totalFrames)+",\"uniqueIds\":"+String(unique)+",\"boardLost\":"+String(overwritten)+",\"gap\":"+String(gap)+
    ",\"rxErrors\":"+String(rxErrors)+",\"txErrors\":"+String(txErrors)+",\"busErrors\":"+String(busErrors)+",\"rxMissed\":"+String(rxMissed)+",\"rxOverrun\":"+String(rxOverrun)+",\"txFailed\":"+String(txFailed)+",\"framesList\":[";
  bool first=true;for(uint32_t seq=start;seq<=end&&seq;seq++){const CanFrame&f=frames[(seq-1)%FRAME_BUFFER_SIZE];if(f.seq!=seq)continue;if(!first)out+=",";first=false;
    out+="{\"seq\":"+String(f.seq)+",\"ts\":"+String(f.ts)+",\"id\":"+String(f.id)+",\"dlc\":"+String(f.dlc)+",\"ext\":"+String(f.ext?"true":"false")+",\"rtr\":"+String(f.rtr?"true":"false")+",\"data\":[";
    for(uint8_t i=0;i<f.dlc;i++){if(i)out+=",";out+=String(f.data[i]);}out+="]}";}
  out+="],\"inventory\":[";first=true;for(const auto&s:ids){if(!s.used)continue;if(!first)out+=",";first=false;uint32_t changed=0;
    out+="{\"id\":"+String(s.id)+",\"ext\":"+String(s.ext?"true":"false")+",\"count\":"+String(s.count)+",\"period\":"+String(s.lastPeriod)+",\"min\":"+String(s.minPeriod==UINT32_MAX?0:s.minPeriod)+",\"max\":"+String(s.maxPeriod)+",\"dlc\":"+String(s.dlc)+",\"changedBytes\":[";
    for(uint8_t i=0;i<8;i++){if(i)out+=",";out+=String(s.changedBytes[i]);if(s.changedBytes[i])changed++;}out+="],\"changed\":"+String(changed)+",\"data\":[";
    for(uint8_t i=0;i<s.dlc;i++){if(i)out+=",";out+=String(s.lastData[i]);}out+="]}";}
  out+="]}";server.sendHeader("Cache-Control","no-store");server.send(200,"application/json; charset=utf-8",out);
}
static void startWiFi(){WiFi.mode(WIFI_AP);WiFi.setSleep(false);bool ok=WiFi.softAP(AP_SSID,AP_PASSWORD,6,false,4);Serial.printf("[WIFI] AP %s SSID=%s IP=%s\n",ok?"READY":"FAILED",AP_SSID,WiFi.softAPIP().toString().c_str());}
static void printStatus(){updateCANStatus();uint32_t n=0;for(const auto&s:ids)if(s.used)n++;Serial.println("\n===== EXEED CAN LAB v3.6 =====");Serial.printf("CAN=%s frames=%lu uniqueIDs=%lu\n",canState.c_str(),(unsigned long)totalFrames,(unsigned long)n);Serial.printf("TWAI RX=%lu TX=%lu BUS=%lu MISSED=%lu OVERRUN=%lu TX_FAILED=%lu\n",(unsigned long)rxErrors,(unsigned long)txErrors,(unsigned long)busErrors,(unsigned long)rxMissed,(unsigned long)rxOverrun,(unsigned long)txFailed);Serial.printf("AP=%s IP=%s\n",AP_SSID,WiFi.softAPIP().toString().c_str());Serial.println("LISTEN ONLY / CAN TX DISABLED / FILE LOGGING DISABLED");}
void setup(){Serial.begin(115200);delay(1200);Serial.println("\nEXEED CAN LAB v3.6 / ESP32-S3 / LOCAL ONLY");canReady=initializeCAN();startWiFi();server.on("/",HTTP_GET,handleRoot);server.on("/api/data",HTTP_GET,handleApi);server.onNotFound([](){server.send(404,"text/plain","Not found");});server.begin();Serial.println("[WEB] http://192.168.4.1");}
void loop(){receiveCAN();server.handleClient();if(millis()-lastStatus>=5000){lastStatus=millis();printStatus();}delay(1);}
