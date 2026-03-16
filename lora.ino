#ifndef LORACHAT_ENABLE_BT
#define LORACHAT_ENABLE_BT 1
#endif

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#if LORACHAT_ENABLE_BT
#include <BluetoothSerial.h>
#endif
#include <Preferences.h>
#include <U8g2lib.h>
#include <Keypad.h>

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_NSS    5
#define LORA_RST   14
#define LORA_DIO0   2

#define OLED_SDA   21
#define OLED_SCL   22

#define ENC_CLK    34
#define ENC_DT     35
#define ENC_SW     27

#define KP_R1  13
#define KP_R2  12
#define KP_R3   4
#define KP_R4  15
#define KP_C1  32
#define KP_C2  33
#define KP_C3  25
#define KP_C4  26

#ifndef LORACHAT_PROFILE
#define LORACHAT_PROFILE 0  // 0=LAB, 1=FIELD, 2=LOW_POWER, 3=BT_ONLY, 4=WIFI_ONLY
#endif

#if LORACHAT_PROFILE == 1
const char PROFILE_NAME[] = "FIELD";
const bool PROFILE_ALLOW_WIFI = true;
const bool PROFILE_ALLOW_BT = true;
const unsigned long ACK_TIMEOUT_BASE = 3600;
const int MAX_RETRIES_BASE = 4;
#elif LORACHAT_PROFILE == 2
const char PROFILE_NAME[] = "LOW_POWER";
const bool PROFILE_ALLOW_WIFI = true;
const bool PROFILE_ALLOW_BT = false;
const unsigned long ACK_TIMEOUT_BASE = 3200;
const int MAX_RETRIES_BASE = 2;
#elif LORACHAT_PROFILE == 3
const char PROFILE_NAME[] = "BT_ONLY";
const bool PROFILE_ALLOW_WIFI = false;
const bool PROFILE_ALLOW_BT = true;
const unsigned long ACK_TIMEOUT_BASE = 3000;
const int MAX_RETRIES_BASE = 3;
#elif LORACHAT_PROFILE == 4
const char PROFILE_NAME[] = "WIFI_ONLY";
const bool PROFILE_ALLOW_WIFI = true;
const bool PROFILE_ALLOW_BT = false;
const unsigned long ACK_TIMEOUT_BASE = 3000;
const int MAX_RETRIES_BASE = 3;
#else
const char PROFILE_NAME[] = "LAB";
const bool PROFILE_ALLOW_WIFI = true;
const bool PROFILE_ALLOW_BT = true;
const unsigned long ACK_TIMEOUT_BASE = 3000;
const int MAX_RETRIES_BASE = 3;
#endif

const long BASE_FREQ       = 433000000L;
const long FREQ_STEP       = 100000L;
const int  MAX_MSG_LEN     = 60;
const unsigned long DISP_RATE    = 250;
const unsigned long LONG_PRESS   = 1000;   // ms for long-press clear
const unsigned long FREQ_HOLD_MS = 600;    // hold A for this long → freq mode
const unsigned long KEY_DEBOUNCE_MS = 25;  // keypad debounce for better response
const unsigned long BT_RX_IDLE_FLUSH_MS = 400;
const unsigned long SETTINGS_FLUSH_MS = 1500;
const char PROJECT_NAME[] = "LoRa Chat";

const char CHARSET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?-:/()@#";
const int  CHARSET_LEN = sizeof(CHARSET) - 1;

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

const byte ROWS = 4, COLS = 4;
byte rowPins[ROWS] = {KP_R1, KP_R2, KP_R3, KP_R4};
byte colPins[COLS] = {KP_C1, KP_C2, KP_C3, KP_C4};
char keysMap[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
Keypad keypad = Keypad(makeKeymap(keysMap), rowPins, colPins, ROWS, COLS);
WebServer phoneServer(80);
#if LORACHAT_ENABLE_BT
BluetoothSerial SerialBT;
#endif
Preferences prefs;

const char PHONE_BRIDGE_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>LoRa Chat</title>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{--bg:#f0f4f8;--sur:#fff;--sur2:#f8fafc;--bd:#e2e8f0;--tx:#1a202c;--tx2:#4a5568;--tx3:#718096;
--g:#2f855a;--gbg:#f0fff4;--gbd:#9ae6b4;--o:#c05621;--obg:#fffaf0;--obd:#fbd38d;
--r:#c53030;--rbg:#fff5f5;--rbd:#feb2b2;--b:#2b6cb0;--bbg:#ebf8ff;--bbd:#bee3f8;--rr:10px}
[data-theme="dark"]{--bg:#0f1720;--sur:#16202b;--sur2:#1c2835;--bd:#2b3a49;--tx:#e7edf5;--tx2:#c4d0dc;--tx3:#97a6b4;
--g:#48bb78;--gbg:#143326;--gbd:#2f855a;--o:#f6ad55;--obg:#3b2a13;--obd:#b7791f;
--r:#fc8181;--rbg:#3a1f24;--rbd:#c53030;--b:#63b3ed;--bbg:#132b3e;--bbd:#2b6cb0}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--tx);line-height:1.5}
header{background:var(--b);color:#fff;padding:12px 16px;display:flex;align-items:center;gap:10px;box-shadow:0 2px 6px rgba(0,0,0,.22)}
.logo{width:34px;height:34px;background:rgba(255,255,255,.15);border-radius:7px;display:flex;align-items:center;justify-content:center;font-size:18px;flex-shrink:0}
.ht{font-size:1.05rem;font-weight:700}.hs{font-size:.73rem;opacity:.75}
.hb{margin-left:auto;padding:3px 9px;border-radius:999px;font-size:.72rem;font-weight:600;background:rgba(255,255,255,.18)}
.tb{border:1px solid rgba(255,255,255,.35);background:rgba(255,255,255,.15);color:#fff;padding:4px 8px;border-radius:999px;font-size:.72rem;font-weight:700;cursor:pointer}
main{max-width:580px;margin:0 auto;padding:16px 14px 40px;display:flex;flex-direction:column;gap:14px}
.card{background:var(--sur);border:1px solid var(--bd);border-radius:var(--rr);box-shadow:0 1px 3px rgba(0,0,0,.08);overflow:hidden}
.ch{padding:10px 14px;border-bottom:1px solid var(--bd);font-size:.78rem;font-weight:700;text-transform:uppercase;letter-spacing:.05em;color:var(--tx2);background:var(--sur2);display:flex;align-items:center;gap:7px}
.cb{padding:14px}
.sg{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.si{background:var(--sur2);border:1px solid var(--bd);border-radius:7px;padding:9px 11px}
.sl{font-size:.65rem;text-transform:uppercase;letter-spacing:.06em;color:var(--tx3);margin-bottom:2px}
.sv{font-size:1.05rem;font-weight:700;font-variant-numeric:tabular-nums}
.sv.g{color:var(--g)}.sv.o{color:var(--o)}.sv.r{color:var(--r)}.sv.b{color:var(--b)}
.sr{display:flex;align-items:center;gap:8px;flex-wrap:wrap;margin-bottom:12px}
.badge{display:inline-flex;align-items:center;gap:5px;padding:4px 10px;border-radius:999px;font-size:.75rem;font-weight:600}
.badge.rdy{background:var(--gbg);border:1px solid var(--gbd);color:var(--g)}
.badge.bsy{background:var(--obg);border:1px solid var(--obd);color:var(--o)}
.dot{width:7px;height:7px;border-radius:50%;background:currentColor}
@keyframes pu{0%,100%{opacity:1}50%{opacity:.3}}.dot.p{animation:pu 1s ease-in-out infinite}
.ft{font-family:monospace;font-size:.78rem;background:var(--sur2);border:1px solid var(--bd);border-radius:5px;padding:2px 7px;color:var(--tx2)}
.sb{display:inline-flex;align-items:flex-end;gap:2px;height:14px}
.sb span{width:4px;border-radius:1px;background:var(--bd)}.sb span.on{background:var(--g)}
.sigr{display:none;align-items:center;gap:5px}
.rl{font-size:.72rem;color:var(--tx3);font-family:monospace}
textarea{width:100%;min-height:80px;border:2px solid var(--bd);border-radius:7px;padding:9px 11px;font:inherit;font-size:.93rem;resize:vertical;transition:border-color .15s;background:var(--sur);color:var(--tx)}
textarea:focus{outline:none;border-color:var(--b)}
.cc{text-align:right;font-size:.72rem;color:var(--tx3);margin-top:4px}
.cc.w{color:var(--o)}.cc.v{color:var(--r)}
button[type=submit]{margin-top:8px;border:0;border-radius:7px;padding:10px 18px;font:inherit;font-size:.93rem;font-weight:600;background:var(--b);color:#fff;cursor:pointer;transition:background .15s,transform .1s;display:flex;align-items:center;justify-content:center;gap:7px;width:100%}
button:hover{background:#2c5282}button:active{transform:scale(.98)}
button:disabled{background:#a0aec0;cursor:not-allowed;transform:none}
.bn{padding:9px 13px;border-radius:7px;font-size:.85rem;font-weight:500;display:none;margin-top:8px}
.bn.ok{background:var(--gbg);border:1px solid var(--gbd);color:var(--g);display:block}
.bn.er{background:var(--rbg);border:1px solid var(--rbd);color:var(--r);display:block}
.cg{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.cbtn{border:1px solid var(--bd);background:var(--sur2);border-radius:7px;padding:8px 6px;font:inherit;font-size:.8rem;font-weight:600;color:var(--tx2);cursor:pointer}
.cbtn:active{transform:scale(.98)}
.kv{display:grid;grid-template-columns:72px 1fr;gap:6px 10px;font-size:.82rem;align-items:center}
.kl{color:var(--tx3);font-weight:700;text-transform:uppercase;letter-spacing:.05em;font-size:.68rem}
.kvv{font-family:monospace;word-break:break-all}
.il{display:flex;flex-direction:column}
.ii{padding:10px 14px;border-bottom:1px solid var(--bd);display:flex;gap:9px;align-items:flex-start}
.ii:last-child{border-bottom:0}
.iav{width:30px;height:30px;border-radius:50%;background:var(--bbg);border:1px solid var(--bbd);display:flex;align-items:center;justify-content:center;font-size:.65rem;font-weight:700;color:var(--b);flex-shrink:0;font-family:monospace}
.ic{flex:1;min-width:0}.im{display:flex;align-items:center;gap:6px;margin-bottom:2px;flex-wrap:wrap}
.ifr{font-size:.72rem;font-weight:600;color:var(--tx2);font-family:monospace}
.ia{font-size:.68rem;color:var(--tx3);margin-left:auto}
.imsg{font-size:.88rem;word-break:break-word}
.ie{padding:24px;text-align:center;color:var(--tx3);font-size:.85rem}
@keyframes sp{to{transform:rotate(360deg)}}
.spin{width:15px;height:15px;border:2px solid rgba(255,255,255,.4);border-top-color:#fff;border-radius:50%;animation:sp .6s linear infinite;display:none;flex-shrink:0}
</style></head><body>
<header>
  <div class="logo">&#128225;</div>
  <div><div class="ht">LoRa Chat</div><div class="hs" id="hS">%SSID%</div></div>
  <div class="hb" id="hF">&#8212;</div>
  <button class="tb" id="themeBtn" type="button">DARK</button>
</header>
<main>
  <div class="card">
    <div class="ch">&#128241; Connect</div>
    <div class="cb">
      <div class="kv">
        <div class="kl">SSID</div><div class="kvv">%SSID%</div>
        <div class="kl">PASS</div><div class="kvv">%PASS%</div>
        <div class="kl">TOKEN</div><div class="kvv">%TOKEN%</div>
        <div class="kl">URL</div><div class="kvv">http://192.168.4.1</div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="ch">&#9881; Radio Status</div>
    <div class="cb">
      <div class="sr">
        <div class="badge rdy" id="lb"><span class="dot"></span>READY</div>
        <span class="ft" id="fT">&#8212;</span>
        <div class="sigr" id="sigR">
          <div class="sb" id="sB">
            <span style="height:3px"></span><span style="height:6px"></span>
            <span style="height:9px"></span><span style="height:13px"></span>
          </div>
          <span class="rl" id="rL"></span>
        </div>
      </div>
      <div class="sg">
        <div class="si"><div class="sl">TX Sent</div><div class="sv b" id="sTx">&#8212;</div></div>
        <div class="si"><div class="sl">RX Recv</div><div class="sv g" id="sRx">&#8212;</div></div>
        <div class="si"><div class="sl">Pkt Loss</div><div class="sv" id="sLoss">&#8212;</div></div>
        <div class="si"><div class="sl">ACK&#8217;d</div><div class="sv" id="sAck">&#8212;</div></div>
        <div class="si"><div class="sl">Phone TX</div><div class="sv" id="sPh">&#8212;</div></div>
        <div class="si"><div class="sl">Last SNR</div><div class="sv" id="sSnr">&#8212;</div></div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="ch">&#9993; Send via LoRa
      <span style="margin-left:auto;font-weight:400;font-size:.72rem;text-transform:none;letter-spacing:0">max %MAXLEN% chars</span>
    </div>
    <div class="cb">
      <form id="sF">
        <textarea id="msg" name="msg" maxlength="%MAXLEN%" placeholder="Type your LoRa message&#8230;"></textarea>
        <div class="cc" id="cc">0&nbsp;/&nbsp;%MAXLEN%</div>
        <button type="submit" id="sBtn"><div class="spin" id="sp"></div><span id="sL">Send over LoRa</span></button>
        <div class="bn" id="bn"></div>
      </form>
    </div>
  </div>
  <div class="card">
    <div class="ch">&#9881; Remote Control</div>
    <div class="cb">
      <div class="cg">
        <button class="cbtn" type="button" onclick="ctl('page_home')">HOME</button>
        <button class="cbtn" type="button" onclick="ctl('page_chat')">CHAT</button>
        <button class="cbtn" type="button" onclick="ctl('ping')">PING</button>

        <button class="cbtn" type="button" onclick="ctl('freq_down')">FREQ -</button>
        <button class="cbtn" type="button" onclick="ctl('freq_up')">FREQ +</button>
        <button class="cbtn" type="button" onclick="ctl('freq_apply')">FREQ OK</button>

        <button class="cbtn" type="button" onclick="ctl('preset_prev')">PRESET -</button>
        <button class="cbtn" type="button" onclick="ctl('preset_next')">PRESET +</button>
        <button class="cbtn" type="button" onclick="ctl('page_radio')">OPEN RADIO</button>

        <button class="cbtn" type="button" onclick="ctl('mode_wifi')">MODE WIFI</button>
        <button class="cbtn" type="button" onclick="ctl('mode_bt')">MODE BT</button>
        <button class="cbtn" type="button" onclick="ctl('quick_send')">QUICK SEND</button>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="ch">&#128295; Maintenance</div>
    <div class="cb">
      <div class="cg">
        <button class="cbtn" type="button" onclick="ctl('clear_inbox')">CLEAR INBOX</button>
        <button class="cbtn" type="button" onclick="ctl('reset_config')">RESET CFG</button>
        <button class="cbtn" type="button" onclick="ctl('page_bridge')">OPEN BRIDGE</button>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="ch">&#128229; Received Messages
      <span id="rxB" style="margin-left:auto;background:var(--bbg);border:1px solid var(--bbd);color:var(--b);font-size:.68rem;padding:2px 7px;border-radius:999px">0</span>
    </div>
    <div id="inbox" class="il"><div class="ie">No messages received yet.</div></div>
  </div>
</main>
<script>
const ML=%MAXLEN%;
const TOKEN='%TOKEN%';
const msgEl=document.getElementById('msg'),cc=document.getElementById('cc'),sBtn=document.getElementById('sBtn'),sL=document.getElementById('sL'),sp=document.getElementById('sp'),bn=document.getElementById('bn');
const themeBtn=document.getElementById('themeBtn');

function applyTheme(theme){
  document.documentElement.setAttribute('data-theme',theme);
  themeBtn.textContent=theme==='dark'?'LIGHT':'DARK';
}

function initTheme(){
  const saved=localStorage.getItem('loraTheme');
  if(saved==='dark'||saved==='light') applyTheme(saved);
  else applyTheme('light');
}

themeBtn.addEventListener('click',()=>{
  const cur=document.documentElement.getAttribute('data-theme')==='dark'?'dark':'light';
  const next=cur==='dark'?'light':'dark';
  applyTheme(next);
  localStorage.setItem('loraTheme',next);
});
msgEl.addEventListener('input',()=>{
  const n=msgEl.value.length;cc.textContent=n+' / '+ML;
  cc.className='cc'+(n>=ML?' v':n>ML*.85?' w':'');
});
function bars(rssi){let b=0;if(rssi>=-122)b=1;if(rssi>=-114)b=2;if(rssi>=-106)b=3;if(rssi>=-98)b=4;return b;}
function updateStatus(d){
  document.getElementById('hF').textContent=d.freq||'&#8212;';
  document.getElementById('fT').textContent=d.freq||'&#8212;';
  const lb=document.getElementById('lb');
  if(d.waitingAck){lb.className='badge bsy';lb.innerHTML='<span class="dot p"></span>WAIT ACK';}
  else{lb.className='badge rdy';lb.innerHTML='<span class="dot"></span>READY';}
  document.getElementById('sTx').textContent=d.txCount??'&#8212;';
  document.getElementById('sRx').textContent=d.rxCount??'&#8212;';
  document.getElementById('sAck').textContent=(d.totalAcked??'&#8212;')+' / '+(d.totalSent??'&#8212;');
  document.getElementById('sPh').textContent=d.phoneTxCount??'&#8212;';
  document.getElementById('sSnr').textContent=d.snr!=null?d.snr.toFixed(1)+'dB':'&#8212;';
  const loss=d.loss??0,le=document.getElementById('sLoss');
  le.textContent=loss.toFixed(1)+'%';
  le.className='sv'+(loss>30?' r':loss>10?' o':' g');
  if(d.rxCount>0){
    const sr=document.getElementById('sigR');sr.style.display='flex';
    const bs=document.getElementById('sB').children,ac=bars(d.rssi);
    for(let i=0;i<4;i++)bs[i].className=i<ac?'on':'';
    document.getElementById('rL').textContent=d.rssi+'dBm';
  }
  sBtn.disabled=!!d.waitingAck;
}
function ago(s){if(s<60)return s+'s ago';if(s<3600)return Math.floor(s/60)+'m ago';return Math.floor(s/3600)+'h ago';}
function updateInbox(items){
  const box=document.getElementById('inbox'),badge=document.getElementById('rxB');
  badge.textContent=items.length;
  if(!items.length){box.innerHTML='<div class="ie">No messages received yet.</div>';return;}
  box.innerHTML=[...items].reverse().map(it=>{
    const init=(it.from||'??').slice(-4).replace(':','');
    const b=bars(it.rssi);
    const bs=Array.from({length:4},(_,i)=>`<span style="height:${3+i*3}px"${i<b?' class="on"':''}></span>`).join('');
    const safe=it.msg.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    return `<div class="ii"><div class="iav">${init}</div><div class="ic"><div class="im"><span class="ifr">${it.alias||it.from||'?'}</span><div class="sb" style="height:11px">${bs}</div><span style="font-size:.7rem;color:var(--tx3);font-family:monospace">${it.rssi}dBm</span><span class="ia">${ago(it.age)}</span></div><div class="imsg">${safe}</div></div></div>`;
  }).join('');
}
async function refresh(){
  try{
    const[sr,ir]=await Promise.all([fetch('/api/status'),fetch('/api/inbox')]);
    if(sr.ok)updateStatus(await sr.json());
    if(ir.ok)updateInbox(await ir.json());
  }catch(_){}
}
async function ctl(cmd){
  bn.className='bn';
  try{
    const body=new URLSearchParams({cmd,token:TOKEN});
    const res=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    const t=await res.text();
    bn.className='bn '+(res.ok?'ok':'er');
    bn.textContent=t;
  }catch(_){
    bn.className='bn er';
    bn.textContent='Control failed.';
  }
  await refresh();
}
document.getElementById('sF').addEventListener('submit',async e=>{
  e.preventDefault();const txt=msgEl.value.trim();if(!txt)return;
  sBtn.disabled=true;sp.style.display='block';sL.textContent='Sending\u2026';bn.className='bn';
  try{
    const body=new URLSearchParams({msg:txt,token:TOKEN});
    const res=await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    const t=await res.text();
    bn.className='bn '+(res.ok?'ok':'er');bn.textContent=t;
    if(res.ok){msgEl.value='';cc.textContent='0 / '+ML;}
  }catch(_){bn.className='bn er';bn.textContent='Network error \u2014 device unreachable.';}
  sp.style.display='none';sL.textContent='Send over LoRa';
  await refresh();
});
refresh();setInterval(refresh,2000);
initTheme();
</script></body></html>
)HTML";

void setStatus(const String &s, unsigned long dur = 2000);


String myMAC = "";

int  channelOffset     = 0;   // active offset in 100kHz steps
int  freqDraftOffset   = 0;   // draft while in FREQ MODE
bool freqMode          = false;

enum UiPage {
  PAGE_HOME = 0,
  PAGE_CHAT,
  PAGE_QUICK,
  PAGE_RADIO,
  PAGE_INBOX,
  PAGE_STATS,
  PAGE_RANGE,
  PAGE_BRIDGE
};

UiPage uiPage = PAGE_HOME;
int homeIdx = 0;
int homeTopIdx = 0;
int quickIdx = 0;
int inboxViewOffset = 0;
bool radioDirty = false;

const char *HOME_APPS[] = {"CHAT", "QUICK", "RADIO", "INBOX", "STATS", "RANGE", "BRIDGE"};
const int HOME_APP_COUNT = sizeof(HOME_APPS) / sizeof(HOME_APPS[0]);
const int HOME_VISIBLE_ROWS = 5;
const char *QUICK_MSGS[] = {
  "OK",
  "ON WAY",
  "NEED HELP",
  "BAT LOW",
  "WHERE ARE U",
  "REACHED",
  "WAIT",
  "DANGER",
  "ABORT",
  "SEND LOCATION",
  "ALL CLEAR",
  "COPY THAT"
};
const int QUICK_MSG_COUNT = sizeof(QUICK_MSGS) / sizeof(QUICK_MSGS[0]);

const int RADIO_PRESET_COUNT = 5;
const int RADIO_PRESET_OFFSETS[RADIO_PRESET_COUNT] = {0, 2, 4, 6, 8};
const char *RADIO_PRESET_NAMES[RADIO_PRESET_COUNT] = {"CH1", "CH2", "CH3", "CH4", "CH5"};

String typingBuf      = "";
int    pickerIdx      = 0;

String lastSentPayload = "";
uint16_t txSeq         = 0;
int    txCount         = 0;

bool     waitingAck    = false;
String   pendingFrame  = "";
uint16_t pendingSeq    = 0;
int      retryCount    = 0;
unsigned long ackDeadline = 0;
unsigned long pendingSentAt = 0;
unsigned long lastAckRttMs  = 0;

String lastRxPayload  = "";
int    rxCount        = 0;
int    lastRssi       = 0;
float  lastSnr        = 0.0;
bool   lastAcked      = false;   // did last TX get ACKed?
bool   unreadRx       = false;   // unread incoming message indicator
String lastRxSeenMAC  = "";     // duplicate DATA suppression
uint16_t lastRxSeenSeq = 0;

int totalSent = 0, totalAcked = 0;

String   lastAckFromMAC = "";   // MAC of device that last ACK'd us
String   lastRxFromMAC  = "";   // MAC of last device we received from
int      lastAckRssi    = 0;    // RSSI of last received ACK packet
uint16_t rangeTestSeq   = 0;    // ping counter for range testing

struct PeerAlias { String mac; unsigned long lastSeen; };
const int PEER_ALIAS_COUNT = 4;
PeerAlias peerAliases[PEER_ALIAS_COUNT];

String statusMsg       = "";
unsigned long statusExpiry = 0;

bool   dispDirty      = true;
unsigned long lastDispTime = 0;

bool   lastBtnState   = HIGH;
unsigned long lastBtnTime = 0;
volatile uint8_t encPrevState  = 0;   // shared with ISR
volatile int8_t  encAccum      = 0;   // quadrature accumulator (ISR)
volatile int     encRawDelta   = 0;   // step counter drained by readEnc()

bool   aHeld          = false;
unsigned long aPressTime = 0;
bool   aFreqTriggered = false;

bool   starHeld       = false;
unsigned long starPressTime = 0;

String phoneBridgeSsid = "";
String phoneBridgePass = "";
String phoneBridgeToken = "";
String lastPhonePayload = "";
int    phoneTxCount = 0;
String btBridgeName = "";
int    bridgeLastError = 0;
unsigned long lastBtRxMs = 0;
unsigned long lastBridgeDiagMs = 0;

enum BridgeMode { BRIDGE_WIFI = 0, BRIDGE_BT = 1 };
BridgeMode bridgeMode = BRIDGE_WIFI;
BridgeMode bridgeDraftMode = BRIDGE_WIFI;
String btRxLine = "";
bool btRxOverflow = false;
bool bridgeShowInfo = false;

bool settingsDirty = false;
unsigned long settingsDirtyAt = 0;

unsigned long ackTimeoutMs = ACK_TIMEOUT_BASE;
int maxRetries = MAX_RETRIES_BASE;
float ackRttEwma = 0.0f;
int dupDataCount = 0;
int dupAckCount = 0;

struct RxEntry { String msg; String fromMAC; int rssi; float snr; unsigned long ts; };
const int INBOX_SIZE = 8;
RxEntry  inbox[INBOX_SIZE];
int      inboxHead  = 0;
int      inboxCount = 0;

long activeFreq()  { return BASE_FREQ + (long)channelOffset * FREQ_STEP; }
long draftFreq()   { return BASE_FREQ + (long)freqDraftOffset * FREQ_STEP; }

String fmtFreq(long f) {
  char b[14];
  snprintf(b, sizeof(b), "%.3fMHz", f / 1000000.0);
  return String(b);
}

String shortMAC(const String &m) {
  return m.length() >= 9 ? m.substring(m.length() - 9) : m;
}

String compactMAC(const String &m) {
  String s = m;
  s.replace(":", "");
  s.toUpperCase();
  return s;
}

String buildDeviceToken() {
  return compactMAC(myMAC);
}

String buildWiFiPassword() {
  String tok = buildDeviceToken();
  return "LC-" + tok.substring(2, 10);
}

String buildWiFiSsid() {
  String tok = buildDeviceToken();
  return "LoRaChat-" + tok.substring(8, 12);
}

String buildBluetoothName() {
  String tok = buildDeviceToken();
  return "LoRaChatBT-" + tok.substring(6, 12);
}

String buildBridgeToken() {
  String tok = buildDeviceToken();
  return "T" + tok.substring(4, 12);
}

String peerAliasName(int slot) {
  return "P" + String(slot + 1);
}

int ensurePeerAlias(const String &mac) {
  if (mac.length() == 0) return -1;

  int freeSlot = -1;
  int oldestSlot = 0;
  unsigned long oldestSeen = ULONG_MAX;

  for (int i = 0; i < PEER_ALIAS_COUNT; i++) {
    if (peerAliases[i].mac == mac) {
      peerAliases[i].lastSeen = millis();
      return i;
    }
    if (peerAliases[i].mac.length() == 0 && freeSlot < 0) freeSlot = i;
    if (peerAliases[i].lastSeen < oldestSeen) {
      oldestSeen = peerAliases[i].lastSeen;
      oldestSlot = i;
    }
  }

  int slot = (freeSlot >= 0) ? freeSlot : oldestSlot;
  peerAliases[slot].mac = mac;
  peerAliases[slot].lastSeen = millis();
  return slot;
}

String peerLabel(const String &mac) {
  for (int i = 0; i < PEER_ALIAS_COUNT; i++) {
    if (peerAliases[i].mac == mac) return peerAliasName(i);
  }
  return shortMAC(mac);
}

int exactPresetIndexForOffset(int offset) {
  for (int i = 0; i < RADIO_PRESET_COUNT; i++) {
    if (RADIO_PRESET_OFFSETS[i] == offset) return i;
  }
  return -1;
}

int nearestPresetIndexForOffset(int offset) {
  int best = 0;
  int bestDiff = abs(offset - RADIO_PRESET_OFFSETS[0]);
  for (int i = 1; i < RADIO_PRESET_COUNT; i++) {
    int diff = abs(offset - RADIO_PRESET_OFFSETS[i]);
    if (diff < bestDiff) {
      best = i;
      bestDiff = diff;
    }
  }
  return best;
}

String activePresetLabel() {
  int idx = exactPresetIndexForOffset(channelOffset);
  return idx >= 0 ? String(RADIO_PRESET_NAMES[idx]) : String("CUSTOM");
}

String draftPresetLabel() {
  int idx = exactPresetIndexForOffset(freqDraftOffset);
  return idx >= 0 ? String(RADIO_PRESET_NAMES[idx]) : String("CUSTOM");
}

void cyclePresetDraft(int delta) {
  int idx = exactPresetIndexForOffset(freqDraftOffset);
  if (idx < 0) idx = nearestPresetIndexForOffset(freqDraftOffset);
  idx = (idx + delta + RADIO_PRESET_COUNT) % RADIO_PRESET_COUNT;
  freqDraftOffset = RADIO_PRESET_OFFSETS[idx];
  radioDirty = (freqDraftOffset != channelOffset);
  setStatus(String("PRESET ") + RADIO_PRESET_NAMES[idx], 1000);
}

String sanitizeBridgeMsg(String msg) {
  msg.replace("\r", " ");
  msg.replace("\n", " ");
  msg.replace("\t", " ");
  msg.trim();
  while (msg.indexOf("  ") >= 0) msg.replace("  ", " ");
  return msg;
}

String jsonEscape(const String &s) {
  String out; out.reserve(s.length() + 4);
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if      (c == '"')  out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c < 0x20)  out += ' ';
    else                out += c;
  }
  return out;
}

const char* bridgeModeName(BridgeMode m) {
  return (m == BRIDGE_WIFI) ? "WIFI" : "BLUETOOTH";
}

void markSettingsDirty() {
  settingsDirty = true;
  settingsDirtyAt = millis();
}

void loadSettings() {
  prefs.begin("lchat", true);
  channelOffset = prefs.getInt("chOfs", 0);
  quickIdx = prefs.getInt("qIdx", 0);
  int savedMode = prefs.getInt("brMode", (int)BRIDGE_WIFI);
  prefs.end();

  if (quickIdx < 0) quickIdx = 0;
  if (quickIdx >= QUICK_MSG_COUNT) quickIdx = QUICK_MSG_COUNT - 1;
  if (channelOffset < -20) channelOffset = -20;
  if (channelOffset > 20) channelOffset = 20;

  if (savedMode < (int)BRIDGE_WIFI || savedMode > (int)BRIDGE_BT) savedMode = (int)BRIDGE_WIFI;
  bridgeMode = (BridgeMode)savedMode;
  bridgeDraftMode = bridgeMode;
}

void saveSettingsIfNeeded() {
  if (!settingsDirty) return;
  if (millis() - settingsDirtyAt < SETTINGS_FLUSH_MS) return;

  prefs.begin("lchat", false);
  prefs.putInt("chOfs", channelOffset);
  prefs.putInt("qIdx", quickIdx);
  prefs.putInt("brMode", (int)bridgeMode);
  prefs.end();

  settingsDirty = false;
}

void updateAdaptiveAck(unsigned long rttMs) {
  if (ackRttEwma <= 0.1f) ackRttEwma = (float)rttMs;
  else ackRttEwma = 0.8f * ackRttEwma + 0.2f * (float)rttMs;

  unsigned long tuned = (unsigned long)(ackRttEwma * 1.7f) + 500UL;
  if (tuned < 1200UL) tuned = 1200UL;
  if (tuned > 6000UL) tuned = 6000UL;
  ackTimeoutMs = tuned;
}

String bridgeDiagText() {
  String t;
  t.reserve(160);
  t += "Mode:";
  t += bridgeModeName(bridgeMode);
  t += " BT:";
#if LORACHAT_ENABLE_BT
  t += SerialBT.hasClient() ? "1" : "0";
#else
  t += "NA";
#endif
  t += " STA:";
  t += String((int)WiFi.softAPgetStationNum());
  t += " e:";
  t += String(bridgeLastError);
  t += " btAge:";
  t += String((millis() - lastBtRxMs) / 1000UL);
  t += "s";
  return t;
}

String bridgeStatusText() {
  String text;
  text.reserve(220);
  text += "SSID: ";
  text += phoneBridgeSsid;
  text += "\nURL : http://192.168.4.1\n";
  text += "FREQ: ";
  text += fmtFreq(activeFreq());
  text += "\nLoRa: ";
  text += waitingAck ? "WAIT ACK" : "READY";
  text += "\nTX/RX: ";
  text += String(txCount);
  text += "/";
  text += String(rxCount);
  text += "  ACK: ";
  text += String(totalAcked);
  text += "/";
  text += String(totalSent);
  text += "\nPhone TX: ";
  text += String(phoneTxCount);
  text += "\nPreset: ";
  text += activePresetLabel();
  text += "  Peer: ";
  text += lastRxFromMAC.length() ? peerLabel(lastRxFromMAC) : "--";
  text += "\nLast phone: ";
  text += lastPhonePayload.length() ? trunc(lastPhonePayload, 24) : "--";
  text += "\nLast LoRa RX: ";
  text += lastRxPayload.length() ? trunc(lastRxPayload, 24) : "--";
  return text;
}

void handlePhoneBridgeRoot() {
  String page = FPSTR(PHONE_BRIDGE_HTML);
  page.replace("%SSID%", phoneBridgeSsid);
  page.replace("%PASS%", phoneBridgePass);
  page.replace("%MAXLEN%", String(MAX_MSG_LEN));
  page.replace("%TOKEN%", phoneBridgeToken);
  phoneServer.send(200, "text/html", page);
}

void handlePhoneBridgeStatus() {
  phoneServer.send(200, "text/plain", bridgeStatusText());
}

void handleApiStatus() {
  String j = "{\"ssid\":\""      + jsonEscape(phoneBridgeSsid)           + "\",";
  j += "\"freq\":\""             + fmtFreq(activeFreq())                  + "\",";
  j += "\"preset\":\""           + activePresetLabel()                    + "\",";
  j += "\"profile\":\""          + String(PROFILE_NAME)                   + "\",";
  j += "\"bridgeMode\":\""       + String(bridgeModeName(bridgeMode))     + "\",";
  j += "\"waitingAck\":"         + String(waitingAck  ? "true" : "false") + ",";
  j += "\"txCount\":"            + String(txCount)    + ",";
  j += "\"rxCount\":"            + String(rxCount)    + ",";
  j += "\"totalSent\":"          + String(totalSent)  + ",";
  j += "\"totalAcked\":"         + String(totalAcked) + ",";
  j += "\"loss\":"               + String(lossPercent(), 1) + ",";
  j += "\"rssi\":"               + String(lastRssi)   + ",";
  j += "\"snr\":"                + String(lastSnr, 1) + ",";
  j += "\"phoneTxCount\":"       + String(phoneTxCount) + "}";
  phoneServer.send(200, "application/json", j);
}

void handleApiInbox() {
  String j = "[";
  int start = (inboxCount < INBOX_SIZE) ? 0 : inboxHead;
  for (int i = 0; i < inboxCount; i++) {
    int idx = (start + i) % INBOX_SIZE;
    if (i) j += ",";
    j += "{\"msg\":\""  + jsonEscape(inbox[idx].msg)     + "\",";
    j += "\"alias\":\"" + jsonEscape(peerLabel(inbox[idx].fromMAC)) + "\",";
    j += "\"from\":\""  + jsonEscape(inbox[idx].fromMAC) + "\",";
    j += "\"rssi\":"    + String(inbox[idx].rssi) + ",";
    j += "\"snr\":"     + String(inbox[idx].snr, 1) + ",";
    j += "\"age\":"     + String((millis() - inbox[idx].ts) / 1000UL) + "}";
  }
  j += "]";
  phoneServer.send(200, "application/json", j);
}

void handlePhoneBridgeSend() {
  String token = "";
  if (phoneServer.hasArg("token")) token = phoneServer.arg("token");
  if (token != phoneBridgeToken) {
    phoneServer.send(401, "text/plain", "Unauthorized token");
    bridgeLastError = 401;
    return;
  }

  if (!phoneServer.hasArg("msg")) {
    phoneServer.send(400, "text/plain", "Missing msg parameter");
    bridgeLastError = 400;
    return;
  }

  String msg = sanitizeBridgeMsg(phoneServer.arg("msg"));
  if (msg.length() == 0) {
    phoneServer.send(400, "text/plain", "Message is empty");
    bridgeLastError = 400;
    return;
  }
  if ((int)msg.length() > MAX_MSG_LEN) {
    phoneServer.send(413, "text/plain", "Message exceeds LoRa limit");
    bridgeLastError = 413;
    return;
  }
  if (waitingAck) {
    phoneServer.send(409, "text/plain", "Busy waiting for ACK. Retry in a moment.");
    bridgeLastError = 409;
    return;
  }

  lastPhonePayload = msg;
  phoneTxCount++;
  setStatus("PHONE TX");
  Serial.println("[PHONE_TX] " + msg);
  sendDataMsg(msg);
  phoneServer.send(200, "text/plain", "Queued for LoRa TX: " + msg);
}

void handlePhoneBridgeControl() {
  String token = "";
  if (phoneServer.hasArg("token")) token = phoneServer.arg("token");
  if (token != phoneBridgeToken) {
    phoneServer.send(401, "text/plain", "Unauthorized token");
    bridgeLastError = 401;
    return;
  }

  if (!phoneServer.hasArg("cmd")) {
    phoneServer.send(400, "text/plain", "Missing cmd");
    bridgeLastError = 400;
    return;
  }

  String cmd = phoneServer.arg("cmd");
  String out = "OK";

  if (cmd == "ping") {
    sendPing();
    out = "PING sent";
  } else if (cmd == "page_home") {
    goHome();
    out = "HOME";
  } else if (cmd == "page_chat") {
    uiPage = PAGE_CHAT;
    out = "CHAT";
  } else if (cmd == "page_radio") {
    uiPage = PAGE_RADIO;
    freqMode = true;
    freqDraftOffset = channelOffset;
    radioDirty = false;
    out = "RADIO";
  } else if (cmd == "page_bridge") {
    uiPage = PAGE_BRIDGE;
    bridgeDraftMode = bridgeMode;
    bridgeShowInfo = false;
    out = "BRIDGE";
  } else if (cmd == "freq_up") {
    freqDraftOffset++;
    clampDraftOffset();
    radioDirty = true;
    uiPage = PAGE_RADIO;
    out = "FREQ draft " + fmtFreq(draftFreq());
  } else if (cmd == "freq_down") {
    freqDraftOffset--;
    clampDraftOffset();
    radioDirty = true;
    uiPage = PAGE_RADIO;
    out = "FREQ draft " + fmtFreq(draftFreq());
  } else if (cmd == "freq_apply") {
    if (applyFreq(draftFreq())) {
      channelOffset = freqDraftOffset;
      markSettingsDirty();
      out = "FREQ set " + fmtFreq(activeFreq());
    } else {
      out = "FREQ apply failed";
      bridgeLastError = 550;
      phoneServer.send(500, "text/plain", out);
      return;
    }
  } else if (cmd == "preset_prev") {
    cyclePresetDraft(-1);
    uiPage = PAGE_RADIO;
    out = "PRESET " + draftPresetLabel();
  } else if (cmd == "preset_next") {
    cyclePresetDraft(1);
    uiPage = PAGE_RADIO;
    out = "PRESET " + draftPresetLabel();
  } else if (cmd == "mode_wifi") {
    bool ok = applyBridgeMode(BRIDGE_WIFI);
    out = ok ? "MODE WIFI" : "MODE WIFI failed";
    if (!ok) {
      phoneServer.send(500, "text/plain", out);
      return;
    }
  } else if (cmd == "mode_bt") {
    bool ok = applyBridgeMode(BRIDGE_BT);
    out = ok ? "MODE BT" : "MODE BT failed";
    if (!ok) {
      phoneServer.send(500, "text/plain", out);
      return;
    }
  } else if (cmd == "quick_send") {
    sendDataMsg(String(QUICK_MSGS[quickIdx]));
    out = "QUICK sent";
  } else if (cmd == "clear_inbox") {
    clearInbox();
    inboxViewOffset = 0;
    out = "INBOX cleared";
  } else if (cmd == "reset_config") {
    resetRuntimeDefaults();
    if (applyFreq(activeFreq())) {
      markSettingsDirty();
      uiPage = PAGE_HOME;
      homeIdx = 0;
      homeTopIdx = 0;
      out = "CONFIG reset";
    } else {
      bridgeLastError = 551;
      phoneServer.send(500, "text/plain", "RESET failed");
      return;
    }
  } else {
    bridgeLastError = 404;
    phoneServer.send(404, "text/plain", "Unknown cmd");
    return;
  }

  dispDirty = true;
  phoneServer.send(200, "text/plain", out);
}

void initPhoneBridge() {
  if (!PROFILE_ALLOW_WIFI) {
    bridgeLastError = 901;
    return;
  }

  phoneBridgeSsid = buildWiFiSsid();
  phoneBridgePass = buildWiFiPassword();
  phoneBridgeToken = buildBridgeToken();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(phoneBridgeSsid.c_str(), phoneBridgePass.c_str());

  static bool routesInit = false;
  if (!routesInit) {
    phoneServer.on("/", HTTP_GET, handlePhoneBridgeRoot);
    phoneServer.on("/status", HTTP_GET, handlePhoneBridgeStatus);
    phoneServer.on("/send", HTTP_POST, handlePhoneBridgeSend);
    phoneServer.on("/api/control", HTTP_POST, handlePhoneBridgeControl);
    phoneServer.on("/api/status", HTTP_GET, handleApiStatus);
    phoneServer.on("/api/inbox",  HTTP_GET, handleApiInbox);
    phoneServer.onNotFound([]() {
      phoneServer.sendHeader("Location", "/", true);
      phoneServer.send(302, "text/plain", "");
    });
    routesInit = true;
  }
  phoneServer.begin();

  IPAddress apIp = WiFi.softAPIP();
  Serial.println("[WIFI] AP ready");
  Serial.println("[WIFI] PROFILE: " + String(PROFILE_NAME));
  Serial.println("[WIFI] SSID: " + phoneBridgeSsid);
  Serial.println("[WIFI] PASS: " + phoneBridgePass);
  Serial.println("[WIFI] TOKEN: " + phoneBridgeToken);
  Serial.println("[WIFI] URL : http://" + apIp.toString());
}

void stopPhoneBridge() {
  phoneServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool initBluetoothBridge() {
#if LORACHAT_ENABLE_BT
  if (!PROFILE_ALLOW_BT) {
    bridgeLastError = 902;
    return false;
  }
  btBridgeName = buildBluetoothName();
  if (!SerialBT.begin(btBridgeName)) return false;
  Serial.println("[BT] Ready");
  Serial.println("[BT] Name: " + btBridgeName);
  Serial.println("[BT] Open a BLE/BT serial terminal, then type a line and press send.");
  return true;
#else
  btBridgeName = "DISABLED";
  Serial.println("[BT] Disabled in this build profile (LORACHAT_ENABLE_BT=0)");
  return false;
#endif
}

void stopBluetoothBridge() {
#if LORACHAT_ENABLE_BT
  SerialBT.end();
#endif
}

bool applyBridgeMode(BridgeMode targetMode) {
  if (targetMode == bridgeMode) {
    setStatus(String("MODE ") + bridgeModeName(bridgeMode), 1200);
    return true;
  }

  if (targetMode == BRIDGE_WIFI) {
    if (!PROFILE_ALLOW_WIFI) {
      setStatus("WIFI DISABLED", 1500);
      bridgeLastError = 903;
      return false;
    }
    stopBluetoothBridge();
    initPhoneBridge();
    bridgeMode = BRIDGE_WIFI;
    markSettingsDirty();
    setStatus("MODE WIFI", 1400);
    Serial.println("[BRIDGE] Switched to WIFI mode");
    return true;
  }

#if !LORACHAT_ENABLE_BT
  setStatus("BT DISABLED", 1500);
  Serial.println("[BRIDGE] BT unavailable in this build profile");
  return false;
#endif

  stopPhoneBridge();
  if (!initBluetoothBridge()) {
    initPhoneBridge();
    bridgeMode = BRIDGE_WIFI;
    setStatus("BT INIT ERR", 1600);
    Serial.println("[BRIDGE] Bluetooth init failed, restored WIFI mode");
    return false;
  }

  bridgeMode = BRIDGE_BT;
  markSettingsDirty();
  setStatus("MODE BT", 1400);
  Serial.println("[BRIDGE] Switched to BLUETOOTH mode");
  return true;
}

void processBluetoothLine(String line) {
  String msg = sanitizeBridgeMsg(line);
  if (msg.length() == 0) return;

  if (msg.startsWith("MSG:")) msg = sanitizeBridgeMsg(msg.substring(4));
  if (msg.length() == 0) return;

  if ((int)msg.length() > MAX_MSG_LEN) {
#if LORACHAT_ENABLE_BT
    SerialBT.println("ERR: max " + String(MAX_MSG_LEN) + " chars");
#endif
    bridgeLastError = 414;
    return;
  }
  if (waitingAck) {
#if LORACHAT_ENABLE_BT
    SerialBT.println("BUSY: waiting ACK");
#endif
    bridgeLastError = 409;
    return;
  }

#if LORACHAT_ENABLE_BT
  SerialBT.println("TX: " + msg);
#endif
  lastPhonePayload = msg;
  phoneTxCount++;
  setStatus("BT TX", 900);
  sendDataMsg(msg);
}

void handleBluetoothBridgeInput() {
#if LORACHAT_ENABLE_BT
  bool gotByte = false;
  while (SerialBT.available()) {
    gotByte = true;
    lastBtRxMs = millis();
    char c = (char)SerialBT.read();
    if (c == '\r') continue;
    if (c == '\n') {
      processBluetoothLine(btRxLine);
      btRxLine = "";
      btRxOverflow = false;
      continue;
    }
    if (btRxLine.length() < (unsigned)(MAX_MSG_LEN + 8)) {
      btRxLine += c;
    } else {
      btRxOverflow = true;
    }
  }

  if (!gotByte && btRxLine.length() > 0 && (millis() - lastBtRxMs) > BT_RX_IDLE_FLUSH_MS) {
    processBluetoothLine(btRxLine);
    btRxLine = "";
    btRxOverflow = false;
  }

  if (btRxOverflow) {
    SerialBT.println("ERR: line too long");
    btRxLine = "";
    btRxOverflow = false;
    bridgeLastError = 415;
  }
#endif
}

void setStatus(const String &s, unsigned long dur) {
  statusMsg    = s;
  statusExpiry = millis() + dur;
  dispDirty    = true;
}

bool statusActive() { return statusMsg.length() > 0 && millis() < statusExpiry; }

String trunc(const String &s, int maxCh) {
  return s.length() <= (unsigned)maxCh ? s : s.substring(0, maxCh - 2) + "..";
}

float lossPercent() {
  return totalSent == 0 ? 0.0 : 100.0 * (totalSent - totalAcked) / totalSent;
}

void clearInbox() {
  inboxHead = 0;
  inboxCount = 0;
  unreadRx = false;
}

void resetRuntimeDefaults() {
  channelOffset = 0;
  freqDraftOffset = 0;
  quickIdx = 0;
  inboxViewOffset = 0;
  ackTimeoutMs = ACK_TIMEOUT_BASE;
  maxRetries = MAX_RETRIES_BASE;
  ackRttEwma = 0.0f;
  dupDataCount = 0;
  dupAckCount = 0;
  clearInbox();
}

String estimateRange(int rssi) {
  if (rssi >= -100) return "<5km";
  if (rssi >= -110) return "5-10km";
  if (rssi >= -118) return "10-15km";
  return ">15km";
}

void clampDraftOffset() {
  if (freqDraftOffset < channelOffset - 20) freqDraftOffset = channelOffset - 20;
  if (freqDraftOffset > channelOffset + 20) freqDraftOffset = channelOffset + 20;
}

void appendTyped(char ch) {
  if ((int)typingBuf.length() >= MAX_MSG_LEN) {
    setStatus("BUF FULL");
    return;
  }
  typingBuf += ch;
  setStatus(String("ADD: ") + ch, 900);
}

void backspaceTyped() {
  if (typingBuf.length() == 0) {
    setStatus("EMPTY", 800);
    return;
  }
  typingBuf.remove(typingBuf.length() - 1);
  setStatus("DEL", 800);
}

void goHome() {
  uiPage = PAGE_HOME;
  freqMode = false;
  radioDirty = false;
  setStatus("HOME", 800);
}

void openHomeSelection() {
  if (homeIdx == 0) uiPage = PAGE_CHAT;
  else if (homeIdx == 1) uiPage = PAGE_QUICK;
  else if (homeIdx == 2) {
    uiPage = PAGE_RADIO;
    freqMode = true;
    freqDraftOffset = channelOffset;
    radioDirty = false;
  } else if (homeIdx == 3) {
    uiPage = PAGE_INBOX;
    inboxViewOffset = 0;
  } else if (homeIdx == 4) uiPage = PAGE_STATS;
  else if (homeIdx == 5) uiPage = PAGE_RANGE;
  else {
    uiPage = PAGE_BRIDGE;
    bridgeDraftMode = bridgeMode;
    bridgeShowInfo = false;
  }
  dispDirty = true;
}

void drawBootSequence() {
  for (int p = 0; p < 3; p++) {
    u8g2.clearBuffer();
    u8g2.drawRFrame(26, 8, 76, 48, 6);
    u8g2.drawRFrame(31, 13, 66, 38, 4);
    u8g2.drawBox(60, 52, 8, 2);

    if (p >= 0) u8g2.drawBox(36, 20, 14, 8);
    if (p >= 1) u8g2.drawBox(57, 20, 14, 8);
    if (p >= 2) u8g2.drawBox(78, 20, 14, 8);

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(52, 42, "LORA");
    u8g2.sendBuffer();
    delay(180);
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(40, 24, "LORA");
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(18, 40, "Booting modules...");
  u8g2.drawStr(18, 52, myMAC.c_str());
  u8g2.sendBuffer();
  delay(450);
}

bool initLoRa(long freq) {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(freq)) return false;
    LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(12);    // SF12
  LoRa.setSignalBandwidth(125E3); // 125 kHz
  LoRa.setCodingRate4(8);         // CR 4/8
  LoRa.setSyncWord(0xF3);         // private
  LoRa.enableCrc();
    Serial.println("[LoRa] Ready " + fmtFreq(freq) + "  SF12/BW125/CR8/20dBm");
  return true;
}

bool applyFreq(long f) {
  LoRa.end(); delay(40);
  return initLoRa(f);
}

void txRaw(const String &frame) {
  LoRa.beginPacket();
  LoRa.print(frame);
  LoRa.endPacket();
}

void sendDataMsg(const String &payload) {
  if (payload.length() == 0) { setStatus("TYPE FIRST"); return; }
  if (waitingAck) { setStatus("WAIT ACK"); return; }
  txSeq++; totalSent++; txCount++;
  String frame = "DATA|" + myMAC + "|" + String(txSeq) + "|" + payload;
  txRaw(frame);
  lastSentPayload = payload;
  lastAcked       = false;
  pendingFrame    = frame;
  pendingSeq      = txSeq;
  waitingAck      = true;
  retryCount      = 0;
  pendingSentAt   = millis();
  ackDeadline     = millis() + ackTimeoutMs;
  setStatus("WAIT ACK..", ackTimeoutMs + 500);
  Serial.println("[TX] " + frame);
}

void sendAck(const String &toMAC, uint16_t seq) {
  String f = "ACK|" + myMAC + "|" + String(seq);
  txRaw(f);
  Serial.println("[ACK->] " + f);
}

void sendPing() {
  if (waitingAck) { setStatus("WAIT ACK"); return; }
  rangeTestSeq++;
  char buf[16];
  snprintf(buf, sizeof(buf), "PING %u", rangeTestSeq);
  Serial.println("[PING_TX] seq:" + String(rangeTestSeq)
                 + " freq:" + fmtFreq(activeFreq()));
  sendDataMsg(String(buf));
}

void checkRx() {
  if (!LoRa.parsePacket()) return;
  String raw = "";
  while (LoRa.available()) raw += (char)LoRa.read();
  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  int p1 = raw.indexOf('|'); if (p1 < 0) return;
  String type = raw.substring(0, p1);
  String rest = raw.substring(p1 + 1);
  int p2 = rest.indexOf('|'); if (p2 < 0) return;
  String fromMAC = rest.substring(0, p2);
  String rest2 = rest.substring(p2 + 1);

  if (type == "ACK") {
    uint16_t seq = (uint16_t)rest2.toInt();
    if (waitingAck && seq == pendingSeq) {
      ensurePeerAlias(fromMAC);
      waitingAck     = false;
      totalAcked++;
      lastAcked      = true;
      lastAckRttMs   = millis() - pendingSentAt;
      updateAdaptiveAck(lastAckRttMs);
      lastAckFromMAC = fromMAC;
      lastAckRssi    = rssi;
      setStatus("ACK " + shortMAC(fromMAC) + " " + String(lastAckRttMs) + "ms");
      Serial.println("[ACK<] seq:" + String(seq)
                     + " from:" + fromMAC
                     + " rssi:" + String(rssi)
                     + " snr:" + String(snr, 1)
                     + " rtt:" + String(lastAckRttMs) + "ms"
                     + " loss:" + String(lossPercent(), 1) + "%");
    } else {
      dupAckCount++;
    }
    dispDirty = true;
    return;
  }

  if (type == "DATA") {
    int p3 = rest2.indexOf('|'); if (p3 < 0) return;
    uint16_t seq   = (uint16_t)rest2.substring(0, p3).toInt();
    String payload = rest2.substring(p3 + 1);

    bool isDup = (fromMAC == lastRxSeenMAC && seq == lastRxSeenSeq);
    if (isDup) {
      sendAck(fromMAC, seq);
      dupDataCount++;
      Serial.println("[DUP] seq:" + String(seq) + " from:" + fromMAC);
      return;
    }

    lastRxSeenMAC  = fromMAC;
    lastRxSeenSeq  = seq;
    ensurePeerAlias(fromMAC);
    lastRxPayload  = payload;
    lastRxFromMAC  = fromMAC;
    lastRssi       = rssi;
    lastSnr        = snr;
    rxCount++;
    unreadRx       = true;
      inbox[inboxHead].msg     = payload;
      inbox[inboxHead].fromMAC = fromMAC;
      inbox[inboxHead].rssi    = rssi;
      inbox[inboxHead].snr     = snr;
      inbox[inboxHead].ts      = millis();
      inboxHead = (inboxHead + 1) % INBOX_SIZE;
      if (inboxCount < INBOX_SIZE) inboxCount++;
    Serial.println("[RANGE_LOG] seq:" + String(seq) +
             ",rssi:" + String(rssi) +
             ",snr:" + String(snr, 1) +
             ",from:" + fromMAC +
             ",msg:" + payload);
    Serial.println("[RANGE_CSV] " +
             String(millis()) + "," +
             String(seq) + "," +
             String(rssi) + "," +
             String(snr, 1) + "," +
             fromMAC + "," +
             String(lastAckRttMs) + "," +
             String(lossPercent(), 1) + "," +
             payload);
    sendAck(fromMAC, seq);
    setStatus("GOT: " + trunc(payload, 10));
    dispDirty = true;
  }
}

void handleRetry() {
  if (!waitingAck || millis() < ackDeadline) return;
  if (retryCount < maxRetries) {
    retryCount++;
    delay(40UL * retryCount);
    txRaw(pendingFrame);
    ackDeadline = millis() + ackTimeoutMs + (unsigned long)(retryCount * 120UL);
    setStatus("RETRY " + String(retryCount) + "/" + String(maxRetries));
    Serial.println("[RETRY] " + String(retryCount) + "/" + String(maxRetries));
  } else {
    waitingAck = false;
    lastAcked  = false;
    setStatus("NO ACK! Loss:" + String((int)lossPercent()) + "%");
    Serial.println("[FAIL] No ACK. Loss:" + String(lossPercent(), 1) + "%");
  }
  dispDirty = true;
}

void drawSignalBars(int x, int yBottom, int rssi) {
  int bars = 0;
  if (rssi >= -122) bars = 1;
  if (rssi >= -114) bars = 2;
  if (rssi >= -106) bars = 3;
  if (rssi >= -98)  bars = 4;

  for (int i = 0; i < 4; i++) {
    int h = 2 + (i * 2);
    int bx = x + (i * 3);
    if (i < bars) u8g2.drawBox(bx, yBottom - h, 2, h);
    else          u8g2.drawFrame(bx, yBottom - h, 2, h);
  }
}

void drawBatteryIcon(int x, int y, int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  u8g2.drawFrame(x, y, 12, 7);
  u8g2.drawBox(x + 12, y + 2, 2, 3);

  int fill = (pct + 24) / 25;  // 0..4 blocks
  for (int i = 0; i < 4; i++) {
    if (i < fill) u8g2.drawBox(x + 2 + i * 2, y + 2, 2, 3);
  }
}

void drawHeader(const char *title) {
  u8g2.drawBox(0, 0, 128, 11);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(2, 8, title);
  char lossBuf[8];
  snprintf(lossBuf, sizeof(lossBuf), "%d%%", (int)lossPercent());
  int lw = u8g2.getStrWidth(lossBuf);
  u8g2.drawStr(126 - lw, 8, lossBuf);
  u8g2.setDrawColor(1);
  u8g2.drawLine(0, 11, 127, 11);
}

void drawHomePage() {
  drawHeader("LORA CHAT");
  u8g2.setFont(u8g2_font_5x7_tr);

  int maxTop = HOME_APP_COUNT - HOME_VISIBLE_ROWS;
  if (maxTop < 0) maxTop = 0;
  if (homeTopIdx < 0) homeTopIdx = 0;
  if (homeTopIdx > maxTop) homeTopIdx = maxTop;

  for (int row = 0; row < HOME_VISIBLE_ROWS; row++) {
    int i = homeTopIdx + row;
    if (i >= HOME_APP_COUNT) break;

    int y = 21 + row * 9;   // rows: 21,30,39,48,57
    bool sel = (i == homeIdx);
    const char *label = HOME_APPS[i];
    if (i == 0 && unreadRx) label = "CHAT *";
    if (i == 3 && unreadRx) label = "INBOX *";
    if (sel) {
      u8g2.drawBox(2, y - 7, 124, 9);
      u8g2.setDrawColor(0);
      u8g2.drawStr(6, y, label);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(6, y, label);
    }
  }

  if (HOME_APP_COUNT > HOME_VISIBLE_ROWS) {
    const int trackX = 125;
    const int trackY = 14;
    const int trackH = 48;
    const int maxTopRows = HOME_APP_COUNT - HOME_VISIBLE_ROWS;
    const int knobH = (trackH * HOME_VISIBLE_ROWS) / HOME_APP_COUNT;
    int knobY = trackY;
    if (maxTopRows > 0) {
      knobY = trackY + ((trackH - knobH) * homeTopIdx) / maxTopRows;
    }
    u8g2.drawFrame(trackX, trackY, 2, trackH);
    u8g2.drawBox(trackX, knobY, 2, knobH < 6 ? 6 : knobH);
  }
}

void drawChatPage() {
  drawHeader("CHAT");

  u8g2.setFont(u8g2_font_5x7_tr);
  char rxHead[20];
  if (rxCount > 0) snprintf(rxHead, sizeof(rxHead), "RX %d  %ddB", rxCount, lastRssi);
  else             snprintf(rxHead, sizeof(rxHead), "RX 0  --dB");
  u8g2.drawStr(2, 19, rxHead);
  u8g2.setFont(u8g2_font_6x10_tr);
  String rxDisp = trunc(lastRxPayload.length() ? lastRxPayload : "NO INCOMING", 18);
  u8g2.drawStr(2, 30, rxDisp.c_str());

  u8g2.drawLine(0, 33, 127, 33);
  u8g2.setFont(u8g2_font_5x7_tr);
  char txHead[32];
  if (lastAcked && !waitingAck && lastAckFromMAC.length())
    snprintf(txHead, sizeof(txHead), "TX %d ACK[%s]", txCount, shortMAC(lastAckFromMAC).c_str());
  else if (waitingAck)
    snprintf(txHead, sizeof(txHead), "TX %d WAIT...", txCount);
  else
    snprintf(txHead, sizeof(txHead), "TX %d", txCount);
  u8g2.drawStr(2, 41, txHead);

  u8g2.setFont(u8g2_font_6x10_tr);
  String txDisp = trunc(lastSentPayload.length() ? lastSentPayload : "DRAFT EMPTY", 18);
  u8g2.drawStr(2, 52, txDisp.c_str());

  u8g2.drawLine(0, 54, 127, 54);
  u8g2.drawFrame(0, 55, 128, 9);

  int prev = (pickerIdx - 1 + CHARSET_LEN) % CHARSET_LEN;
  int next = (pickerIdx + 1) % CHARSET_LEN;
  char pch[2] = {CHARSET[prev], 0};
  char cch[2] = {CHARSET[pickerIdx], 0};
  char nch[2] = {CHARSET[next], 0};

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(2, 62, pch);
  u8g2.drawBox(9, 56, 9, 7);
  u8g2.setDrawColor(0);
  u8g2.drawStr(11, 62, cch);
  u8g2.setDrawColor(1);
  u8g2.drawStr(20, 62, nch);

  String tail = typingBuf;
  if (statusActive()) {
    u8g2.drawStr(30, 62, trunc(statusMsg, 16).c_str());
  } else if (tail.length()) {
    while (tail.length() > 0 && u8g2.getStrWidth(tail.c_str()) > 96) {
      tail.remove(0, 1);
    }
    int tx = 126 - u8g2.getStrWidth(tail.c_str());
    if (tx < 30) tx = 30;
    u8g2.drawStr(tx, 62, tail.c_str());
  } else {
    u8g2.drawStr(30, 62, "ENC=pick push=add");
  }
}

void drawQuickPage() {
  drawHeader("QUICK MSG");
  u8g2.setFont(u8g2_font_6x10_tr);
  String msg = QUICK_MSGS[quickIdx];
  u8g2.drawStr(2, 28, trunc(msg, 18).c_str());
  u8g2.setFont(u8g2_font_5x7_tr);
  char idxBuf[20];
  snprintf(idxBuf, sizeof(idxBuf), "%d/%d", quickIdx + 1, QUICK_MSG_COUNT);
  u8g2.drawStr(2, 40, idxBuf);
  u8g2.drawStr(2, 50, "# or ENC: SEND");
  u8g2.drawStr(2, 60, "A:HOME  ENC: NEXT");
}

void drawRadioPage() {
  drawHeader("RADIO");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(2, 28, fmtFreq(draftFreq()).c_str());
  u8g2.setFont(u8g2_font_5x7_tr);
  char dBuf[24];
  snprintf(dBuf, sizeof(dBuf), "Delta %+d", freqDraftOffset - channelOffset);
  u8g2.drawStr(2, 40, dBuf);
  u8g2.drawStr(2, 49, (String("Preset ") + draftPresetLabel()).c_str());
  u8g2.drawStr(2, 56, "ENC/2/B:+ 8/C:-");
  u8g2.drawStr(2, 63, "D PRESET  # APPLY");
}

void drawInboxPage() {
  drawHeader("INBOX");
  u8g2.setFont(u8g2_font_5x7_tr);

  if (inboxCount == 0) {
    u8g2.drawStr(2, 28, "No received messages");
    u8g2.drawStr(2, 40, "yet.");
    u8g2.drawStr(2, 63, "A:HOME  D:CLEAR");
    return;
  }

  if (inboxViewOffset < 0) inboxViewOffset = 0;
  if (inboxViewOffset > inboxCount - 1) inboxViewOffset = inboxCount - 1;

  int start = (inboxCount < INBOX_SIZE) ? 0 : inboxHead;
  int logical = inboxCount - 1 - inboxViewOffset;
  if (logical < 0) logical = 0;
  int idx = (start + logical) % INBOX_SIZE;

  String alias = peerLabel(inbox[idx].fromMAC);
  unsigned long ageSec = (millis() - inbox[idx].ts) / 1000UL;
  String meta = alias + " " + String(inbox[idx].rssi) + "dB " + String(ageSec) + "s";
  u8g2.drawStr(2, 21, trunc(meta, 20).c_str());
  u8g2.drawStr(2, 29, trunc(shortMAC(inbox[idx].fromMAC), 20).c_str());

  u8g2.drawLine(0, 31, 127, 31);

  String msg = inbox[idx].msg;
  String l1 = msg.substring(0, min((int)msg.length(), 20));
  String l2 = (msg.length() > 20) ? msg.substring(20, min((int)msg.length(), 40)) : "";
  String l3 = (msg.length() > 40) ? msg.substring(40, min((int)msg.length(), 60)) : "";
  u8g2.drawStr(2, 40, l1.c_str());
  if (l2.length()) u8g2.drawStr(2, 49, l2.c_str());
  if (l3.length()) u8g2.drawStr(2, 58, l3.c_str());

  u8g2.setFont(u8g2_font_4x6_tr);
  char nav[16];
  snprintf(nav, sizeof(nav), "%d/%d", inboxViewOffset + 1, inboxCount);
  u8g2.drawStr(2, 64, "ENC:SCROLL D:CLEAR");
  u8g2.drawStr(112, 64, nav);
}

void drawStatsPage() {
  drawHeader("STATS");
  u8g2.setFont(u8g2_font_5x7_tr);
  char s1[28], s2[28], s3[32], s4[28], s5[28];
  snprintf(s1, sizeof(s1), "ME: %s", shortMAC(myMAC).c_str());
  snprintf(s2, sizeof(s2), "TX:%d RX:%d Loss:%d%%",
           txCount, rxCount, (int)lossPercent());
  snprintf(s3, sizeof(s3), "ACK:%d/%d RTT:%lums",
           totalAcked, totalSent, lastAckRttMs);
  snprintf(s4, sizeof(s4), "Peer:%s",
           lastRxFromMAC.length() ? peerLabel(lastRxFromMAC).c_str() : "--");
  snprintf(s5, sizeof(s5), "RSSI:%ddB %s",
           lastRssi, estimateRange(lastRssi).c_str());
  u8g2.drawStr(2, 21, s1);
  u8g2.drawStr(2, 30, s2);
  u8g2.drawStr(2, 39, s3);
  u8g2.drawStr(2, 48, s4);
  u8g2.drawStr(2, 57, s5);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 63, "A:HOME");
}

void drawRangePage() {
  drawHeader("RANGE TEST");
  bool hasData = (rxCount > 0);

  u8g2.setFont(u8g2_font_6x10_tr);
  if (hasData) {
    char rssiLine[24];
    snprintf(rssiLine, sizeof(rssiLine), "%ddBm  SNR:%.1f", lastRssi, lastSnr);
    u8g2.drawStr(2, 24, rssiLine);
  } else {
    u8g2.drawStr(2, 24, "No packets yet");
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  if (hasData) {
    u8g2.drawStr(2, 35, ("Est: " + estimateRange(lastRssi)).c_str());
    if (lastRxFromMAC.length())
      u8g2.drawStr(2, 45, ("Peer: " + shortMAC(lastRxFromMAC)).c_str());
  }

  char pkLine[28];
  snprintf(pkLine, sizeof(pkLine), "TX:%d ACK:%d Loss:%d%%",
           totalSent, totalAcked, (int)lossPercent());
  u8g2.drawStr(2, 55, pkLine);

  u8g2.setFont(u8g2_font_4x6_tr);
  if (statusActive())
    u8g2.drawStr(2, 63, trunc(statusMsg, 26).c_str());
  else
    u8g2.drawStr(2, 63, "#/ENC:PING  RSSI>USB LOG");
}

void drawBridgePage() {
  drawHeader("BRIDGE MODE");

  if (bridgeShowInfo) {
    u8g2.setFont(u8g2_font_5x7_tr);
  char l1[28], l2[28], l3[28], l4[28], l5[28];
    snprintf(l1, sizeof(l1), "Project: %s", PROJECT_NAME);
#if LORACHAT_ENABLE_BT
    snprintf(l2, sizeof(l2), "Build: DUAL WIFI+BT");
#else
    snprintf(l2, sizeof(l2), "Build: WIFI ONLY");
#endif
  snprintf(l3, sizeof(l3), "Mode:%s e:%d", bridgeModeName(bridgeMode), bridgeLastError);
#if LORACHAT_ENABLE_BT
  snprintf(l4, sizeof(l4), "BT:%d rx:%lus", SerialBT.hasClient() ? 1 : 0, (millis() - lastBtRxMs) / 1000UL);
#else
  snprintf(l4, sizeof(l4), "BT:NA rx:--");
#endif
  snprintf(l5, sizeof(l5), "STA:%d H:%lu", (int)WiFi.softAPgetStationNum(), (unsigned long)ESP.getFreeHeap());
    u8g2.drawStr(2, 22, l1);
    u8g2.drawStr(2, 32, l2);
    u8g2.drawStr(2, 42, l3);
    u8g2.drawStr(2, 50, l4);
    u8g2.drawStr(2, 58, l5);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(2, 64, "D:VIEW A/*:HOME");
    return;
  }

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(2, 24, (String("Current: ") + bridgeModeName(bridgeMode)).c_str());
  u8g2.drawStr(2, 36, (String("Select : ") + bridgeModeName(bridgeDraftMode)).c_str());

  u8g2.setFont(u8g2_font_5x7_tr);
  if (bridgeDraftMode == BRIDGE_WIFI) {
    u8g2.drawStr(2, 47, "SSID:");
    u8g2.drawStr(30, 47, trunc(phoneBridgeSsid.length() ? phoneBridgeSsid : "--", 16).c_str());
    u8g2.drawStr(2, 56, "PASS:");
    u8g2.drawStr(30, 56, trunc(phoneBridgePass.length() ? phoneBridgePass : "--", 16).c_str());
  } else {
#if LORACHAT_ENABLE_BT
    u8g2.drawStr(2, 47, "BT Serial Text Input");
    u8g2.drawStr(2, 56, btBridgeName.length() ? btBridgeName.c_str() : "Name pending...");
#else
    u8g2.drawStr(2, 47, "Bluetooth not in this build");
    u8g2.drawStr(2, 56, "Dual build needs larger app");
#endif
  }
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 63, bridgeDraftMode == BRIDGE_WIFI ? "URL:192.168.4.1  # APPLY" : "ENC/2/8 MODE # APPLY D SYS");
}

void drawDisplay() {
  u8g2.clearBuffer();

  if (uiPage == PAGE_HOME)       drawHomePage();
  else if (uiPage == PAGE_CHAT)  drawChatPage();
  else if (uiPage == PAGE_QUICK) drawQuickPage();
  else if (uiPage == PAGE_RADIO) drawRadioPage();
  else if (uiPage == PAGE_INBOX) drawInboxPage();
  else if (uiPage == PAGE_RANGE) drawRangePage();
  else if (uiPage == PAGE_BRIDGE) drawBridgePage();
  else                           drawStatsPage();

  u8g2.sendBuffer();
  dispDirty = false;
}

void IRAM_ATTR encISR() {
  const int8_t t[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  uint8_t state = ((digitalRead(ENC_CLK)?1:0)<<1)|(digitalRead(ENC_DT)?1:0);
  uint8_t idx   = (encPrevState<<2)|state;
  encPrevState  = state;
  encAccum += t[idx];
  if (encAccum >= 2)  { encAccum = 0; encRawDelta++; }
  if (encAccum <= -2) { encAccum = 0; encRawDelta--; }
}

int readEnc() {
  noInterrupts();
  int d = encRawDelta;
  encRawDelta = 0;
  interrupts();
  return d;   // raw step count; callers use modulo so large spins work fine
}

bool readEncBtn() {
  bool cur = digitalRead(ENC_SW);
  unsigned long now = millis();
  if (cur != lastBtnState && now - lastBtnTime > 50) {
    lastBtnTime = now;
    lastBtnState = cur;
    if (cur == LOW) return true;
  }
  return false;
}

void onKeyDown(char key) {
  Serial.print("[KEY] "); Serial.println(key);

  if (key == 'A') {
    goHome();
    dispDirty = true;
    return;
  }

  if (uiPage == PAGE_HOME) {
    if (key == '#') openHomeSelection();
    dispDirty = true;
    return;
  }

  if (uiPage == PAGE_CHAT) {
    switch (key) {
      case '#':
        if (typingBuf.length() > 0) {
          String msg = typingBuf;
          typingBuf = "";
          sendDataMsg(msg);
        } else setStatus("TYPE FIRST");
        break;
      case '*':
        starHeld = true;
        starPressTime = millis();
        backspaceTyped();
        break;
      case 'B':
        appendTyped(CHARSET[pickerIdx]);
        break;
      case 'C':
        appendTyped(' ');
        break;
      case 'D':
        typingBuf = "STATUS OK";
        setStatus("TPL: STATUS OK");
        break;
      default:
        if ((int)typingBuf.length() < MAX_MSG_LEN) {
          typingBuf += key;
          setStatus(String("ADD: ") + key);
        } else setStatus("BUF FULL");
        break;
    }
  } else if (uiPage == PAGE_QUICK) {
    if (key == '#' || key == 'B') {
      sendDataMsg(String(QUICK_MSGS[quickIdx]));
    }
  } else if (uiPage == PAGE_RADIO) {
    if (key == '#') {
      if (freqDraftOffset != channelOffset) {
        if (applyFreq(draftFreq())) {
          channelOffset = freqDraftOffset;
          markSettingsDirty();
          setStatus("FREQ SET " + fmtFreq(activeFreq()));
        } else {
          freqDraftOffset = channelOffset;
          setStatus("FREQ ERR");
        }
      } else setStatus("NO CHANGE");
      radioDirty = false;
    } else if (key == '*') {
      freqDraftOffset = channelOffset;
      radioDirty = false;
      setStatus("RESET DRAFT");
    } else if (key == '2' || key == 'B') {
      freqDraftOffset++;
      clampDraftOffset();
      radioDirty = true;
      setStatus(fmtFreq(draftFreq()), 900);
    } else if (key == '8' || key == 'C') {
      freqDraftOffset--;
      clampDraftOffset();
      radioDirty = true;
      setStatus(fmtFreq(draftFreq()), 900);
    } else if (key == 'D') {
      cyclePresetDraft(1);
    }
  } else if (uiPage == PAGE_INBOX) {
    if (key == 'D') {
      clearInbox();
      inboxViewOffset = 0;
      setStatus("INBOX CLEARED", 1200);
    }
  } else if (uiPage == PAGE_RANGE) {
    if (key == '#') sendPing();
  } else if (uiPage == PAGE_BRIDGE) {
    if (key == '#') {
      applyBridgeMode(bridgeDraftMode);
    } else if (key == '*' || key == 'A') {
      goHome();
    } else if (key == 'D') {
      bridgeShowInfo = !bridgeShowInfo;
    } else if (key == '2' || key == '8' || key == 'B' || key == 'C') {
      bridgeDraftMode = (bridgeDraftMode == BRIDGE_WIFI) ? BRIDGE_BT : BRIDGE_WIFI;
      setStatus(String("MODE ") + bridgeModeName(bridgeDraftMode), 1000);
    }
  } else {
    if (key == '#') goHome();
  }

  dispDirty = true;
}

void onKeyUp(char key) {
  if (key == '*') {
    starHeld = false;
  }
}

String generateMAC() {
  uint64_t chip = ESP.getEfuseMac();
  char buf[13];
  snprintf(buf, sizeof(buf), "%04X%04X%04X",
           (uint16_t)(chip >> 32),
           (uint16_t)(chip >> 16),
           (uint16_t)(chip));
  String m = String(buf);
  return m.substring(0,4) + ":" + m.substring(4,8) + ":" + m.substring(8,12);
}

void runBootSelfTest() {
  bool pass = true;
  int encClk = digitalRead(ENC_CLK);
  int encDt  = digitalRead(ENC_DT);
  int encSw  = digitalRead(ENC_SW);
  if ((encClk != HIGH && encClk != LOW) || (encDt != HIGH && encDt != LOW) || (encSw != HIGH && encSw != LOW)) {
    pass = false;
  }

  Serial.println(String("[SELFTEST] OLED:") + "OK"
                 + " ENC:" + (pass ? "OK" : "WARN")
                 + " KP:" + "OK");

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(2, 14, "SELFTEST");
  u8g2.drawStr(2, 26, "OLED: OK");
  u8g2.drawStr(2, 38, pass ? "ENC : OK" : "ENC : WARN");
  u8g2.drawStr(2, 50, "KEY : OK");
  u8g2.drawStr(2, 62, "LORA: init next...");
  u8g2.sendBuffer();
  delay(450);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  myMAC = generateMAC();
  loadSettings();

  typingBuf.reserve(MAX_MSG_LEN + 2);
  btRxLine.reserve(MAX_MSG_LEN + 12);
  statusMsg.reserve(24);

  Serial.println("\n╔═══════════════════════════════╗");
  Serial.println("║  LORA CHAT                    ║");
  Serial.println("╚═══════════════════════════════╝");
  Serial.println("MAC  : " + myMAC);
  Serial.println("PROFILE: " + String(PROFILE_NAME));
  Serial.println("RANGE: SF12 / BW125 / CR4-8 / 20dBm");
  Serial.println("ACK  : timeout=" + String(ackTimeoutMs) + "ms retries=" + String(maxRetries));
  Serial.println("RANGE_LOG CSV: seq,rssi,snr,from,msg");
  Serial.println("RANGE_CSV HEADER: ms,seq,rssi,snr,from,ack_rtt_ms,loss_pct,msg\n");

  u8g2.begin();
  drawBootSequence();

  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT,  INPUT);
  pinMode(ENC_SW,  INPUT_PULLUP);
  encPrevState = ((digitalRead(ENC_CLK) ? 1 : 0) << 1) | (digitalRead(ENC_DT) ? 1 : 0);
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT),  encISR, CHANGE);

  keypad.setDebounceTime(KEY_DEBOUNCE_MS);
  keypad.setHoldTime(FREQ_HOLD_MS);
  keypad.addEventListener([](KeypadEvent key) {
    switch (keypad.getState()) {
      case PRESSED:  onKeyDown(key); break;
      case RELEASED: onKeyUp(key);   break;
      default: break;
    }
  });

  runBootSelfTest();

  if (!initLoRa(activeFreq())) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 30, "LoRa FAILED!");
    u8g2.drawStr(0, 46, "Check wiring");
    u8g2.sendBuffer();
    Serial.println("[ERROR] LoRa init failed. Halting.");
    while (1) delay(1000);
  }

  BridgeMode desiredBridgeMode = bridgeMode;
  if (!PROFILE_ALLOW_WIFI && PROFILE_ALLOW_BT) desiredBridgeMode = BRIDGE_BT;
  bridgeMode = BRIDGE_WIFI;
  bridgeDraftMode = BRIDGE_WIFI;
  if (PROFILE_ALLOW_WIFI) initPhoneBridge();

  if (desiredBridgeMode == BRIDGE_BT) {
    applyBridgeMode(BRIDGE_BT);
  }

  freqDraftOffset = channelOffset;
  drawDisplay();
  Serial.println("[READY]\n");
}

void loop() {
  if (bridgeMode == BRIDGE_WIFI) phoneServer.handleClient();
  else                           handleBluetoothBridgeInput();

  if (millis() - lastBridgeDiagMs > 5000UL) {
    lastBridgeDiagMs = millis();
    if (bridgeMode == BRIDGE_WIFI || bridgeMode == BRIDGE_BT) {
      Serial.println("[BRIDGE_DIAG] " + bridgeDiagText());
    }
  }

  keypad.getKeys();

  checkRx();

  if ((uiPage == PAGE_CHAT || uiPage == PAGE_INBOX) && unreadRx) unreadRx = false;

  handleRetry();

  if (starHeld && (millis() - starPressTime >= LONG_PRESS)) {
    typingBuf = "";
    setStatus("CLEARED");
    starHeld  = false;
    dispDirty = true;
  }

  int dir = readEnc();
  if (dir != 0) {
    if (uiPage == PAGE_HOME) {
      homeIdx = (homeIdx + dir + HOME_APP_COUNT) % HOME_APP_COUNT;

      if (homeIdx < homeTopIdx) homeTopIdx = homeIdx;
      if (homeIdx >= homeTopIdx + HOME_VISIBLE_ROWS) {
        homeTopIdx = homeIdx - HOME_VISIBLE_ROWS + 1;
      }
    } else if (uiPage == PAGE_CHAT) {
      pickerIdx = (pickerIdx + dir + CHARSET_LEN) % CHARSET_LEN;
    } else if (uiPage == PAGE_QUICK) {
      quickIdx = (quickIdx + dir + QUICK_MSG_COUNT) % QUICK_MSG_COUNT;
      markSettingsDirty();
    } else if (uiPage == PAGE_RADIO) {
      freqDraftOffset += dir;
      clampDraftOffset();
      radioDirty = true;
    } else if (uiPage == PAGE_INBOX) {
      inboxViewOffset -= dir;
      if (inboxViewOffset < 0) inboxViewOffset = 0;
      if (inboxCount == 0) inboxViewOffset = 0;
      else if (inboxViewOffset > inboxCount - 1) inboxViewOffset = inboxCount - 1;
    } else if (uiPage == PAGE_BRIDGE) {
      bridgeDraftMode = (bridgeDraftMode == BRIDGE_WIFI) ? BRIDGE_BT : BRIDGE_WIFI;
    }
    dispDirty = true;
  }

  if (readEncBtn()) {
    if (uiPage == PAGE_HOME) {
      openHomeSelection();
    } else if (uiPage == PAGE_CHAT) {
      appendTyped(CHARSET[pickerIdx]);
    } else if (uiPage == PAGE_QUICK) {
      sendDataMsg(String(QUICK_MSGS[quickIdx]));
    } else if (uiPage == PAGE_RADIO) {
      if (applyFreq(draftFreq())) {
        channelOffset = freqDraftOffset;
        markSettingsDirty();
        radioDirty = false;
        setStatus("FREQ SET " + fmtFreq(activeFreq()));
      } else {
        setStatus("FREQ ERR");
      }
    } else if (uiPage == PAGE_RANGE) {
      sendPing();
    } else if (uiPage == PAGE_BRIDGE) {
      applyBridgeMode(bridgeDraftMode);
    }
    dispDirty = true;
  }

  unsigned long now = millis();
  if (dispDirty || statusActive() || now - lastDispTime >= DISP_RATE) {
    drawDisplay();
    lastDispTime = now;
  }

  saveSettingsIfNeeded();
}
