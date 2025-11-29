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
<html lang=\"ro\">
<head><meta charset='UTF-8'>
<title>Laborator</title>
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
</style>
</head><body>
<header>Laborator Senzori și Traductoare</header>
<nav>
  <a href="http://192.168.1.215/index">🏠 Index</a>
  <a href="/">Acasă</a>
  <a href="/control">Control</a>
  <a href="/help">Ajutor</a>
</nav>
<main>
  <h2>Măsurarea distanței (inductiv)</h2>
  <p>Alege una dintre opțiunile din meniu pentru a continua.</p>
</main>
</body></html>
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
    <a href="/index">🏠 Index</a>
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




// Pagina de ajutor + encoder, motor, esp, driver (adaugabilă în continuare)
const char pagina_help[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head><meta charset="UTF-8"><title>Ajutor</title>
<style>
  body { font-family: 'Segoe UI', sans-serif; background: #f0f4f8; text-align: center; }
  header { background: #2d3748; color: white; padding: 1rem; font-size: 1.5em; }
  nav { background: #4a5568; padding: 0.5rem; }
  nav a {
    color: white; text-decoration: none; margin: 0 1rem;
    padding: 0.5rem 1rem; background: #2b6cb0; border-radius: 5px;
  }
  nav a:hover { background: #2c5282; }
  main { padding: 2rem; }
  .btn-section {
    display: inline-block; margin: 10px; padding: 15px 25px;
    font-size: 18px; background: #3498db; color: white;
    text-decoration: none; border-radius: 10px; border: none;
  }
  .btn-section:hover { background: #2980b9; }
</style>
</head>
<body>
  <header>Ajutor - Alege o componentă</header>
  <nav>
    <a href="http://192.168.1.215/index">🏠 Index</a>
    <a href="/">Acasă</a>
    <a href="/control">Control</a>
    <a href="/help">Ajutor</a>
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
<!DOCTYPE html><html lang=\"ro\"><head><meta charset=\"UTF-8\"><title>Ajutor - Encoder</title>
<style>body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; padding: 2rem; text-align: center; }
h1 { color: #2d3748; }
.content { max-width: 800px; margin: auto; text-align: left; font-size: 18px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
a { display: inline-block; margin-top: 20px; padding: 10px 20px; background: #3498db; color: white; text-decoration: none; border-radius: 8px; border: 2px solid #1f4f82; }
a:hover { background: #2980b9; }</style>
<body>
<h1>🌀 Encoder incremental</h1>
<div class=\"content\">
<p>Encoderul incremental utilizat este un senzor rotativ care generează impulsuri la fiecare pas de rotație. Canalele A și B sunt folosite pentru a detecta sensul mișcării.</p>
<ul>
  <li>✔️ Tip: Incremental, 2 canale</li>
  <li>⚙️ Rezoluție: 600–1000 PPR</li>
  <li>🔌 Alimentare: 5V</li>
  <li>📏 Utilizat pentru măsurarea poziției motorului pas cu pas</li>
</ul>
<a href=\"/help\">⏪ Înapoi</a>
</div></body></html>
)rawliteral";

const char pagina_help_motor[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang=\"ro\"><head><meta charset=\"UTF-8\"><title>Ajutor - Motor</title>
<style>body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; padding: 2rem; text-align: center; }
h1 { color: #2d3748; }
.content { max-width: 800px; margin: auto; text-align: left; font-size: 18px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
a { display: inline-block; margin-top: 20px; padding: 10px 20px; background: #3498db; color: white; text-decoration: none; border-radius: 8px; border: 2px solid #1f4f82; }
a:hover { background: #2980b9; }</style>
<body>
<h1>⚙️ Motor Pas cu Pas</h1>
<div class=\"content\">
<p>Motorul folosit este un 28BYJ-48 controlat de ESP32 prin driverul ULN2003.</p>
<ul>
  <li>🔁 Unghi pas: 5.625°</li>
  <li>⚙️ Raport de reducere: 64:1</li>
  <li>🌀 4096 pași pe tur complet</li>
  <li>🔌 Alimentare: 5V</li>
</ul>
<p>Este ideal pentru aplicații care necesită poziționare de precizie mică.</p>
<a href=\"/help\">⏪ Înapoi</a>
</div></body></html>
)rawliteral";

const char pagina_help_esp[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang=\"ro\"><head><meta charset=\"UTF-8\"><title>Ajutor - ESP32</title>
<style>body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; padding: 2rem; text-align: center; }
h1 { color: #2d3748; }
.content { max-width: 800px; margin: auto; text-align: left; font-size: 18px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
a { display: inline-block; margin-top: 20px; padding: 10px 20px; background: #3498db; color: white; text-decoration: none; border-radius: 8px; border: 2px solid #1f4f82; }
a:hover { background: #2980b9; }</style>
<body>
<h1>📡 ESP32</h1>
<div class=\"content\">
<p>ESP32 este un microcontroler WiFi+BT dual-core utilizat pentru control motor, citire encoder și afișare web.</p>
<ul>
  <li>🧠 240 MHz, dual-core</li>
  <li>📶 WiFi + BLE integrate</li>
  <li>🔌 GPIO multiple, ADC, PWM</li>
  <li>📘 Programabil prin Arduino IDE</li>
</ul>
<a href=\"/help\">⏪ Înapoi</a>
</div></body></html>
)rawliteral";

const char pagina_help_driver[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang=\"ro\"><head><meta charset=\"UTF-8\"><title>Ajutor - Driver</title>
<style>body { font-family: 'Segoe UI', sans-serif; background: #f8fafc; padding: 2rem; text-align: center; }
h1 { color: #2d3748; }
.content { max-width: 800px; margin: auto; text-align: left; font-size: 18px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
a { display: inline-block; margin-top: 20px; padding: 10px 20px; background: #3498db; color: white; text-decoration: none; border-radius: 8px; border: 2px solid #1f4f82; }
a:hover { background: #2980b9; }</style>
<body>
<h1>🔌 ULN2003 - Driver Motor</h1>
<div class=\"content\">
<p>ULN2003 este folosit pentru a comanda motorul pas cu pas. Este conectat la ESP32 și la cele 4 bobine ale motorului.</p>
<ul>
  <li>📦 7 canale cu tranzistoare Darlington</li>
  <li>🔋 Tensiune max: 50V</li>
  <li>⚡ Curent max: 500 mA/canal</li>
  <li>🛡️ Diode de protecție incluse</li>
</ul>
<a href=\"/help\">⏪ Înapoi</a>
</div></body></html>
)rawliteral";

