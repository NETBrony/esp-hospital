#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>Setup WiFi</title>
  <style>
    :root { --p: #22d3ee; --bg: #0f172a; --g: rgba(30, 41, 59, 0.7); }
    body { margin:0; font-family:'Inter',sans-serif; background:var(--bg); color:white; display:flex; flex-direction:column; min-height:100vh; }
    .container { padding: 20px; max-width: 400px; margin: 0 auto; width: 100%; box-sizing: border-box; }
    h2 { text-align: center; font-weight: 300; letter-spacing: 1px; margin-bottom: 30px; }
    
    /* Glass Card */
    .card { background: var(--g); backdrop-filter: blur(12px); border: 1px solid rgba(255,255,255,0.1); border-radius: 20px; padding: 20px; margin-bottom: 15px; box-shadow: 0 4px 20px rgba(0,0,0,0.3); }
    
    /* List Items */
    .wifi-item { display: flex; justify-content: space-between; align-items: center; padding: 15px; border-bottom: 1px solid rgba(255,255,255,0.05); cursor: pointer; transition: 0.2s; }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item:active { background: rgba(34, 211, 238, 0.1); }
    .ssid { font-weight: 600; font-size: 1rem; }
    .rssi { font-size: 0.8rem; color: #94a3b8; }
    
    /* Form */
    input { width: 100%; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.1); padding: 12px; color: white; border-radius: 10px; margin-top: 10px; box-sizing: border-box; font-size: 1rem; outline: none; }
    input:focus { border-color: var(--p); }
    
    button { width: 100%; background: var(--p); color: #0f172a; border: none; padding: 14px; border-radius: 12px; font-weight: 800; font-size: 1rem; margin-top: 20px; cursor: pointer; transition: 0.2s; }
    button:active { transform: scale(0.98); opacity: 0.9; }
    button:disabled { background: #475569; color: #94a3b8; }

    .hidden { display: none; }
    .loader { border: 3px solid rgba(255,255,255,0.1); border-top: 3px solid var(--p); border-radius: 50%; width: 24px; height: 24px; animation: spin 1s linear infinite; margin: 20px auto; }
    @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <div class="container">
    <h2>WiFi Setup</h2>
    
    <div id="scan-view">
      <div class="card">
        <div id="wifi-list"></div>
        <div id="loading" class="loader"></div>
      </div>
      <button onclick="scanNetworks()">Rescan</button>
    </div>

    <div id="connect-view" class="hidden">
      <div class="card">
        <div style="margin-bottom: 10px; color: var(--p); font-weight:bold;" id="selected-ssid"></div>
        <input type="password" id="password" placeholder="Enter Password">
      </div>
      <div style="display:flex; gap:10px;">
        <button style="background:#334155; color:white;" onclick="showScan()">Back</button>
        <button onclick="connectWifi()">Connect</button>
      </div>
    </div>
  </div>

  <script>
    let selectedSSID = "";

    function showScan() {
      document.getElementById('scan-view').classList.remove('hidden');
      document.getElementById('connect-view').classList.add('hidden');
    }

    function showConnect(ssid) {
      selectedSSID = ssid;
      document.getElementById('selected-ssid').innerText = ssid;
      document.getElementById('scan-view').classList.add('hidden');
      document.getElementById('connect-view').classList.remove('hidden');
      document.getElementById('password').focus();
    }

    function scanNetworks() {
      const list = document.getElementById('wifi-list');
      const loader = document.getElementById('loading');
      list.innerHTML = '';
      loader.classList.remove('hidden');

      fetch('/scan').then(res => res.json()).then(data => {
        loader.classList.add('hidden');
        if(data.length === 0) list.innerHTML = '<div style="text-align:center; padding:20px;">No networks found</div>';
        
        data.forEach(net => {
          const div = document.createElement('div');
          div.className = 'wifi-item';
          // Lock icon if secured
          const lock = net.secure ? '🔒' : '';
          div.innerHTML = `<span class="ssid">${net.ssid}</span> <span class="rssi">${lock} ${net.rssi}dBm</span>`;
          div.onclick = () => showConnect(net.ssid);
          list.appendChild(div);
        });
      }).catch(e => {
        loader.classList.add('hidden');
        list.innerHTML = '<div style="text-align:center; color:#f87171;">Scan Failed</div>';
      });
    }

    function connectWifi() {
      const pass = document.getElementById('password').value;
      const btn = document.querySelector('#connect-view button:last-child');
      btn.disabled = true;
      btn.innerText = "Connecting...";

      // ส่งข้อมูลกลับไปหา ESP32
      const formData = new FormData();
      formData.append('ssid', selectedSSID);
      formData.append('pass', pass);

      fetch('/connect', { method: 'POST', body: formData }).then(res => {
        alert('Credentials saved! ESP32 is restarting...');
        btn.innerText = "Saved";
      });
    }

    // Start scanning on load
    window.onload = scanNetworks;
  </script>
</body>
</html>
)rawliteral";

#endif