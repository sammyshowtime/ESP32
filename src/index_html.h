// The single-page web UI, served straight from flash.
#pragma once

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="color-scheme" content="light dark">
<title>ESP32 Remote</title>
<style>
  :root {
    --bg: #f4f5f7;
    --card: #ffffff;
    --line: #e2e5ea;
    --text: #14171c;
    --muted: #6b7280;
    --accent: #2f6fed;
    --on: #16a34a;
    --off: #9aa1ab;
    --shadow: 0 1px 2px rgba(16, 20, 28, .06), 0 6px 20px rgba(16, 20, 28, .06);
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --bg: #0e1116;
      --card: #171b22;
      --line: #262c36;
      --text: #e7eaee;
      --muted: #9099a6;
      --accent: #5b8dff;
      --on: #35c46a;
      --off: #4a515c;
      --shadow: none;
    }
  }
  * { box-sizing: border-box; }
  body {
    margin: 0;
    padding: 24px 16px 40px;
    background: var(--bg);
    color: var(--text);
    font: 15px/1.5 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
    -webkit-font-smoothing: antialiased;
  }
  main { max-width: 640px; margin: 0 auto; }
  header {
    display: flex; align-items: center; gap: 10px;
    margin-bottom: 20px;
  }
  h1 { font-size: 20px; margin: 0; letter-spacing: -.01em; }
  #link {
    width: 9px; height: 9px; border-radius: 50%;
    background: var(--off); flex: none; transition: background .2s;
  }
  #link.up { background: var(--on); }
  .grid {
    display: grid; gap: 12px;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  }
  .card {
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 14px;
    box-shadow: var(--shadow);
    padding: 16px;
  }
  .switch {
    display: flex; align-items: center; justify-content: space-between;
    gap: 12px; width: 100%;
    font: inherit; color: inherit; text-align: left;
    cursor: pointer; user-select: none;
    -webkit-tap-highlight-color: transparent;
  }
  .switch:disabled { opacity: .5; cursor: progress; }
  .switch .name { font-weight: 550; }
  .switch .state { font-size: 12px; color: var(--muted); margin-top: 2px; }
  .knob {
    flex: none; width: 46px; height: 27px; border-radius: 999px;
    background: var(--off); position: relative; transition: background .18s;
  }
  .knob::after {
    content: ""; position: absolute; top: 3px; left: 3px;
    width: 21px; height: 21px; border-radius: 50%;
    background: #fff; transition: transform .18s;
    box-shadow: 0 1px 3px rgba(0, 0, 0, .3);
  }
  .on .knob { background: var(--on); }
  .on .knob::after { transform: translateX(19px); }
  .row { display: flex; gap: 12px; margin-top: 12px; }
  .row button {
    flex: 1; padding: 12px; border-radius: 12px;
    border: 1px solid var(--line); background: var(--card); color: var(--text);
    font: inherit; font-weight: 550; cursor: pointer;
  }
  .row button:active { transform: translateY(1px); }
  .dimmer { margin-top: 12px; }
  .dimmer .head {
    display: flex; justify-content: space-between; align-items: baseline;
    margin-bottom: 14px;
  }
  .dimmer .head b { font-weight: 550; }
  .dimmer .head span { color: var(--muted); font-variant-numeric: tabular-nums; }
  input[type=range] {
    width: 100%; accent-color: var(--accent); height: 24px;
  }
  footer {
    margin-top: 20px; color: var(--muted); font-size: 12.5px;
    display: flex; flex-wrap: wrap; gap: 4px 16px;
  }
  footer b { font-weight: 550; color: var(--text); font-variant-numeric: tabular-nums; }
  #error {
    margin-top: 14px; padding: 10px 12px; border-radius: 10px;
    background: #b91c1c; color: #fff; font-size: 13px; display: none;
  }
</style>
</head>
<body>
<main>
  <header><span id="link"></span><h1>ESP32 Remote</h1></header>

  <div class="grid" id="switches"></div>

  <div class="row">
    <button onclick="all(1)">All on</button>
    <button onclick="all(0)">All off</button>
  </div>

  <div class="card dimmer">
    <div class="head"><b>Dimmer</b><span id="dimval">0%</span></div>
    <input type="range" id="dim" min="0" max="255" value="0">
  </div>

  <footer>
    <span>Network <b id="f-ssid">-</b></span>
    <span>IP <b id="f-ip">-</b></span>
    <span>Signal <b id="f-rssi">-</b></span>
    <span>Uptime <b id="f-up">-</b></span>
  </footer>

  <div id="error"></div>
</main>

<script>
var token = localStorage.getItem('token') || '';
var dimMax = 255;
var holding = false;   // true while the slider is being dragged
var built = false;

function url(path) {
  return token ? path + (path.indexOf('?') < 0 ? '?' : '&') + 'token=' + encodeURIComponent(token) : path;
}

function fail(msg) {
  var el = document.getElementById('error');
  el.textContent = msg;
  el.style.display = msg ? 'block' : 'none';
  document.getElementById('link').classList.toggle('up', !msg);
}

function call(path) {
  return fetch(url(path), { method: 'POST' }).then(function (r) {
    if (r.status === 401) {
      var t = prompt('API token');
      if (t) { token = t; localStorage.setItem('token', t); }
      throw new Error('Unauthorized');
    }
    if (!r.ok) throw new Error('HTTP ' + r.status);
    return r.json();
  }).then(render).catch(function (e) { fail(e.message); });
}

function build(state) {
  var host = document.getElementById('switches');
  host.innerHTML = '';
  state.switches.forEach(function (sw, i) {
    var card = document.createElement('button');
    card.className = 'card switch';
    card.id = 'sw' + i;
    card.onclick = function () { toggle(i); };
    card.innerHTML = '<span><span class="name"></span><span class="state"></span></span><span class="knob"></span>';
    card.querySelector('.name').textContent = sw.name;
    host.appendChild(card);
  });
  dimMax = state.dimmer_max;
  document.getElementById('dim').max = dimMax;
  built = true;
}

function render(state) {
  if (!built || state.switches.length !== document.querySelectorAll('.switch').length) build(state);
  state.switches.forEach(function (sw, i) {
    var card = document.getElementById('sw' + i);
    card.classList.toggle('on', sw.on);
    card.querySelector('.state').textContent = sw.on ? 'On' : 'Off';
  });
  if (!holding) {
    document.getElementById('dim').value = state.dimmer;
    showDim(state.dimmer);
  }
  document.getElementById('f-ssid').textContent = state.ssid + (state.mode === 'ap' ? ' (AP)' : '');
  document.getElementById('f-ip').textContent = state.ip;
  document.getElementById('f-rssi').textContent = state.mode === 'ap' ? 'n/a' : state.rssi + ' dBm';
  document.getElementById('f-up').textContent = uptime(state.uptime_s);
  fail('');
}

function showDim(v) {
  document.getElementById('dimval').textContent = Math.round(v / dimMax * 100) + '%';
}

function uptime(s) {
  var d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600),
      m = Math.floor(s % 3600 / 60);
  if (d) return d + 'd ' + h + 'h';
  if (h) return h + 'h ' + m + 'm';
  if (m) return m + 'm ' + (s % 60) + 's';
  return s + 's';
}

function toggle(i) {
  var card = document.getElementById('sw' + i);
  card.classList.toggle('on');   // optimistic, the response corrects it
  call('/api/switch?index=' + i + '&value=toggle');
}

function all(v) { call('/api/all?value=' + v); }

var dim = document.getElementById('dim');
dim.addEventListener('input', function () {
  holding = true;
  showDim(+dim.value);
});
dim.addEventListener('change', function () {
  holding = false;
  call('/api/dimmer?value=' + dim.value);
});

function poll() {
  fetch(url('/api/state')).then(function (r) {
    if (!r.ok) throw new Error('HTTP ' + r.status);
    return r.json();
  }).then(render).catch(function (e) { fail(e.message); });
}

poll();
setInterval(poll, 2000);
</script>
</body>
</html>
)HTML";
