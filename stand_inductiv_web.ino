#include <WiFi.h>
#include <EEPROM.h>
#include <AccelStepper.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <algorithm>

const char* ssid = "TP-Link_4DAC";
const char* password = "Pistol30";

#define ENCODER_A 26
#define ENCODER_B 27
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define ADC_PIN 34

const long ENCODER_MIN = 0;
const long ENCODER_MAX = 42730;
const float distMax_mm = 105.0;
const int EEPROM_ADDR_ENCODER = 0;
const int EEPROM_SIZE = 512;

#define NUM_SAMPLES 200
#define SCALARE_CORECTIE 2.45

volatile long pozitieEncoder = 0;
long pozitieTintita = 0;
long poz_tinta = -1;
int directie = 0;
bool calibrareInDesfasurare = false;
bool calibrareConfirmata = false;
unsigned long ultimaSalvareMillis = 0;
long ultimaPozitieSalvata = 0;
const unsigned long intervalSalvare = 2000;

uint16_t esantioane[NUM_SAMPLES];
float Vpp = 0.0;
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Pagini HTML
// pagini.h – Interfață modernizată pentru standul inductiv

const char pagina_index[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang=\"ro\">
<head>
  <meta charset='UTF-8'>
  <title>Laborator Virtual - Index</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f0f4f8;
      text-align: center;
      padding: 2rem;
    }
    h1 {
      color: #2d3748;
      margin-bottom: 2rem;
    }
    .link-box {
      margin: 1rem;
      padding: 1rem;
      border-radius: 10px;
      background: #3498db;
      color: white;
      text-decoration: none;
      font-size: 20px;
      display: inline-block;
      width: 250px;
      transition: background 0.3s;
    }
    .link-box:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>
  <h1>Laborator Senzori - Acces Standuri</h1>
  <a class=\"link-box\" href=\"http://192.168.0.101\">🔌 Stand Capacitiv</a><br>
  <a class=\"link-box\" href=\"http://192.168.1.212\">🧲 Stand Inductiv</a><br>
  <a class=\"link-box\" href=\"/control\">🎚️ Stand Rezistiv (curent)</a>
</body>
</html>
)rawliteral";

const char pagina_principala[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='ro'>
<head>
  <meta charset='UTF-8'>
  <title>Măsurare distanță - Senzor Inductiv</title>
  <style>
    body {
      background: #f0f4f8;
      font-family: 'Segoe UI', sans-serif;
      margin: 0;
      padding: 0;
    }
    header {
      background: #2d3748;
      color: white;
      padding: 1rem;
      text-align: center;
      font-size: 1.5em;
    }
    nav {
      background: #4a5568;
      padding: 0.5rem;
      text-align: center;
    }
    nav a {
      color: white;
      text-decoration: none;
      margin: 0 1rem;
      padding: 0.5rem 1rem;
      background: #2b6cb0;
      border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover {
      background: #2c5282;
    }
    main {
      max-width: 800px;
      margin: 2rem auto;
      background: white;
      padding: 2rem;
      border-radius: 12px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
    h2 {
      color: #2d3748;
      margin-bottom: 1.5rem;
    }
    p, li {
      font-size: 17px;
      color: #333;
      line-height: 1.6;
      text-align: justify;
    }
    ul {
      margin-top: 1rem;
      padding-left: 1.5rem;
    }
  </style>
</head>
<body>
  <header>Laborator Senzori și Traductoare</header>
  <nav>
    <a href="http://192.168.1.215/index">🏠 Index</a>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <h2>Măsurarea distanței cu senzor inductiv</h2>
    <p>
      Această secțiune este dedicată studierii <strong>senzorului inductiv de deplasare</strong>, care funcționează pe baza modificării unei mărimi inductive proporțional cu poziția unui element mobil.
    </p>
    <p>Prin intermediul acestui stand, studentul poate:</p>
    <ul>
      <li>Controla poziția tijei metalice printr-un motor pas cu pas și encoder incremental;</li>
      <li>Observa variația tensiunii Vpp măsurate de senzorul inductiv în funcție de distanță;</li>
      <li>Vizualiza în timp real graficul Vpp vs. distanță și salva datele ca imagine sau fișier .csv;</li>
      <li>Testa funcționalități de resetare, memorare EEPROM și recalibrare cu parolă;</li>
      <li>Învăța principiile de funcționare ale senzorilor inductivi într-un mediu interactiv.</li>
    </ul>
    <p>
      Interfața web modernă permite control precis și oferă feedback vizual clar, contribuind la înțelegerea practică a comportamentului senzorului inductiv într-un sistem real.
    </p>
  </main>
</body>
</html>
)rawliteral";


const char pagina_control[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Control Senzor Inductiv</title>
  <style>
    body { background: #f8fafc; font-family: 'Segoe UI', sans-serif; margin: 0; padding: 0; }
    header { background: #2d3748; color: white; padding: 1rem; text-align: center; font-size: 1.5em; }
    nav { background: #4a5568; padding: 0.5rem; text-align: center; }
    nav a {
      color: white; text-decoration: none;
      margin: 0 1rem; padding: 0.5rem 1rem;
      background: #2b6cb0; border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover { background: #2c5282; }
    main { padding: 2rem; text-align: center; }
    button {
      padding: 10px 20px; margin: 5px;
      border-radius: 10px; font-size: 16px;
      background: #3498db; color: white;
      border: 2px solid #1f4f82;
      cursor: pointer;
    }
    button:hover { background: #2980b9; }
    #reset-button { background: #e74c3c; border: 2px solid #a5281c; }
    #reset-button:hover { background: #c0392b; }
    #recalibrare-button { background: #27ae60; border: 2px solid #1e8449; }
    #recalibrare-button:hover { background: #1e8449; }
    #progress-container {
      width: 80%; height: 25px;
      background: #ccc; margin: 20px auto;
      border-radius: 10px; border: 2px solid #888;
      overflow: hidden;
    }
    #progress-bar {
      height: 100%; width: 0%; background: #4caf50;
      color: white; line-height: 25px; font-weight: bold;
    }
    #info-box {
      margin-top: 20px; display: flex;
      flex-direction: column; align-items: flex-start;
      justify-content: center; gap: 10px;
      font-size: 18px; max-width: 500px;
      margin-left: auto; margin-right: auto;
      padding: 15px; border: 1px solid #ccc;
      border-radius: 10px; background: #ffffff;
      box-shadow: 0 0 10px rgba(0,0,0,0.05);
    }
    #log-container {
      position: fixed;
      right: 20px;
      bottom: 20px;
      width: 300px;
      background: #ffffff;
      border: 1px solid #ccc;
      border-radius: 8px;
      box-shadow: 0 0 10px rgba(0,0,0,0.2);
      padding: 10px;
    }
    #log-box {
      max-height: 120px;
      overflow-y: auto;
      white-space: pre-wrap;
      text-align: left;
    }
    #tooltip {
      display: none;
      position: absolute;
      background: #fff;
      border: 2px solid black;
      padding: 4px 8px;
      font-size: 13px;
      border-radius: 6px;
      pointer-events: none;
      transform: translate(-50%, -100%);
      z-index: 10;
    }
    #confirmBox, #confirmRecalibrareBox, #confirmRecalibrare2Box {
  display: none;
  position: fixed;
  top: 50%; left: 50%;
  transform: translate(-50%, -50%);
  background: white;
  padding: 20px;
  border: 2px solid #333;
  border-radius: 10px;
  box-shadow: 0 0 10px rgba(0,0,0,0.5);
  z-index: 1000;
  max-width: 90%;
}

#confirmBox button,
#confirmRecalibrareBox button,
#confirmRecalibrare2Box button {
  padding: 8px 14px;
  margin: 5px;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  font-size: 14px;
  background: #3498db;
  color: white;
}

#confirmBox button:hover,
#confirmRecalibrareBox button:hover,
#confirmRecalibrare2Box button:hover {
  background: #2c80b4;
}

  </style>
</head>
<body>
  <header>Control Senzor Inductiv</header>
  <nav>
    <a href="http://192.168.1.215/index">🏠 Index</a>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <div>
      <button onclick="send('b')">◀️ Înapoi</button>
      <button onclick="send('s')">⛔ Stop</button>
      <button onclick="send('f')">▶️ Înainte</button>
      <button onclick="send('p')">📌 EEPROM</button>
      <button id="reset-button" onclick="confirmReset()">🚫 Reset</button>
      <button id="recalibrare-button" onclick="confirmRecalibrare()">🛠️ Recalibrare</button>
    </div>
    <div style="margin-top:20px;">
      <input type="number" id="input-mm" placeholder="Introduceți distanța [mm]" min="0" max="105" style="padding: 8px; width: 200px; border-radius: 8px;">
      <button onclick="gotoMM()">Mergi</button>
    </div>
    <div id="progress-container"><div id="progress-bar">0 mm</div></div>
    <div id="info-box">
      <div><strong>⚙️ Valoare encoder:</strong> <span id="enc">-</span></div>
      <div><strong>📏 Distanța actuală:</strong> <span id="dist">-</span> mm</div>
      <div><strong>📉 Vpp măsurat:</strong> <span id="vpp">-</span> V</div>
      <div><strong>🔄 Stare motor:</strong> <span id="motor-dir">-</span></div>
    </div>
    <div style="width: fit-content; margin: 30px auto 0 auto; text-align: center;">
      <canvas id="graficRezistiv" width="500" height="320" style="background:#fff; border:1px solid #ccc;"></canvas>
      <div id="tooltip"></div>
      <div style="display: flex; justify-content: space-between; margin-top: 10px;">
        <button onclick="salveazaGrafic()" style="background:#2c3e50; border:2px solid #000; color:white;">💾 Salvează grafic (.jpg)</button>
        <button onclick="salveazaDateCSV()" style="background:#16a085; border:2px solid #0e6655; color:white;">📄 Salvează date (.csv)</button>
        <button onclick="resetGrafic()" style="background:#e67e22; border:2px solid #b35400;">🔁 Resetare Grafic</button>
      </div>
    </div>
    <div id="log-container">
      <strong>🧾 Log comenzi:</strong>
      <div id="log-box"></div>
    </div>
    <div id="confirmBox">
  <p><strong>Atenție!</strong> Această acțiune va reseta valoarea encoderului la 0!<br>Vrei să continui?</p>
  <button onclick="executeReset()">Da</button>
  <button onclick="closeConfirm()">Nu</button>
</div>

<div id="confirmRecalibrareBox">
  <p><strong>Recalibrare encoder</strong></p>
  <p>Introduceți parola pentru a continua:</p>
  <input type="password" id="recalibrarePass">
  <div style="margin-top: 10px;">
    <button onclick="checkRecalibrarePass()">Confirmă</button>
    <button onclick="closeRecalibrare()">Anulează</button>
  </div>
</div>

<div id="confirmRecalibrare2Box">
  <p><strong>Poziționați sistemul în poziția de start (0 mm)</strong></p>
  <p>Valoare encoder: <span id="encRecal">-</span></p>
  <p>Vpp măsurat: <span id="potRecal">-</span></p>
  <p>
    Apăsați <strong>Calibrare automata</strong> pentru a deplasa motorul până la valoarea de referință (0).<br>
    În caz de eroare, folosiți <strong>Oprire urgentă</strong>.
  </p>
  <button onclick="send('c')">🔄 Calibrare automata</button>
  <button onclick="send('s')">⛔ Oprire urgentă</button>
  <button onclick="confirmRecalibrat()">✅ Confirmă recalibrare</button>
  <button onclick="closeRecalibrare2()">Anulează</button>
</div>

  </main>
  <script>
let ws;
const canvas = document.getElementById("graficRezistiv");
const ctx = canvas.getContext("2d");
const MAX_POINTS = 100;
let dataPoints = [];
let puncteRosii = [];
let hoverPoint = null;
const tooltip = document.getElementById("tooltip");

window.onload = () => {
  ws = new WebSocket('ws://' + location.host + '/ws');
  ws.onmessage = (e) => {
    const txt = e.data;
    let encoder = 0, dist = 0, vpp = 0;
    try {
      encoder = parseInt(txt.split("Encoder:")[1].split("|")[0]);
      dist = parseFloat(txt.split("Dist:")[1].split("mm")[0]);
      vpp = parseFloat(txt.split("Vpp:")[1]);
    } catch (err) {}

    document.getElementById("enc").innerText = encoder;
    document.getElementById("dist").innerText = dist.toFixed(1);
    document.getElementById("vpp").innerText = vpp.toFixed(3);

    document.getElementById("progress-bar").style.width = Math.min(100, dist / 105 * 100) + "%";
    document.getElementById("progress-bar").innerText = dist.toFixed(1) + " mm";
    adaugaPunct(dist, vpp);
  };
};

function send(cmd) {
  if (ws.readyState === WebSocket.OPEN) {
    ws.send(cmd);
    logAction(cmd);
    let dir = "-";
    if (cmd === 'b') dir = "◀️ Înapoi";
    else if (cmd === 's') dir = "⛔ Stop";
    else if (cmd === 'f') dir = "▶️ Înainte";
    else if (cmd === 'r') dir = "❌ Reset";
    else if (cmd === 'p') dir = "📌 EEPROM";
    else if (cmd === 'x') dir = "🛠️ Recalibrare";
    else if (cmd === 'c') dir = "🔄 Calibrare automata";
    else if (cmd.startsWith("g")) dir = `🌟 Mers la ${cmd.substring(1)} mm`;
    document.getElementById("motor-dir").innerText = dir;
  }
}

function gotoMM() {
  const val = parseFloat(document.getElementById("input-mm").value);
  if (isNaN(val) || val < 0 || val > 105) {
    alert("Introduceți o valoare între 0 și 105 mm.");
    return;
  }
  send("g" + val.toFixed(1));
}

function confirmReset() {
  document.getElementById("confirmBox").style.display = "block";
}
function closeConfirm() {
  document.getElementById("confirmBox").style.display = "none";
}
function executeReset() {
  send('r');
  closeConfirm();
}
function confirmRecalibrare() {
  document.getElementById("confirmRecalibrareBox").style.display = "block";
}
function closeRecalibrare() {
  document.getElementById("confirmRecalibrareBox").style.display = "none";
  document.getElementById("recalibrarePass").value = "";
}
function checkRecalibrarePass() {
  const parola = document.getElementById("recalibrarePass").value;
  if (parola === "parola123") {
    closeRecalibrare();
    document.getElementById("confirmRecalibrare2Box").style.display = "block";
  } else {
    alert("Parolă greșită!");
  }
}
function closeRecalibrare2() {
  document.getElementById("confirmRecalibrare2Box").style.display = "none";
}
function confirmRecalibrat() {
  send('x');
  closeRecalibrare2();
}

function logAction(cmd) {
  const box = document.getElementById("log-box");
  const timestamp = new Date().toLocaleTimeString();
  let descriere = "";
  if (cmd === 'f') descriere = "▶️ Motor înainte";
  else if (cmd === 'b') descriere = "◀️ Motor înapoi";
  else if (cmd === 's') descriere = "⛔ Oprire motor";
  else if (cmd === 'r') descriere = "❌ Reset encoder";
  else if (cmd === 'p') descriere = "📌 Salvare EEPROM";
  else if (cmd === 'x') descriere = "🛠️ Recalibrare encoder";
  else if (cmd === 'c') descriere = "🔄 Calibrare automata";
  else if (cmd.startsWith('g')) descriere = `🌟 Mers la ${cmd.substring(1)} mm`;
  else descriere = `Comandă necunoscută: ${cmd}`;
  box.innerHTML += `[${timestamp}] ${descriere}<br>`;
  box.scrollTop = box.scrollHeight;
}

function map(val, inMin, inMax, outMin, outMax) {
  return (val - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

function adaugaPunct(x, y) {
  dataPoints.push({ x, y });
  if (Math.round(x) % 5 === 0 && !puncteRosii.some(p => Math.round(p.x) === Math.round(x))) {
    puncteRosii.push({ x, y });
  }
  deseneazaGrafic();
}

canvas.addEventListener("mousemove", handleHover);
function handleHover(e) {
  const rect = canvas.getBoundingClientRect();
  const mouseX = (e.clientX - rect.left) * canvas.width / canvas.offsetWidth;
  const mouseY = (e.clientY - rect.top) * canvas.height / canvas.offsetHeight;
  const margineSt = 60, margineJos = 270, margineSus = 40, latime = canvas.width;
  hoverPoint = null;
  for (const p of dataPoints) {
    const x = map(p.x, 0, 105, margineSt, latime - 20);
    const y = map(p.y, 0, 0.5, margineJos, margineSus);
    if (Math.abs(mouseX - x) < 8 && Math.abs(mouseY - y) < 8) {
      tooltip.style.display = "block";
      tooltip.style.left = `${rect.left + x}px`;
      tooltip.style.top = `${rect.top + y - 30}px`;
      tooltip.innerText = `X: ${p.x.toFixed(1)} mm | Vpp: ${p.y.toFixed(3)} V`;
      hoverPoint = { x, y };
      deseneazaGrafic();
      return;
    }
  }
  tooltip.style.display = "none";
  hoverPoint = null;
  deseneazaGrafic();
}

function deseneazaGrafic() {
  const margineSt = 60, margineJos = 270, margineSus = 40, latime = canvas.width;
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  ctx.strokeStyle = "#000"; ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(margineSt, margineSus);
  ctx.lineTo(margineSt, margineJos);
  ctx.lineTo(latime - 20, margineJos);
  ctx.stroke();

  // Grile si etichete
  ctx.strokeStyle = "#e0e0e0"; ctx.lineWidth = 1; ctx.font = "12px sans-serif"; ctx.fillStyle = "#333";
  for (let y = 0; y <= 0.5; y += 0.05) {
    const yy = map(y, 0, 0.5, margineJos, margineSus);
    ctx.beginPath(); ctx.moveTo(margineSt, yy); ctx.lineTo(latime - 20, yy); ctx.stroke();
    ctx.fillText(y.toFixed(2), margineSt - 40, yy + 4);
  }
  for (let x = 0; x <= 105; x += 5) {
    const xx = map(x, 0, 105, margineSt, latime - 20);
    ctx.beginPath(); ctx.moveTo(xx, margineSus); ctx.lineTo(xx, margineJos); ctx.stroke();
    if (x % 10 === 0) ctx.fillText(x.toString(), xx - 8, margineJos + 15);
  }
  ctx.fillStyle = "#000";
  ctx.fillText("Distanță (mm)", latime / 2 - 40, margineJos + 35);
  ctx.save(); ctx.translate(13, canvas.height / 2 + 90); ctx.rotate(-Math.PI / 2);
  ctx.fillText("Tensiune (Vpp)", 0, 0);
  ctx.restore();

  // Linie si puncte
  ctx.strokeStyle = "#007bff"; ctx.lineWidth = 2;
  ctx.beginPath();
  dataPoints.forEach((p, i) => {
    const x = map(p.x, 0, 105, margineSt, latime - 20);
    const y = map(p.y, 0, 0.5, margineJos, margineSus);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  ctx.fillStyle = "#ff0000";
  puncteRosii.forEach(p => {
    const x = map(p.x, 0, 105, margineSt, latime - 20);
    const y = map(p.y, 0, 0.5, margineJos, margineSus);
    ctx.beginPath(); ctx.arc(x, y, 3, 0, 2 * Math.PI); ctx.fill();
  });

  if (hoverPoint) {
    ctx.strokeStyle = "#999"; ctx.setLineDash([5, 5]);
    ctx.beginPath(); ctx.moveTo(hoverPoint.x, margineSus); ctx.lineTo(hoverPoint.x, margineJos); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(margineSt, hoverPoint.y); ctx.lineTo(latime - 20, hoverPoint.y); ctx.stroke();
    ctx.setLineDash([]);
  }
}

function resetGrafic() {
  dataPoints = [];
  puncteRosii = [];
  deseneazaGrafic();
}

function salveazaGrafic() {
  const tmpCanvas = document.createElement('canvas');
  tmpCanvas.width = canvas.width;
  tmpCanvas.height = canvas.height;
  const tmpCtx = tmpCanvas.getContext('2d');
  tmpCtx.fillStyle = "#ffffff";
  tmpCtx.fillRect(0, 0, tmpCanvas.width, tmpCanvas.height);
  tmpCtx.drawImage(canvas, 0, 0);
  const now = new Date();
  const dataStr = now.toLocaleDateString("ro-RO");
  const oraStr = now.toLocaleTimeString("ro-RO").slice(0, 5);
  tmpCtx.fillStyle = "#000";
  tmpCtx.font = "14px sans-serif";
  tmpCtx.fillText(`Salvat: ${dataStr}, ${oraStr}`, 10, 20);
  const link = document.createElement('a');
  link.download = 'GraficValoriInductiv.jpg';
  link.href = tmpCanvas.toDataURL('image/jpeg', 1.0);
  link.click();
}

function salveazaDateCSV() {
  if (dataPoints.length === 0) {
    alert("Nu există date de salvat!"); return;
  }
  let csvContent = '\uFEFF';
  csvContent += "Distanță (mm),Vpp (V)\n";
  dataPoints.forEach(p => {
    csvContent += `${p.x.toFixed(2)},${p.y.toFixed(3)}\n`;
  });
  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = "ValoriInductiv.csv";
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}
</script>

</body>
</html>
)rawliteral";



const char pagina_help[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Ajutor</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f0f4f8;
      margin: 0;
      padding: 0;
      text-align: center;
    }
    header {
      background: #2d3748;
      color: white;
      padding: 1rem;
      font-size: 1.5em;
    }
    nav {
      background: #4a5568;
      padding: 0.5rem;
      text-align: center;
    }
    nav a {
      color: white;
      text-decoration: none;
      margin: 0 1rem;
      padding: 0.5rem 1rem;
      background: #2b6cb0;
      border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover {
      background: #2c5282;
    }
    main {
      padding: 2rem;
    }
    .btn-section {
      display: inline-block;
      margin: 10px;
      padding: 15px 25px;
      font-size: 18px;
      background: #3498db;
      color: white;
      text-decoration: none;
      border-radius: 10px;
      border: 2px solid #1f4f82;
      transition: background 0.3s;
    }
    .btn-section:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>
  <header>Ajutor - Alege o componentă</header>
  <nav>
    <a href="http://192.168.1.215/index">🏠 Index</a>
    <a href="/">Acasă</a>
    <a href="/control">🎛️ Control</a>
    <a href="/help">❓ Ajutor</a>
  </nav>
  <main>
    <a href="/encoder" class="btn-section">🌀 Encoder</a>
    <a href="/motor" class="btn-section">⚙️ Motor</a>
    <a href="/esp" class="btn-section">📡 ESP32</a>
    <a href="/driver" class="btn-section">🔌 Driver Motor</a>
  </main>
</body>
</html>
)rawliteral";



const char pagina_help_encoder[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Ajutor - Encoder</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f8fafc;
      padding: 2rem;
      text-align: center;
    }
    h1 {
      color: #2d3748;
    }
    .content {
      max-width: 800px;
      margin: auto;
      text-align: left;
      font-size: 18px;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    a {
      display: inline-block;
      margin-top: 20px;
      padding: 10px 20px;
      background: #3498db;
      color: white;
      text-decoration: none;
      border-radius: 8px;
      border: 2px solid #1f4f82;
    }
    a:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>
  <h1>🌀 Encoder incremental</h1>
  <div class="content">
    <p>Encoderul incremental folosit este un tip de senzor rotativ care generează impulsuri la rotirea axului.</p>
    <ul>
      <li>✔️ Tip: Incremental, cu două canale A și B</li>
      <li>⚡ Semnal: Digital (0/5V)</li>
      <li>🔄 Rezoluție tipică: 600-1000 PPR (Pulse per Revolution)</li>
      <li>🔌 Alimentare: 5V</li>
      <li>🔧 Ieșire: Semnal pătrat, compatibil cu ESP32</li>
    </ul>
    <p>Funcționarea se bazează pe diferențierea fazelor A și B pentru a determina sensul rotației. Este utilizat pentru a măsura poziția precisă a unui ax rotativ în proiecte cu motor pas cu pas.</p>
    <a href="/help">⏪ Înapoi</a>
  </div>
</body>
</html>
)rawliteral";


const char pagina_help_motor[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Ajutor - Motor Pas cu Pas</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f8fafc;
      padding: 2rem;
      text-align: center;
    }
    h1 {
      color: #2d3748;
    }
    .content {
      max-width: 800px;
      margin: auto;
      text-align: left;
      font-size: 18px;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    a {
      display: inline-block;
      margin-top: 20px;
      padding: 10px 20px;
      background: #3498db;
      color: white;
      text-decoration: none;
      border-radius: 8px;
      border: 2px solid #1f4f82;
    }
    a:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>
  <h1>⚙️ Motor Pas cu Pas 28BYJ-48</h1>
  <div class="content">
    <p>Motorul <strong>28BYJ-48</strong> este un motor pas cu pas unipolar, foarte utilizat în proiecte embedded și aplicații de precizie redusă.</p>
    <ul>
      <li>🔌 Tensiune de alimentare: 5V</li>
      <li>🔁 Unghi pe pas: 5.625°</li>
      <li>⚙️ Raport de reducere: 1:64</li>
      <li>🌀 Număr de pași pe rotație completă: 4096 (cu gearbox)</li>
      <li>📦 Cuplu redus, ideal pentru sarcini ușoare</li>
      <li>🎯 Controlat prin ULN2003 sau direct prin 4 pini digitali</li>
    </ul>
    <p>Este potrivit pentru aplicații de poziționare, cum ar fi reglajul tijei sau a unei scale de măsurare.</p>
    <a href="/help">⏪ Înapoi</a>
  </div>
</body>
</html>
)rawliteral";

const char pagina_help_esp[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Ajutor - ESP32</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f8fafc;
      padding: 2rem;
      text-align: center;
    }
    h1 {
      color: #2d3748;
    }
    .content {
      max-width: 800px;
      margin: auto;
      text-align: left;
      font-size: 18px;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    a {
      display: inline-block;
      margin-top: 20px;
      padding: 10px 20px;
      background: #3498db;
      color: white;
      text-decoration: none;
      border-radius: 8px;
      border: 2px solid #1f4f82;
    }
    a:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>
  <h1>📡 ESP32</h1>
  <div class="content">
    <p>ESP32 este un microcontroler performant, ideal pentru aplicații IoT, comunicație și control în timp real.</p>
    <ul>
      <li>⚙️ Procesor dual-core Xtensa LX6 @ 240 MHz</li>
      <li>💾 Memorie: ~520 KB SRAM intern</li>
      <li>📶 Conectivitate WiFi 802.11 b/g/n + Bluetooth BLE</li>
      <li>🛠️ Interfețe: GPIO, ADC, DAC, SPI, I2C, UART, PWM</li>
      <li>🔌 Alimentare: 3.3V (logica și I/O)</li>
      <li>🧠 Programabil cu Arduino IDE, ESP-IDF, PlatformIO</li>
    </ul>
    <p>În proiectul curent, ESP32 controlează motorul pas cu pas, citește date de la Arduino prin UART și servește interfața web către utilizator prin WiFi.</p>
    <a href="/help">⏪ Înapoi</a>
  </div>
</body>
</html>
)rawliteral";

const char pagina_help_driver[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Ajutor - Driver Motor</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f8fafc;
      padding: 2rem;
      text-align: center;
    }
    h1 {
      color: #2d3748;
    }
    .content {
      max-width: 800px;
      margin: auto;
      text-align: left;
      font-size: 18px;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    a {
      display: inline-block;
      margin-top: 20px;
      padding: 10px 20px;
      background: #3498db;
      color: white;
      text-decoration: none;
      border-radius: 8px;
      border: 2px solid #1f4f82;
    }
    a:hover {
      background: #2980b9;
    }
  </style>
</head>
<body>
  <h1>🔌 Driver Motor ULN2003</h1>
  <div class="content">
    <p>Driverul <strong>ULN2003</strong> este un circuit integrat folosit pentru comanda motoarelor de curent continuu sau pas cu pas.</p>
    <ul>
      <li>📦 7 canale de ieșire cu tranzistoare Darlington</li>
      <li>🔁 Tensiune maximă: 50V</li>
      <li>⚡ Curent maxim per canal: ~500 mA</li>
      <li>🔒 Diode de protecție integrate pentru inducție inversă</li>
      <li>📘 Folosit pentru controlul motorului 28BYJ-48</li>
    </ul>
    <p>Funcționează ca interfață între ESP32 și motor, amplificând semnalul de comandă. Este conectat direct la cei 4 pini IN1-IN4 care activează bobinele motorului.</p>
    <a href="/help">⏪ Înapoi</a>
  </div>
</body>
</html>
)rawliteral";




void IRAM_ATTR citireEncoder() {
  if (digitalRead(ENCODER_B) != digitalRead(ENCODER_A))
    pozitieEncoder++;
  else
    pozitieEncoder--;
}

float calculeazaVpp() {
  for (int i = 0; i < NUM_SAMPLES; i++) {
    esantioane[i] = analogRead(ADC_PIN);
    delayMicroseconds(100);
  }

  uint16_t smooth[NUM_SAMPLES];
  const int window = 5;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    uint32_t suma = 0;
    int cnt = 0;
    for (int j = -window/2; j <= window/2; j++) {
      int idx = i + j;
      if (idx >= 0 && idx < NUM_SAMPLES) {
        suma += esantioane[idx];
        cnt++;
      }
    }
    smooth[i] = suma / cnt;
  }

  std::sort(smooth, smooth + NUM_SAMPLES);
  int low_idx = NUM_SAMPLES * 0.05;
  int high_idx = NUM_SAMPLES * 0.95;
  uint16_t min_val = smooth[low_idx];
  uint16_t max_val = smooth[high_idx];

  float Vref = 2.0 * SCALARE_CORECTIE;
  return (max_val - min_val) * Vref / 4095.0;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectat: " + WiFi.localIP().toString());

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR_ENCODER, pozitieEncoder);
  if (pozitieEncoder < 0 || pozitieEncoder > ENCODER_MAX) pozitieEncoder = 0;
  ultimaPozitieSalvata = pozitieEncoder;
  ultimaSalvareMillis = millis();

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), citireEncoder, RISING);

  analogReadResolution(12);
  analogSetAttenuation(ADC_6db);

  stepper.setMaxSpeed(800);

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
      String msg = "";
      for (size_t i = 0; i < len; i++) msg += (char)data[i];
      if (msg == "f") {
        directie = 1;
        calibrareInDesfasurare = false;
      } else if (msg == "b") {
        directie = -1;
        calibrareInDesfasurare = false;
      } else if (msg == "s") {
        directie = 0;
        EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
        EEPROM.commit();
      } else if (msg == "r") {
        pozitieEncoder = 0;
        EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
        EEPROM.commit();
      } else if (msg == "p") {
        long e;
        EEPROM.get(EEPROM_ADDR_ENCODER, e);
        client->text("📦 EEPROM → Encoder: " + String(e) + " | Vpp: -");
      } else if (msg.startsWith("g")) {
        float tinta_mm = msg.substring(1).toFloat();
        tinta_mm = constrain(tinta_mm, 0.0, distMax_mm);
        poz_tinta = tinta_mm * ENCODER_MAX / distMax_mm;
        calibrareInDesfasurare = false;
      } else if (msg == "c") {
        calibrareInDesfasurare = true;
        calibrareConfirmata = false;
        directie = -1;
      } else if (msg == "x") {
  if (calibrareInDesfasurare) {
    pozitieEncoder = 0;
    EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
    EEPROM.commit();
    directie = 0;
    poz_tinta = -1;  // ADĂUGĂ ASTA
    calibrareInDesfasurare = false;
    calibrareConfirmata = true;
  }
}
    }
  });

  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_principala);
  });
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_index);
  });
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_control);
  });
  server.on("/help", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help);
  });
  server.on("/encoder", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_encoder);
  });
  server.on("/motor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_motor);
  });
  server.on("/esp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_esp);
  });
  server.on("/driver", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_driver);
  });

  server.begin();
  Serial.println("✅ Server pornit");
}

void loop() {

    static unsigned long tUpdate = 0;
  if (millis() - tUpdate > 500) {
    Vpp = calculeazaVpp();
    float dist_mm = (float)pozitieEncoder * distMax_mm / ENCODER_MAX;
    String info = "Encoder: " + String(pozitieEncoder) + " | Dist: " + String(dist_mm, 1) + " mm | Vpp: " + String(Vpp, 3) + " V";
    ws.textAll(info);
    tUpdate = millis();
  }
  static unsigned long ultimaVerificareCalibrare = 0;

  // === Calibrare automată ===
// === Calibrare automată pe baza Vpp ===
if (calibrareInDesfasurare) {
  static unsigned long ultimaVerificareCalibrare = 0;
  if (millis() - ultimaVerificareCalibrare > 100) {
    ultimaVerificareCalibrare = millis();

    if (Vpp <= 0.106) {  // prag de referință pentru capătul inferior
      stepper.setSpeed(0);
      directie = 0;
      pozitieEncoder = 0;
      EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
      EEPROM.commit();
      calibrareInDesfasurare = false;
      calibrareConfirmata = true;
      Serial.println("✅ Calibrare completă: Vpp <= 0.106V, encoder resetat.");
    } else {
      stepper.setSpeed(-500);  // deplasare înapoi
    }
  }
  stepper.runSpeed();
  return;  // oprim restul logicii cât timp calibrăm
}

  // === Mers la poziție țintă ===
  if (poz_tinta != -1) {
    if (abs(pozitieEncoder - poz_tinta) <= 10) {
      stepper.setSpeed(0);
      poz_tinta = -1;
      directie = 0;
    } else {
      int dir = (poz_tinta > pozitieEncoder) ? 1 : -1;
      stepper.setSpeed(dir * 500);
    }
  } else {
    if (directie == 1 && pozitieEncoder < ENCODER_MAX)
      stepper.setSpeed(500);
    else if (directie == -1 && pozitieEncoder > ENCODER_MIN)
      stepper.setSpeed(-500);
    else
      stepper.setSpeed(0);
  }

  stepper.runSpeed();



  if (pozitieEncoder != ultimaPozitieSalvata && millis() - ultimaSalvareMillis > intervalSalvare) {
    EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
    EEPROM.commit();
    ultimaPozitieSalvata = pozitieEncoder;
    ultimaSalvareMillis = millis();
  }
}
