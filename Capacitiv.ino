// === COMPLET ===
#include <AccelStepper.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>

// === CONFIG WiFi ===
const char* ssid = "TP-Link_4DAC";
const char* password = "Pistol30";

// === PINI ===
#define ENCODER_A 26
#define ENCODER_B 27
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define TOUCH_PIN T0  // GPIO4

// === PARAMETRI ENCODER ===
const long ENCODER_MIN = 0;
const long ENCODER_MAX = 68414;
const float distMax_mm = 170.0;
volatile long pozitieEncoder = 0;

// === EEPROM ===
const int EEPROM_ADDR_ENCODER = 0;
const int EEPROM_ADDR_CAPMM = EEPROM_ADDR_ENCODER + sizeof(pozitieEncoder);
const int EEPROM_SIZE = 512;
unsigned long ultimaSalvareMillis = 0;
const unsigned long intervalSalvare = 2000;
long ultimaPozitieSalvata = 0;
int ultimaPozitieCap = 0;
bool calibrareInDesfasurare = false;
unsigned long ultimaVerificareCalibrare = 0;
const unsigned long intervalVerificare = 100;
const int VALOARE_MIN_CAP = 7;

// === MOTOR ===
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);
int directie = 0;

// === CAPACITIV ===
const int val_min = 7;
const int val_max = 15;
const float lungime_mm = 170.0;
float poz_filtrata = 0;
const float alpha = 0.2;

// === SERVER ===
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// === VARIABILA GLOBALA TINTA ===
static long poz_tinta = -1;


const char pagina_principala[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='ro'>
<head>
  <meta charset='UTF-8'>
  <title>Măsurare distanță - Senzor Capacitiv</title>
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
    <h2>Măsurarea distanței cu senzor capacitiv</h2>
    <p>
      În această secțiune este explorat <strong>senzorul capacitiv de deplasare</strong>, realizat dintr-o tijă metalică mobilă izolată introdusă într-un cilindru conductor. Acest tip de senzor este ideal pentru măsurarea fără contact a poziției.
    </p>
    <p>Prin utilizarea acestui stand, studentul are posibilitatea de a:</p>
    <ul>
      <li>Controla cu precizie poziția tijei folosind un motor pas cu pas și un encoder incremental;</li>
      <li>Observa în timp real variația valorii capacitive, afișată pe un grafic interactiv;</li>
      <li>Exporta datele măsurate în format .csv sau .jpg pentru analiză și documentare;</li>
      <li>Înțelege modul în care capacitatea electrică variază în funcție de poziția elementului mobil.</li>
    </ul>
    <p>
      Interfața web oferă un mediu interactiv și intuitiv pentru testarea senzorilor capacitivi, fiind potrivită pentru învățarea practică a principiilor de funcționare ale acestora.
    </p>
  </main>
</body>
</html>
)rawliteral";


// === Pagina control ===
const char pagina_control[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Control motor</title>
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
      padding: 10px 20px;
      margin: 5px;
      border-radius: 10px;
      background: #3498db;
      color: white;
      border: 2px solid #1f4f82;
      font-size: 16px;
      cursor: pointer;
    }
    button:hover { background: #2980b9; }
    #reset-button {
      background: #e74c3c;
      margin-left: 10px;
      border: 2px solid #a5281c;
    }
    #reset-button:hover {
      background: #c0392b;
    }
    #recalibrare-button {
      background: #27ae60;
      border: 2px solid #1e8449;
      margin-left: 10px;
    }
    #recalibrare-button:hover {
      background: #1e8449;
    }
    #progress-container {
      width: 80%; height: 25px;
      background: #ccc;
      margin: 20px auto;
      border-radius: 10px;
      border: 2px solid #888;
      overflow: hidden;
    }
    #progress-bar {
      height: 100%; width: 0%;
      background: #4caf50;
      color: white;
      line-height: 25px;
      font-weight: bold;
    }
    #info-box {
      margin-top: 20px;
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      justify-content: center;
      gap: 10px;
      font-size: 18px;
      max-width: 500px;
      margin-left: auto;
      margin-right: auto;
      padding: 15px;
      border: 1px solid #ccc;
      border-radius: 10px;
      background: #ffffff;
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
    #confirmBox, #confirmRecalibrareBox, #confirmRecalibrare2Box {
      display: none;
      position: fixed;
      top: 50%; left: 50%; transform: translate(-50%, -50%);
      background: white; padding: 20px;
      border: 2px solid #333; border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.5);
    }
    #confirmRecalibrareBox input {
      margin-top: 10px;
      padding: 5px;
      width: 100%;
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
  </style>
</head>
<body>
  <header>Control Motor + Poziție</header>
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
      <input type="number" id="input-mm" placeholder="Introduceți distanța [mm]" min="0" max="170" style="padding: 8px; width: 200px; border-radius: 8px;">
      <button onclick="gotoMM()">Mergi</button>
    </div>
    <div id="progress-container">
      <div id="progress-bar">0 mm</div>
    </div>
    <div id="info-box">
      <div><strong>⚙️ Valoare encoder:</strong> <span id="enc">-</span></div>
      <div><strong>📏 Distanța actuală:</strong> <span id="dist">-</span> mm</div>
      <div><strong>📶 Valoare senzor capacitiv:</strong> <span id="cap">-</span></div>
      <div id="motor-status"><strong>🔄 Stare motor:</strong> <span id="motor-dir">-</span></div>
    </div>
    <div id="log-container">
      <strong>🧾 Log comenzi:</strong>
      <div id="log-box"></div>
    </div>
    
    <div style="width: fit-content; margin: 30px auto 0 auto; text-align: center;">
  <canvas id="graficCapacitiv" width="500" height="320" style="background:#fff; border:1px solid #ccc;"></canvas>
  <div id="tooltip"></div>
  <div style="display: flex; justify-content: space-between; margin-top: 10px;">
    <button onclick="salveazaGrafic()" style="background:#2c3e50; border:2px solid #000; color:white;">
      💾 Salvează grafic (.jpg)
    </button>
    <button onclick="salveazaDateCSV()" style="background:#16a085; border:2px solid #0e6655; color:white;">
  📄 Salvează date (.csv)
    </button>
    <button onclick="resetGrafic()" style="background:#e67e22; border:2px solid #b35400;">
      🔁 Resetare Grafic
    </button>
    
  </div>
</div>
  </main>

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
    <p><strong>Poziționați tija la capătul inițial (valoare minimă capacitivă)</strong></p>
    <p>Valoare encoder: <span id="encRecal">-</span></p>
    <p>Valoare senzor capacitiv: <span id="capRecal">-</span></p>
    <p>
      Apăsați pe <strong>Calibrare automata</strong> pentru a deplasa motorul până la valoarea capacitivă de referință (7).
      În caz de eroare, folosiți <strong>Oprire urgentă</strong>.
    </p>
    <button onclick="send('c')">🔄 Calibrare automata</button>
    <button onclick="send('s')">⛔ Oprire urgentă</button>
    <button onclick="confirmRecalibrat()">✅ Confirmă recalibrare</button>
    <button onclick="closeRecalibrare2()">Anulează</button>
  </div>

<script>
    let ws;
    window.onload = () => {
      ws = new WebSocket('ws://' + location.host + '/ws');
      ws.onmessage = (e) => {
  const txt = e.data;
  let encoder = 0, dist = 0, cap = 0;
  try {
    encoder = parseInt(txt.split("Encoder:")[1].split("|")[0]);
    dist = parseFloat(txt.split("Dist:")[1].split("mm")[0]);
    cap = parseFloat(txt.split("Val cap:")[1]);
  } catch (err) {}
  document.getElementById("enc").innerText = encoder;
  document.getElementById("dist").innerText = dist.toFixed(1);
  document.getElementById("cap").innerText = cap;
  document.getElementById("encRecal").innerText = encoder;
  document.getElementById("capRecal").innerText = cap.toFixed(1);
  document.getElementById("progress-bar").style.width = Math.min(100, dist/170*100) + "%";
  document.getElementById("progress-bar").innerText = dist.toFixed(1) + " mm";

  adaugaPunct(dist, cap);

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
        else if (cmd === 'r') dir = "🚫 Reset";
        else if (cmd === 'p') dir = "📌 EEPROM";
        else if (cmd === 'x') dir = "🛠️ Recalibrare";
        else if (cmd === 'c') dir = "🔄 Calibrare automată";
        else if (cmd.startsWith("g")) dir = `🎯 Mers la ${cmd.substring(1)} mm`;
        document.getElementById("motor-dir").innerText = dir;
      }
    }
    function gotoMM() {
      const val = parseFloat(document.getElementById("input-mm").value);
      if (isNaN(val) || val < 0 || val > 170) {
        alert("Introduceți o valoare între 0 și 170 mm.");
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
      else if (cmd === 'r') descriere = "🚫 Reset encoder";
      else if (cmd === 'p') descriere = "📌 Salvare EEPROM";
      else if (cmd === 'x') descriere = "🛠️ Recalibrare encoder";
      else if (cmd === 'c') descriere = "🔄 Calibrare automată";
      else if (cmd.startsWith('g')) descriere = `🎯 Mers la ${cmd.substring(1)} mm`;
      else descriere = `Comandă necunoscută: ${cmd}`;
      box.innerHTML += `[${timestamp}] ${descriere}<br>`;
      box.scrollTop = box.scrollHeight;
    }
    const canvas = document.getElementById("graficCapacitiv");
  const ctx = canvas.getContext("2d");

  const MAX_POINTS = 100;
  let dataPoints = [];

  const tooltip = document.getElementById("tooltip");
let hoverPoint = null;

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
const scaleX = canvas.width / canvas.offsetWidth;
const scaleY = canvas.height / canvas.offsetHeight;
const mouseX = (e.clientX - rect.left) * scaleX;
const mouseY = (e.clientY - rect.top) * scaleY;

  const margineSt = 60;
  const margineJos = 270;
  const margineSus = 40;
  const latime = canvas.width;
  const inaltime = canvas.height;

  let gasit = false;

  for (const p of dataPoints) {
    const x = map(p.x, 0, 170, margineSt, latime - 20);
    const y = map(p.y, 0, 100, margineJos, margineSus);

    if (Math.abs(mouseX - x) < 8 && Math.abs(mouseY - y) < 8) {
      tooltip.style.display = "block";
      tooltip.style.left = `${rect.left + x}px`;
      tooltip.style.top = `${rect.top + y -30}px`;
      tooltip.innerText = `X: ${p.x.toFixed(1)} mm | Y: ${p.y.toFixed(1)}`;
      hoverPoint = { x, y, valoareX: p.x, valoareY: p.y };
      deseneazaGrafic();
      gasit = true;
      break;
    }
  }

  if (!gasit) {
    tooltip.style.display = "none";
    hoverPoint = null;
    deseneazaGrafic();
  }
}

function map(value, in_min, in_max, out_min, out_max) {
  return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

let puncteRosii = [];
let hoveredPoint = null;

function deseneazaGrafic() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  const margineSt = 60;
  const margineSus = 40;
  const margineJos = 270;
  const latime = canvas.width;
  const inaltime = canvas.height;

  const yMin = 6;
  const yMax = 17;

  // Axe + săgeți
  ctx.strokeStyle = "#000";
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(margineSt, margineSus);
  ctx.lineTo(margineSt, margineJos);
  ctx.lineTo(latime - 20, margineJos);
  ctx.stroke();

  // Săgeți
  ctx.beginPath();
  ctx.moveTo(margineSt - 5, margineSus + 5);
  ctx.lineTo(margineSt, margineSus);
  ctx.lineTo(margineSt + 5, margineSus + 5);
  ctx.stroke();

  ctx.beginPath();
  ctx.moveTo(latime - 25, margineJos - 5);
  ctx.lineTo(latime - 20, margineJos);
  ctx.lineTo(latime - 25, margineJos + 5);
  ctx.stroke();

  // Grilă Y (capacitivă)
  ctx.strokeStyle = "#e0e0e0";
  ctx.fillStyle = "#333";
  ctx.lineWidth = 1;
  ctx.font = "12px sans-serif";
  for (let yVal = yMin; yVal <= yMax; yVal += 1) {
    const y = map(yVal, yMin, yMax, margineJos, margineSus);
    ctx.beginPath();
    ctx.moveTo(margineSt, y);
    ctx.lineTo(latime - 20, y);
    ctx.stroke();
    ctx.fillText(yVal.toString(), margineSt - 40, y + 4);
  }

  // Grilă X (distanță)
  for (let xVal = 0; xVal <= 170; xVal += 10) {
    const x = map(xVal, 0, 170, margineSt, latime - 20);
    ctx.beginPath();
    ctx.moveTo(x, margineSus);
    ctx.lineTo(x, margineJos);
    ctx.stroke();
    if (xVal % 20 === 0)
      ctx.fillText(xVal.toString(), x - 10, margineJos + 15);
  }

  // Etichete axe
  ctx.fillStyle = "#000";
  ctx.font = "14px sans-serif";
  ctx.fillText("Distanță (mm)", latime / 2 - 40, margineJos + 35);
  ctx.save();
  ctx.translate(13, inaltime / 2 + 90);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("Valoare capacitivă (7–16)", 0, 0);
  ctx.restore();

  // Linie grafic
  ctx.strokeStyle = "#007bff";
  ctx.lineWidth = 2;
  ctx.beginPath();
  for (let i = 0; i < dataPoints.length; i++) {
    const x = map(dataPoints[i].x, 0, 170, margineSt, latime - 20);
    const y = map(dataPoints[i].y, yMin, yMax, margineJos, margineSus);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // Puncte roșii
  ctx.fillStyle = "#ff0000";
  puncteRosii.forEach(p => {
    const x = map(p.x, 0, 170, margineSt, latime - 20);
    const y = map(p.y, yMin, yMax, margineJos, margineSus);
    ctx.beginPath();
    ctx.arc(x, y, 3, 0, 2 * Math.PI);
    ctx.fill();
  });

  // Tooltip ghidaj (dacă există punct hoverat)
  if (hoverPoint) {
    ctx.strokeStyle = "#999";
    ctx.lineWidth = 1;
    ctx.setLineDash([5, 5]);
    ctx.beginPath();
    ctx.moveTo(hoverPoint.x, margineSus);
    ctx.lineTo(hoverPoint.x, margineJos);
    ctx.moveTo(margineSt, hoverPoint.y);
    ctx.lineTo(latime - 20, hoverPoint.y);
    ctx.stroke();
    ctx.setLineDash([]);

    const text = `X: ${hoverPoint.valoareX.toFixed(1)} mm | Y: ${hoverPoint.valoareY.toFixed(1)}`;
    const padding = 4;
    const textWidth = ctx.measureText(text).width;
    const boxX = hoverPoint.x + 10;
    const boxY = hoverPoint.y - 150;

    ctx.fillStyle = "#fff";
    ctx.fillRect(boxX - padding, boxY - 18, textWidth + 2 * padding, 20);
    ctx.strokeStyle = "#000";
    ctx.strokeRect(boxX - padding, boxY - 18, textWidth + 2 * padding, 20);
    ctx.fillStyle = "#000";
    ctx.font = "12px sans-serif";
    ctx.fillText(text, boxX, boxY - 4 + 10);
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
  link.download = 'GraficCapacitiv.jpg';
  link.href = tmpCanvas.toDataURL('image/jpeg', 1.0);
  link.click();
}

function salveazaDateCSV() {
  if (dataPoints.length === 0) {
    alert("Nu există date de salvat!");
    return;
  }

  let csvContent = '\uFEFF';
  csvContent += "Distanță (mm),Valoare capacitivă\n";
  dataPoints.forEach(p => {
    csvContent += `${p.x.toFixed(2)},${p.y.toFixed(2)}\n`;
  });

  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = "ValoriCapacitiv.csv";
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

</script>
</body>
</html>


)rawliteral";



// === Pagina help ===
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
    <a href="/senzor" class="btn-section">📶 Senzor Capacitiv</a>
  </main>
  
</body>
</html>
)rawliteral";

// Pagina ajutor: Motor
const char pagina_help_encoder[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Encoder - Ajutor</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; margin: 0; padding: 0; color: #2d3748; }
    header { background: #2d3748; color: white; padding: 1rem; text-align: center; font-size: 1.5em; }
    nav { background: #4a5568; padding: 0.5rem; text-align: center; }
    nav a {
      color: white; text-decoration: none;
      margin: 0 1rem; padding: 0.5rem 1rem;
      background: #2b6cb0; border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover { background: #2c5282; }
    main { padding: 2rem; }
    h1 { color: #2b6cb0; }
    p, ul { line-height: 1.6; max-width: 700px; margin: auto; }
    ul { padding-left: 1.5rem; }
  </style>
</head>
<body>
  <header>Encoder - Ajutor</header>
  <nav>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <h1>🌀 Encoder</h1>
    <p>Encoderul incremental folosit măsoară poziția absolută a motorului în funcție de numărul de impulsuri. Este folosit pentru a determina distanța parcursă de tija filetată. Poziția encoderului este utilizată pentru afișarea în interfață și controlul precis al poziției motorului.</p>

    <h2>📋 Specificații tehnice</h2>
    <ul>
      <li>Tip encoder: incremental, fotoelectric</li>
      <li>Rezoluție: 600 impulsuri per rotație (PPR)</li>
      <li>Tensiune de alimentare: 5–24 VDC</li>
      <li>Curent consumat: ≤ 40 mA</li>
      <li>Frecvență de răspuns: 0–100 kHz</li>
      <li>Viteză mecanică maximă: 5000 rpm</li>
      <li>Ieșire: NPN open collector, faze A și B</li>
      <li>Diametru corp: 38 mm</li>
      <li>Diametru ax: 6 mm</li>
      <li>Greutate: aprox. 180 g</li>
      <li>Protecție: IP52</li>
      <li>Temperatură de operare: -10°C până la 70°C</li>
    </ul>
  </main>
</body>
</html>
)rawliteral";

// Pagina ajutor: Potentiometru
const char pagina_help_motor[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Motor - Ajutor</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; margin: 0; padding: 0; color: #2d3748; }
    header { background: #2d3748; color: white; padding: 1rem; text-align: center; font-size: 1.5em; }
    nav { background: #4a5568; padding: 0.5rem; text-align: center; }
    nav a {
      color: white; text-decoration: none;
      margin: 0 1rem; padding: 0.5rem 1rem;
      background: #2b6cb0; border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover { background: #2c5282; }
    main { padding: 2rem; }
    h1 { color: #2b6cb0; }
    p, ul { line-height: 1.6; max-width: 700px; margin: auto; }
    ul { padding-left: 1.5rem; }
  </style>
</head>
<body>
  <header>Motor - Ajutor</header>
  <nav>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <h1>⚙️ Motor pas cu pas 28BYJ-48</h1>
    <p>Motorul pas cu pas folosit este modelul 28BYJ-48, un motor unipolar cu 4 faze. Este conectat la un driver ULN2003 și permite controlul precis al poziției prin impulsuri secvențiale. Este utilizat pentru deplasarea unei tije filetate pentru măsurători liniare.</p>

    <h2>📋 Specificații tehnice</h2>
    <ul>
      <li>Tensiune nominală: 5V DC</li>
      <li>Unghi pas: 5.625° / 64 (reducție internă)</li>
      <li>Raport de reducție: 1/64</li>
      <li>Număr pași/rev: 2048 (în modul half-step)</li>
      <li>Cuplu maxim: ~34 mNm</li>
      <li>Curent per fază: ~240 mA</li>
      <li>Viteză maximă recomandată: 10–15 rpm</li>
      <li>Conector: 5 pini (JST XH)</li>
    </ul>
  </main>
</body>
</html>
)rawliteral";


// Pagina ajutor: Rezistor
const char pagina_help_driver[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Driver motor - Ajutor</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; margin: 0; padding: 0; color: #2d3748; }
    header { background: #2d3748; color: white; padding: 1rem; text-align: center; font-size: 1.5em; }
    nav { background: #4a5568; padding: 0.5rem; text-align: center; }
    nav a {
      color: white; text-decoration: none;
      margin: 0 1rem; padding: 0.5rem 1rem;
      background: #2b6cb0; border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover { background: #2c5282; }
    main { padding: 2rem; }
    h1 { color: #2b6cb0; }
    p, ul { line-height: 1.6; max-width: 700px; margin: auto; }
  </style>
</head>
<body>
  <header>Driver Motor - Ajutor</header>
  <nav>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <h1>🔌 Driver motor ULN2003</h1>
    <p>Driverul de motor ULN2003 este un circuit integrat folosit pentru a comanda motoare pas cu pas sau alte sarcini inductive. El funcționează ca interfață între microcontroller (ESP32) și motor, amplificând semnalele logice de la microcontroller pentru a activa bobinele motorului.</p>

    <h2>📋 Specificații tehnice</h2>
    <ul>
      <li>Număr canale: 7 (folosite 4 pentru motor pas cu pas)</li>
      <li>Tensiune de intrare: 5V logic compatibil</li>
      <li>Tensiune de ieșire: până la 50V</li>
      <li>Curent maxim per canal: 500 mA</li>
      <li>Include diode de protecție împotriva tensiunilor inverse</li>
      <li>Conector standard de 5 pini pentru motorul 28BYJ-48</li>
      <li>Funcționează pe baza tranzistoarelor Darlington în pereche</li>
    </ul>
  </main>
</body>
</html>
)rawliteral";

const char pagina_help_esp[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>ESP32 - Ajutor</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; margin: 0; padding: 0; color: #2d3748; }
    header { background: #2d3748; color: white; padding: 1rem; text-align: center; font-size: 1.5em; }
    nav { background: #4a5568; padding: 0.5rem; text-align: center; }
    nav a {
      color: white; text-decoration: none;
      margin: 0 1rem; padding: 0.5rem 1rem;
      background: #2b6cb0; border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover { background: #2c5282; }
    main { padding: 2rem; }
    h1 { color: #2b6cb0; }
    p, ul { line-height: 1.6; max-width: 700px; margin: auto; }
    ul { padding-left: 1.5rem; }
  </style>
</head>
<body>
  <header>ESP32 - Ajutor</header>
  <nav>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <h1>📡 ESP32 LOLIN32 v1.0.0</h1>
    <p>Placa ESP32 LOLIN32 v1.0.0 este un microcontroller puternic cu suport WiFi și Bluetooth, folosit ca unitate principală de control pentru senzorul capacitiv, motorul pas cu pas și interfața web.</p>

    <h2>📋 Specificații tehnice</h2>
    <ul>
      <li>Procesor: Dual-core Tensilica LX6 @ 240 MHz</li>
      <li>Memorie: 520 KB SRAM, suport extern PSRAM</li>
      <li>Conectivitate: WiFi 802.11 b/g/n, Bluetooth v4.2 BR/EDR & BLE</li>
      <li>Număr GPIO-uri disponibile: ~25 (funcționalități multiple)</li>
      <li>Suport ADC (12 biți), DAC, PWM, UART, SPI, I2C, Touch</li>
      <li>Alimentare: 5V prin USB sau 3.3V direct</li>
      <li>Interfață USB: CH340/CP2102 (variază în funcție de model)</li>
      <li>Compatibil cu Arduino, PlatformIO, ESP-IDF</li>
    </ul>
  </main>
</body>
</html>
)rawliteral";

const char pagina_help_senzor[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
  <meta charset="UTF-8">
  <title>Senzor Capacitiv - Ajutor</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; margin: 0; padding: 0; color: #2d3748; }
    header { background: #2d3748; color: white; padding: 1rem; text-align: center; font-size: 1.5em; }
    nav { background: #4a5568; padding: 0.5rem; text-align: center; }
    nav a {
      color: white; text-decoration: none;
      margin: 0 1rem; padding: 0.5rem 1rem;
      background: #2b6cb0; border-radius: 5px;
      transition: background 0.3s;
    }
    nav a:hover { background: #2c5282; }
    main { padding: 2rem; }
    h1 { color: #2b6cb0; }
    p, ul { line-height: 1.6; max-width: 700px; margin: auto; }
    ul { padding-left: 1.5rem; }
  </style>
</head>
<body>
  <header>Senzor Capacitiv - Ajutor</header>
  <nav>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
  </nav>
  <main>
    <h1>📶 Senzor Capacitiv Cilindric</h1>
    <p>Senzorul capacitiv utilizat în acest proiect este realizat dintr-o tijă de aluminiu introdusă concentric într-o țeavă cilindrică de aluminiu, formând astfel un condensator de tip coaxial. Între cele două elemente metalice se află un strat de izolație termocontractibilă și un strat subțire de aer. Acest sistem permite măsurarea poziției liniare prin detectarea variației capacității rezultate din modificarea lungimii de suprapunere a celor două conductoare.</p>

    <h2>⚙️ Principiu de funcționare</h2>
    <p>Capacitatea unui condensator cilindric coaxial este dată de relația:</p>
    <p style="text-align: center;"><strong>C = (2πε₀ε<sub>r</sub> · L) / ln(b/a)</strong></p>
    <p>unde:</p>
    <ul>
      <li><strong>C</strong> este capacitatea electrică [F]</li>
      <li><strong>ε₀</strong> este permitivitatea vidului ≈ 8.85 × 10⁻¹² F/m</li>
      <li><strong>ε<sub>r</sub></strong> este permitivitatea relativă a mediului (≈ 1.2 pentru amestec de aer și izolație termocontractibilă)</li>
      <li><strong>L</strong> este lungimea de suprapunere între conductori (adică poziția tijei) [m]</li>
      <li><strong>a</strong> este raza exterioară a tijei (3 mm)</li>
      <li><strong>b</strong> este raza interioară a țevii (4 mm)</li>
    </ul>
    <p>Această capacitate crește liniar cu lungimea <strong>L</strong>, permițând estimarea poziției prin citirea indirectă a variațiilor de sarcină.</p>

    <h2>🔍 Achiziție cu ESP32</h2>
    <p>ESP32 integrează funcții de detecție capacitivă prin intermediul perifericului <code>TOUCH</code>. Când pinul tactil este conectat la un senzor capacitiv, acesta măsoară timpul necesar pentru descărcarea unui condensator intern prin impedanța circuitului extern. Timpul de descărcare scade odată cu creșterea capacității senzorului, rezultând o valoare numerică invers proporțională cu capacitatea efectivă.</p>
    <p>În practică:</p>
    <ul>
      <li>Valori mai <strong>mici</strong> indicate de <code>touchRead()</code> ⇒ <strong>capacitate mai mare</strong> ⇒ <strong>tija mai mult introdusă</strong></li>
      <li>Valori mai <strong>mari</strong> ⇒ <strong>capacitate mai mică</strong> ⇒ <strong>tija retrasă</strong></li>
    </ul>

    <h2>🛠️ Parametri constructivi</h2>
    <ul>
      <li>Țeavă exterioară (goala): aluminiu, Ø ext = 10 mm, Ø int = 8 mm, lungime: 290 mm</li>
      <li>Tijă interioară (plină): aluminiu, Ø = 6 mm, lungime: 310 mm</li>
      <li>Izolație: termocontractibil 0.5 mm + strat de aer ≈ 1.5 mm total</li>
      <li>Distanță de măsurare: 170 mm (deplasare liniară maximă)</li>
      <li>Valori ESP32 măsurate: ~7 (tija complet introdusă), ~15 (tija complet retrasă)</li>
    </ul>

    <p>Semnalul este filtrat software folosind o medie exponențială (EMA) pentru stabilitate și mapat liniar pe domeniul 0–170 mm.</p>
  </main>
</body>
</html>
)rawliteral";




// === FUNCȚII ===
float readTouchAvg(int pin, int n = 20) {
  long sum = 0;
  for (int i = 0; i < n; i++) {
    sum += touchRead(pin);
    delayMicroseconds(200);
  }
  return (float)sum / n;
}
int citesteSenzorCapacitiv() {
  return (int)readTouchAvg(TOUCH_PIN);
}

void IRAM_ATTR citireEncoder() {
  if (digitalRead(ENCODER_B) != digitalRead(ENCODER_A))
    pozitieEncoder++;
  else
    pozitieEncoder--;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ Conectat la WiFi: " + WiFi.localIP().toString());

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR_ENCODER, pozitieEncoder);
  EEPROM.get(EEPROM_ADDR_CAPMM, ultimaPozitieCap);
  ultimaPozitieSalvata = pozitieEncoder;

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), citireEncoder, RISING);

  stepper.setMaxSpeed(800);

ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    String msg;
    for (size_t i = 0; i < len; i++) msg += (char)data[i];

    if (msg == "f") {
      directie = 1;
    } else if (msg == "b") {
      directie = -1;
    } else if (msg == "s") {
      directie = 0;
      EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
      EEPROM.put(EEPROM_ADDR_CAPMM, (int)poz_filtrata);
      EEPROM.commit();
    } else if (msg == "r") {
      pozitieEncoder = 0;
      EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
      EEPROM.commit();
    } else if (msg == "p") {
      long e;
      int cap;
      EEPROM.get(EEPROM_ADDR_ENCODER, e);
      EEPROM.get(EEPROM_ADDR_CAPMM, cap);
      client->text("EEPROM -> Encoder: " + String(e) + " | Cap(mm): " + String(cap));
    } else if (msg.startsWith("g")) {
      float tinta_mm = msg.substring(1).toFloat();
      tinta_mm = constrain(tinta_mm, 0, distMax_mm);
      poz_tinta = (long)(tinta_mm * ENCODER_MAX / distMax_mm);
      directie = (poz_tinta > pozitieEncoder) ? 1 : -1;
      stepper.setSpeed(0);
      Serial.println("🎯 Comanda GOTO mm: " + String(tinta_mm) + " mm → encoder " + String(poz_tinta));
    }

    // === Recalibrare automată ===
    else if (msg == "c") {
      calibrareInDesfasurare = true;
      ultimaVerificareCalibrare = millis();
      Serial.println("🔧 Calibrare automată pornită...");
    }

    // === Confirmare manuală reset encoder ===
    else if (msg == "x") {
      int valCap = citesteSenzorCapacitiv();  // trebuie definită funcția
      if (valCap <= VALOARE_MIN_CAP) {
        pozitieEncoder = 0;
        EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
        EEPROM.commit();
        Serial.println("✅ Confirmare: Encoder resetat la 0 (calibrare manuală)");
      } else {
        Serial.println("⚠️ Valoarea senzorului prea mare, calibrul refuzat: " + String(valCap));
      }
    }
  }
});


  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_principala);
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
  server.on("/driver", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_driver);
  });
    server.on("/esp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_esp);
  });
    server.on("/senzor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pagina_help_senzor);
  });
  
  server.begin();
  Serial.println("✅ Server pornit");
}

void loop() {
  // === Calibrare automată ===
  if (calibrareInDesfasurare) {
    if (millis() - ultimaVerificareCalibrare > intervalVerificare) {
      ultimaVerificareCalibrare = millis();
      int valCap = citesteSenzorCapacitiv();
      Serial.println("🔍 Calibrare automată: valoare senzor = " + String(valCap));

      if (valCap <= VALOARE_MIN_CAP) {
        calibrareInDesfasurare = false;
        stepper.setSpeed(0);
        directie = 0;
        Serial.println("✅ Poziție minimă atinsă. Așteaptă confirmarea pentru resetare encoder.");
      } else {
        stepper.setSpeed(-500); // deplasare înapoi
      }
    }
    stepper.runSpeed();
    return;  // evită orice altă logică în timpul calibrării
  }

  // === Deplasare către o poziție țintă (gotoMM) ===
  if (poz_tinta != -1) {
    if (abs(pozitieEncoder - poz_tinta) <= 10) {
      stepper.setSpeed(0);
      poz_tinta = -1;
      directie = 0;
      Serial.println("✅ Ajuns la poziția țintă.");
    } else {
      int dir = (poz_tinta > pozitieEncoder) ? 1 : -1;
      stepper.setSpeed(dir * 500);
    }
  } else {
    // === Control manual din butoane f / b / s ===
    if (directie == 1 && pozitieEncoder < ENCODER_MAX)
      stepper.setSpeed(500);
    else if (directie == -1 && pozitieEncoder > ENCODER_MIN)
      stepper.setSpeed(-500);
    else
      stepper.setSpeed(0);
  }

  stepper.runSpeed();

  // === Transmisie periodică WebSocket ===
  static unsigned long tPrint = 0;
  if (millis() - tPrint > 1000) {
    float dist_mm = (float)pozitieEncoder * distMax_mm / ENCODER_MAX;
    float val = readTouchAvg(TOUCH_PIN);
    if (val <= 0 || isnan(val)) val = val_min;
    int int_val = (int)val;
    int poz_mm = (int_val - val_min) * 21.25;
    poz_mm = constrain(poz_mm, 0, (int)lungime_mm);
    poz_filtrata = alpha * poz_mm + (1 - alpha) * poz_filtrata;

String msg = "Encoder:" + String(pozitieEncoder);
msg += "|Dist:" + String(dist_mm, 1) + "mm";
msg += "|Val cap:" + String(val, 1);

    ws.textAll(msg);
    tPrint = millis();
  }

  // === Salvare automată encoder dacă e modificat ===
  if (pozitieEncoder != ultimaPozitieSalvata && millis() - ultimaSalvareMillis > intervalSalvare) {
    EEPROM.put(EEPROM_ADDR_ENCODER, pozitieEncoder);
    EEPROM.commit();
    ultimaPozitieSalvata = pozitieEncoder;
    ultimaSalvareMillis = millis();
    Serial.println("💾 Salvare automată encoder în EEPROM.");
  }
}


