#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cstring>
#include "driver/twai.h"

// EXEED CAN LAB v3.1: local-only passive CAN capture.
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
.panel{display:none}.panel.active{display:block}.tablewrap{overflow:auto;max-height:430px;border:1px solid var(--line);border-radius:8px}table{width:100%;border-collapse:collapse;white-space:nowrap;font:12px ui-monospace,monospace}th,td{text-align:left;padding:8px;border-bottom:1px solid var(--line)}th{position:sticky;top:0;background:#17243a;color:var(--muted)}td.id{color:var(--blue)}.pill{border:1px solid var(--line);border-radius:20px;padding:4px 9px}.event{border-left:3px solid var(--amber);padding:7px 10px;margin:6px 0;background:#101a2b}.heat{display:grid;grid-template-columns:repeat(8,minmax(28px,1fr));gap:6px}.bit{padding:9px 3px;text-align:center;border:1px solid var(--line);border-radius:8px;background:var(--p2)}.bit b{display:block;font-size:17px}.bit small{color:var(--muted);font-size:10px}.signalgrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}@media(max-width:700px){.signalgrid{grid-template-columns:1fr}}
@media(max-width:700px){main{padding:9px}.stats{grid-template-columns:repeat(2,1fr)}.metric{font-size:20px}h1{font-size:18px}}
</style></head><body><main>
<header><div><h1>EXEED CAN LAB <span class="pill">v3.1 LOCAL</span></h1><div class="hint">ESP32-S3 · 500 kbit/s · пассивное чтение · без интернета</div></div><span class="pill" id="link">Соединение…</span></header>
<section class="stats">
<div class="card"><div class="muted">CAN</div><div class="metric" id="can">…</div></div><div class="card"><div class="muted">Всего кадров</div><div class="metric" id="total">0</div></div>
<div class="card"><div class="muted">Уникальных ID</div><div class="metric" id="unique">0</div></div><div class="card"><div class="muted">Кадров в браузере</div><div class="metric" id="client">0</div></div></section>
<div class="card"><div id="errors" class="bar">Ожидание статистики…</div><p class="hint">RAM ESP32: 512 кадров. Браузер iPad: до 12 000 кадров текущей сессии. На флеш платы ничего не записывается.</p><div class="bar hint"><span>Потеряно в буфере платы: <b id="lost">0</b></span><span>Потеряно при опросе: <b id="gap">0</b></span><span>Скорость: <b id="rate">0</b> кадр/с</span></div></div>
<nav class="tabs"><button class="tab active" data-tab="live">CAN монитор</button><button class="tab" data-tab="ids">ID анализатор</button><button class="tab" data-tab="signals">Сигналы</button><button class="tab" data-tab="lab">Эксперименты</button><button class="tab" data-tab="export">Экспорт</button></nav>
<section class="panel active" id="p-live"><div class="card"><div class="toolbar"><input id="fid" placeholder="Фильтр ID, например 0x123" style="flex:1"><input id="fdata" placeholder="Поиск DATA" style="flex:1"><select id="ftype"><option value="ALL">Все кадры</option><option>STD</option><option>EXT</option></select><button id="pause">Пауза отображения</button><button id="clearview">Очистить экран</button></div><p class="hint">Пауза останавливает только отображение, сбор сессии продолжается.</p><div class="tablewrap"><table><thead><tr><th>Seq</th><th>Uptime ms</th><th>ID</th><th>Тип</th><th>DLC</th><th>DATA</th></tr></thead><tbody id="log"><tr><td colspan="6">Ожидание кадров…</td></tr></tbody></table></div></div></section>
<section class="panel" id="p-ids"><div class="card"><div class="toolbar"><input id="idq" placeholder="Поиск ID/метки" style="flex:1"><select id="sort"><option value="count">По количеству</option><option value="period">По периоду</option><option value="changes">По изменяемости</option><option value="id">По ID</option></select></div><div class="tablewrap"><table><thead><tr><th>ID</th><th>Тип</th><th>Кадров</th><th>Период</th><th>Min/Max</th><th>Байты изменялись</th><th>Последние данные</th><th>Метка (редактируется)</th></tr></thead><tbody id="inventory"></tbody></table></div><p class="hint">Нажми ID, чтобы отфильтровать монитор. Названия ID сохраняются только в локальном браузере iPad.</p></div></section>
<section class="panel" id="p-signals">
<div class="card"><b>Анализатор сигналов и битов</b><p class="hint">Выбери CAN ID и байт. Анализ строится по кадрам текущей сессии в браузере. Помогает находить биты, связанные с изменением органов управления климатом.</p><div class="toolbar"><select id="sigid" style="flex:2"><option value="">Нет ID</option></select><select id="sigbyte"><option value="0">Байт 0</option><option value="1">Байт 1</option><option value="2">Байт 2</option><option value="3">Байт 3</option><option value="4">Байт 4</option><option value="5">Байт 5</option><option value="6">Байт 6</option><option value="7">Байт 7</option></select><button id="sigrefresh">Обновить анализ</button></div><div class="signalgrid"><div><div class="muted">Статистика выбранного байта</div><div id="sigsummary" class="hint">Выбери ID с полученными кадрами.</div></div><div><div class="muted">Распределение значений</div><div id="sighist" class="hint">Нет данных.</div></div></div><p class="muted">Активность битов</p><div id="sigbits" class="heat"></div><p class="hint">«Переходы» показывает, сколько раз бит менял состояние между соседними кадрами этого ID. Это статистика, а не готовая расшифровка сигнала.</p></div>
<div class="card"><b>Периодичность выбранного ID</b><div id="sigperiod" class="hint">Нет данных.</div></div>
</section>
<section class="panel" id="p-lab">
<div class="card"><b>Метки действий</b><p class="hint">Ставь метку непосредственно перед или после изменения одной настройки: A/C, температура, вентилятор, направление обдува, обогрев.</p><div class="toolbar"><input id="eventname" placeholder="Например: Климат включён" style="flex:1"><button class="primary" id="mark">Добавить метку</button></div><div id="events"></div></div>
<div class="card"><b>Сравнение A/B</b><p class="hint">A фиксируй перед действием, B после него. Сравнение анализирует окно A→B и окно B→текущий момент.</p><div class="toolbar"><button id="seta">Зафиксировать A</button><button id="setb">Зафиксировать B</button><button class="primary" id="compare">Сравнить</button><button id="resetab">Сбросить</button></div><p class="hint" id="abstatus">Точки не установлены.</p><div class="tablewrap"><table><thead><tr><th>ID</th><th>Кадров A/B</th><th>DATA A</th><th>DATA B</th><th>Изменения</th></tr></thead><tbody id="ab"></tbody></table></div></div>
<div class="card"><button id="clearsession">Очистить сессию браузера</button> <span class="hint">Очищает только данные текущей вкладки.</span></div></section>
<section class="panel" id="p-export"><div class="card"><b>Экспорт на iPad</b><p class="hint">Файл формируется на iPad из данных текущей сессии. Ничего не скачивается автоматически и не сохраняется на ESP32.</p><p>Кадров: <b id="ecount">0</b> · Меток: <b id="evcount">0</b></p><div class="toolbar"><button class="primary" id="csv">Скачать CSV</button><button id="json">Скачать JSON</button></div></div></section>
<p class="hint">READ ONLY · TWAI LISTEN ONLY · CAN TX disabled · CAN FD не поддерживается.</p></main>
<script>
"use strict";
const $=id=>document.getElementById(id),S={seq:0,frames:[],events:[],inventory:[],paused:false,A:null,B:null,gap:0,labels:JSON.parse(localStorage.getItem("exeedCanLabels")||"{}"),rate:0,rateAt:Date.now(),rateCount:0};
const MAX=12000,pad=(n,w=2)=>Number(n).toString(16).toUpperCase().padStart(w,"0"),key=x=>(x.ext?"EXT":"STD")+":0x"+pad(x.id,x.ext?8:3),data=f=>f.data.map(v=>pad(v)).join(" ");
function switchTab(t){document.querySelectorAll(".tab").forEach(b=>b.classList.toggle("active",b.dataset.tab===t));document.querySelectorAll(".panel").forEach(p=>p.classList.toggle("active",p.id==="p-"+t));}
document.querySelectorAll(".tab").forEach(b=>b.onclick=()=>switchTab(b.dataset.tab));
function parseHex(s){s=s.trim().toLowerCase().replace(/^0x/,"");return s?(/^[0-9a-f]+$/.test(s)?parseInt(s,16):NaN):null;}
function addFrame(f){S.frames.push(f);if(S.frames.length>MAX){const n=S.frames.length-MAX;S.frames.splice(0,n);if(S.A)S.A.index=Math.max(0,S.A.index-n);if(S.B)S.B.index=Math.max(0,S.B.index-n);S.events.forEach(e=>e.index=Math.max(0,e.index-n));}}
function counts(){$("client").textContent=S.frames.length.toLocaleString();$("ecount").textContent=S.frames.length;$("evcount").textContent=S.events.length;}
function drawLog(){if(S.paused)return;const q=parseHex($("fid").value),d=$("fdata").value.replace(/\s/g,"").toUpperCase(),t=$("ftype").value;const list=S.frames.slice(-250).reverse().filter(f=>(q===null||(!Number.isNaN(q)&&f.id===q))&&(t==="ALL"||f.ext===(t==="EXT"))&&(!d||data(f).replace(/\s/g,"").includes(d)));const b=$("log");b.textContent="";for(const f of list){const tr=document.createElement("tr");[f.seq,f.ts,key(f),f.ext?"EXT":"STD",f.dlc,data(f)+(f.rtr?" RTR":"")].forEach((v,i)=>{const td=document.createElement("td");td.textContent=v;if(i===2)td.className="id";tr.appendChild(td)});b.appendChild(tr);}if(!list.length)b.innerHTML="<tr><td colspan='6'>Нет кадров по фильтру</td></tr>";}
function drawInventory(){const q=$("idq").value.toLowerCase(),sort=$("sort").value;let a=S.inventory.filter(x=>key(x).toLowerCase().includes(q)||(S.labels[key(x)]||"").toLowerCase().includes(q));a.sort((x,y)=>sort==="id"?x.id-y.id:sort==="period"?x.period-y.period:sort==="changes"?y.changed-x.changed:y.count-x.count);const b=$("inventory");b.textContent="";for(const x of a){const tr=document.createElement("tr"),k=key(x),vals=[k,x.ext?"EXT":"STD",x.count,x.period+" ms",x.min+"/"+x.max+" ms",x.changedBytes.join(", "),x.data.map(v=>pad(v)).join(" ")];vals.forEach((v,i)=>{const td=document.createElement("td");td.textContent=v;if(i===0){td.className="id";td.onclick=()=>{$("fid").value="0x"+pad(x.id,x.ext?8:3);switchTab("live");drawLog();};}tr.appendChild(td);});const td=document.createElement("td");td.contentEditable="true";td.textContent=S.labels[k]||"";td.onblur=()=>{S.labels[k]=td.textContent.trim();localStorage.setItem("exeedCanLabels",JSON.stringify(S.labels));};tr.appendChild(td);b.appendChild(tr);}}
function drawSignals(){const sel=$("sigid"),old=sel.value,items=S.inventory.slice().sort((a,b)=>a.id-b.id);sel.textContent="";items.forEach(x=>{const o=document.createElement("option");o.value=(x.ext?"E":"S")+":"+x.id;o.textContent=key(x)+" · "+x.count+" кадров";sel.appendChild(o);});if(items.some(x=>(x.ext?"E":"S")+":"+x.id===old))sel.value=old;if(!sel.value&&items.length)sel.value=(items[0].ext?"E":"S")+":"+items[0].id;const parts=sel.value.split(":");if(parts.length!==2){$("sigsummary").textContent="Пока нет CAN ID.";$("sighist").textContent="Нет данных.";$("sigbits").textContent="";$("sigperiod").textContent="Нет данных.";return;}const ext=parts[0]==="E",id=Number(parts[1]),bi=Number($("sigbyte").value),arr=S.frames.filter(f=>f.id===id&&f.ext===ext&&f.dlc>bi),vals=arr.map(f=>f.data[bi]);if(!vals.length){$("sigsummary").textContent="В текущей сессии нет кадров с выбранным байтом.";$("sighist").textContent="Нет данных.";$("sigbits").textContent="";return;}let ones=Array(8).fill(0),toggles=Array(8).fill(0);for(let n=0;n<vals.length;n++){for(let b=0;b<8;b++){const v=(vals[n]>>b)&1;ones[b]+=v;if(n&&v!==((vals[n-1]>>b)&1))toggles[b]++;}}const uniq=new Map();vals.forEach(v=>uniq.set(v,(uniq.get(v)||0)+1));const sorted=[...uniq.entries()].sort((a,b)=>b[1]-a[1]);const min=Math.min(...vals),max=Math.max(...vals),last=vals[vals.length-1];$("sigsummary").innerHTML="Кадров: <b>"+vals.length+"</b><br>Мин/макс: <b>0x"+pad(min)+" / 0x"+pad(max)+"</b><br>Уникальных значений: <b>"+uniq.size+"</b><br>Последнее значение: <b>0x"+pad(last)+" ("+last+")</b><br>Изменений байта: <b>"+vals.filter((v,i)=>i&&v!==vals[i-1]).length+"</b>";$("sighist").textContent=sorted.slice(0,16).map(([v,n])=>"0x"+pad(v)+" : "+n+" ("+(100*n/vals.length).toFixed(1)+"%)").join(" · ");const bits=$("sigbits");bits.textContent="";for(let b=7;b>=0;b--){const d=document.createElement("div");d.className="bit";d.innerHTML="<b>"+b+"</b><small>1: "+(100*ones[b]/vals.length).toFixed(1)+"%</small><small>Δ: "+toggles[b]+"</small>";bits.appendChild(d);}const periods=[];for(let i=1;i<arr.length;i++)periods.push(arr[i].ts-arr[i-1].ts);periods.sort((a,b)=>a-b);const median=periods.length?periods[Math.floor(periods.length/2)]:0;$("sigperiod").textContent="Кадров: "+arr.length+" · Медианный интервал: "+median+" мс · Min: "+(periods[0]??0)+" мс · Max: "+(periods[periods.length-1]??0)+" мс";}
$("sigid").onchange=drawSignals;$("sigbyte").onchange=drawSignals;$("sigrefresh").onclick=drawSignals;
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
$("clearsession").onclick=()=>{if(confirm("Очистить текущую сессию в браузере?")){S.frames=[];S.events=[];S.A=S.B=null;S.seq=0;S.gap=0;counts();drawEvents();drawLog();$("ab").textContent="";}};
async function poll(){try{const r=await fetch("/api/data?since="+S.seq,{cache:"no-store"});if(!r.ok)throw Error("HTTP "+r.status);const d=await r.json();$("link").textContent="ESP32 · "+location.host;$("link").className="pill green";$("can").textContent=d.canState;$("can").className="metric "+(d.canState==="RUNNING"?"green":"red");$("total").textContent=d.totalFrames.toLocaleString();$("unique").textContent=d.uniqueIds;$("errors").textContent="RX "+d.rxErrors+" · TX "+d.txErrors+" · BUS "+d.busErrors+" · MISSED "+d.rxMissed+" · OVERRUN "+d.rxOverrun+" · TX FAILED "+d.txFailed;$("lost").textContent=d.boardLost.toLocaleString();S.gap+=d.gap;$("gap").textContent=S.gap.toLocaleString();d.framesList.forEach(f=>{addFrame(f);S.seq=Math.max(S.seq,f.seq);});S.inventory=d.inventory;const now=Date.now();if(now-S.rateAt>=1000){S.rate=(S.frames.length-S.rateCount)*1000/(now-S.rateAt);S.rateCount=S.frames.length;S.rateAt=now;}$("rate").textContent=S.rate.toFixed(1);counts();drawInventory();if(document.getElementById("p-signals").classList.contains("active"))drawSignals();if(!S.paused)drawLog();}catch(e){$("link").textContent="Нет связи с ESP32";$("link").className="pill red";}}
setInterval(poll,180);poll();
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
static void printStatus(){updateCANStatus();uint32_t n=0;for(const auto&s:ids)if(s.used)n++;Serial.println("\n===== EXEED CAN LAB v3.1 =====");Serial.printf("CAN=%s frames=%lu uniqueIDs=%lu\n",canState.c_str(),(unsigned long)totalFrames,(unsigned long)n);Serial.printf("TWAI RX=%lu TX=%lu BUS=%lu MISSED=%lu OVERRUN=%lu TX_FAILED=%lu\n",(unsigned long)rxErrors,(unsigned long)txErrors,(unsigned long)busErrors,(unsigned long)rxMissed,(unsigned long)rxOverrun,(unsigned long)txFailed);Serial.printf("AP=%s IP=%s\n",AP_SSID,WiFi.softAPIP().toString().c_str());Serial.println("LISTEN ONLY / CAN TX DISABLED / FILE LOGGING DISABLED");}
void setup(){Serial.begin(115200);delay(1200);Serial.println("\nEXEED CAN LAB v3.1 / ESP32-S3 / LOCAL ONLY");canReady=initializeCAN();startWiFi();server.on("/",HTTP_GET,handleRoot);server.on("/api/data",HTTP_GET,handleApi);server.onNotFound([](){server.send(404,"text/plain","Not found");});server.begin();Serial.println("[WEB] http://192.168.4.1");}
void loop(){receiveCAN();server.handleClient();if(millis()-lastStatus>=5000){lastStatus=millis();printStatus();}delay(1);}
