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
  #chip {
    margin-left: auto; font-size: 12px; font-weight: 550;
    padding: 3px 10px; border-radius: 999px;
    background: var(--line); color: var(--muted); display: none;
  }
  main.spent #switches, main.spent .row, main.spent .dimmer {
    opacity: .4; pointer-events: none;
  }
  .stickers { margin-top: 20px; }
  .stickers h2 { font-size: 16px; margin: 0 0 12px; }
  .stickers label { display: block; font-size: 13px; color: var(--muted); }
  .stickers input[type=text], .stickers input[type=number], .stickers select {
    display: block; width: 100%; margin-top: 4px; padding: 9px 10px;
    border-radius: 10px; border: 1px solid var(--line);
    background: var(--bg); color: var(--text); font: inherit;
  }
  .fields { display: grid; gap: 12px; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); }
  .fields .wide { grid-column: 1 / -1; }
  .opt { display: flex; gap: 10px; align-items: flex-start; margin-top: 12px; cursor: pointer; }
  .opt input { margin: 3px 0 0; width: 18px; height: 18px; accent-color: var(--accent); flex: none; }
  .opt b { display: block; color: var(--text); font-weight: 550; font-size: 14px; }
  .primary {
    margin-top: 16px; width: 100%; padding: 12px; border: 0; border-radius: 12px;
    background: var(--accent); color: #fff; font: inherit; font-weight: 600; cursor: pointer;
  }
  .primary:disabled { opacity: .6; cursor: progress; }
  .warn { margin: 0 0 12px; font-size: 13px; color: #b45309; }
  #passlist { margin-top: 16px; border-top: 1px solid var(--line); }
  .pass {
    display: flex; align-items: center; gap: 8px; flex-wrap: wrap;
    padding: 10px 0; border-bottom: 1px solid var(--line); font-size: 13px;
  }
  .pass code { font-weight: 600; font-size: 13px; }
  .pass .lbl { color: var(--muted); flex: 1; min-width: 80px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .tag { font-size: 11px; padding: 1px 7px; border-radius: 999px; background: var(--line); color: var(--muted); }
  .tag.bad { background: #b91c1c; color: #fff; }
  .pass button {
    padding: 4px 10px; border-radius: 8px; border: 1px solid var(--line);
    background: var(--card); color: var(--text); font: inherit; font-size: 12px; cursor: pointer;
  }
  .empty { color: var(--muted); font-size: 13px; padding: 10px 0; }

  /* Sticker sheet: hidden on screen, the only thing printed. Sizes are in
     millimetres so a sticker prints at the size picked. */
  #sheet { display: none; }
  @media print {
    @page { margin: 6mm; }
    body { background: #fff; padding: 0; }
    main { display: none; }
    #sheet { display: flex; flex-wrap: wrap; gap: 3mm; color: #000; }
    #sheet.perpage { display: block; }
    #sheet.perpage .sticker { break-after: page; margin: 0 auto; }
  }
  .sticker {
    width: var(--s); padding: calc(var(--s) * .04);
    border: .2mm dashed #999; border-radius: calc(var(--s) * .1);
    text-align: center; break-inside: avoid; background: #fff;
  }
  #sheet.perpage .sticker { border: 0; }
  .sticker svg { display: block; width: 100%; height: auto; }
  .sticker .t {
    font: 600 calc(var(--s) * .1) / 1.1 system-ui, sans-serif;
    white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
  }
  .sticker .c { font: calc(var(--s) * .07) / 1.2 ui-monospace, monospace; letter-spacing: .05em; }
  #error {
    margin-top: 14px; padding: 10px 12px; border-radius: 10px;
    background: #b91c1c; color: #fff; font-size: 13px; display: none;
  }
</style>
</head>
<body>
<main>
  <header><span id="link"></span><h1>ESP32 Remote</h1><span id="chip"></span></header>

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

  <section class="card stickers" id="stickers" hidden>
    <h2>Print sticker passes</h2>
    <p class="warn" id="nowarn" hidden>No API token is set, so anyone on this network can use the
      remote without a sticker. Set API_TOKEN in secrets.h to make stickers the only way in.</p>
    <div class="fields">
      <label class="wide">Text on sticker
        <input type="text" id="s-label" maxlength="24" value="ESP32 Remote"></label>
      <label>How many
        <input type="number" id="s-count" min="1" max="32" value="1"></label>
      <label>Size
        <select id="s-size">
          <option value="20">20 mm</option>
          <option value="25" selected>25 mm</option>
          <option value="30">30 mm</option>
          <option value="40">40 mm</option>
          <option value="50">50 mm</option>
        </select></label>
      <label>Link uses
        <select id="s-host">
          <option value="ip">IP address</option>
          <option value="mdns">hostname (.local)</option>
        </select></label>
    </div>
    <label class="opt"><input type="checkbox" id="s-lock" checked>
      <span><b>Lock to this location</b>Only works while the remote is on the network it is on
        now. Untick for a sticker that works anywhere.</span></label>
    <label class="opt"><input type="checkbox" id="s-once">
      <span><b>One-time pass</b>Works for a single tap, then stops working. Each sticker in the
        batch gets its own code.</span></label>
    <label class="opt"><input type="checkbox" id="s-page">
      <span><b>One sticker per page</b>For label printers that feed one sticker at a time.</span></label>
    <button class="primary" id="s-print">Print stickers</button>
    <div id="passlist"></div>
  </section>
</main>

<div id="sheet"></div>

<script>
// Minimal QR encoder: byte mode, error correction level M, versions 1-10
// (up to 213 bytes, plenty for a pass URL). Returns a square array of rows of
// booleans, true = dark.
function qrEncode(text) {
  var data = unescape(encodeURIComponent(text)), bytes = [];
  for (var i = 0; i < data.length; i++) bytes.push(data.charCodeAt(i));
  // [ecc codewords per block, blocks, data codewords per block] for level M;
  // versions 8+ add a second group with one more data codeword per block.
  var EC = [0, [10,1,16], [16,1,28], [26,1,44], [18,2,32], [24,2,43], [16,4,27],
            [18,4,31], [22,2,38,2,39], [22,3,36,2,37], [26,4,43,1,44]];
  var ver, t;
  for (ver = 1; ver <= 10; ver++) {
    t = EC[ver];
    var cap = t[1] * t[2] + (t[3] || 0) * (t[4] || 0);
    if (4 + (ver < 10 ? 8 : 16) + bytes.length * 8 <= cap * 8) break;
  }
  if (ver > 10) throw new Error('text too long for a QR code');

  var bits = [];
  function put(v, n) { for (var i = n - 1; i >= 0; i--) bits.push(v >>> i & 1); }
  put(4, 4); put(bytes.length, ver < 10 ? 8 : 16);
  bytes.forEach(function (b) { put(b, 8); });
  var capBits = cap * 8;
  put(0, Math.min(4, capBits - bits.length));
  put(0, (8 - bits.length % 8) % 8);
  for (var pad = 0xEC; bits.length < capBits; pad ^= 0xEC ^ 0x11) put(pad, 8);
  var cw = [];
  for (i = 0; i < bits.length; i += 8) {
    var v = 0;
    for (var j = 0; j < 8; j++) v = v << 1 | bits[i + j];
    cw.push(v);
  }

  // Reed-Solomon over GF(256), polynomial 0x11D.
  function mul(x, y) {
    var z = 0;
    for (var i = 7; i >= 0; i--) { z = z << 1 ^ (z >>> 7) * 0x11D; z ^= (y >>> i & 1) * x; }
    return z;
  }
  var deg = t[0], div = [], root = 1;
  for (i = 0; i < deg; i++) div.push(0);
  div[deg - 1] = 1;
  for (i = 0; i < deg; i++) {
    for (j = 0; j < deg; j++) { div[j] = mul(div[j], root); if (j + 1 < deg) div[j] ^= div[j + 1]; }
    root = mul(root, 2);
  }
  function ecc(block) {
    var r = div.map(function () { return 0; });
    block.forEach(function (b) {
      var f = b ^ r.shift(); r.push(0);
      div.forEach(function (c, k) { r[k] ^= mul(c, f); });
    });
    return r;
  }
  var blocks = [], pos = 0, groups = [[t[1], t[2]], [t[3] || 0, t[4] || 0]];
  groups.forEach(function (g) {
    for (var b = 0; b < g[0]; b++) { blocks.push(cw.slice(pos, pos + g[1])); pos += g[1]; }
  });
  var out = [], longest = blocks[blocks.length - 1].length;
  for (i = 0; i < longest; i++) blocks.forEach(function (b) { if (i < b.length) out.push(b[i]); });
  var eccs = blocks.map(ecc);
  for (i = 0; i < deg; i++) eccs.forEach(function (e) { out.push(e[i]); });

  // Function patterns.
  var size = ver * 4 + 17, m = [], fn = [];
  for (i = 0; i < size; i++) { m.push([]); fn.push([]); for (j = 0; j < size; j++) { m[i].push(false); fn[i].push(false); } }
  function set(x, y, dark) { m[y][x] = dark; fn[y][x] = true; }
  for (i = 0; i < size; i++) { set(6, i, i % 2 === 0); set(i, 6, i % 2 === 0); }
  [[3, 3], [size - 4, 3], [3, size - 4]].forEach(function (c) {
    for (var dy = -4; dy <= 4; dy++) for (var dx = -4; dx <= 4; dx++) {
      var x = c[0] + dx, y = c[1] + dy, d = Math.max(Math.abs(dx), Math.abs(dy));
      if (x >= 0 && x < size && y >= 0 && y < size) set(x, y, d !== 2 && d !== 4);
    }
  });
  if (ver > 1) {
    var last = size - 7, step = ver < 7 ? last - 6 : (last - 6) / 2;
    var al = ver < 7 ? [6, last] : [6, last - step, last];
    al.forEach(function (ay) { al.forEach(function (ax) {
      if ((ax === 6 && ay === 6) || (ax === 6 && ay === last) || (ax === last && ay === 6)) return;
      for (var dy = -2; dy <= 2; dy++) for (var dx = -2; dx <= 2; dx++)
        set(ax + dx, ay + dy, Math.max(Math.abs(dx), Math.abs(dy)) !== 1);
    }); });
  }
  function format(mask) {
    var d = mask, r = d;   // level M encodes as 00
    for (var i = 0; i < 10; i++) r = r << 1 ^ (r >>> 9) * 0x537;
    var b = (d << 10 | r) ^ 0x5412;
    function bit(i) { return (b >>> i & 1) === 1; }
    for (i = 0; i <= 5; i++) set(8, i, bit(i));
    set(8, 7, bit(6)); set(8, 8, bit(7)); set(7, 8, bit(8));
    for (i = 9; i < 15; i++) set(14 - i, 8, bit(i));
    for (i = 0; i < 8; i++) set(size - 1 - i, 8, bit(i));
    for (i = 8; i < 15; i++) set(8, size - 15 + i, bit(i));
    set(8, size - 8, true);
  }
  format(0);
  if (ver >= 7) {
    var r = ver;
    for (i = 0; i < 12; i++) r = r << 1 ^ (r >>> 11) * 0x1F25;
    var vb = ver << 12 | r;
    for (i = 0; i < 18; i++) {
      var a = size - 11 + i % 3, b = Math.floor(i / 3), dark = (vb >>> i & 1) === 1;
      set(a, b, dark); set(b, a, dark);
    }
  }

  // Data, in the zigzag column-pair order.
  var k = 0;
  for (var right = size - 1; right >= 1; right -= 2) {
    if (right === 6) right = 5;
    for (var vert = 0; vert < size; vert++) for (j = 0; j < 2; j++) {
      var x = right - j, up = (right + 1 & 2) === 0, y = up ? size - 1 - vert : vert;
      if (!fn[y][x] && k < out.length * 8) { m[y][x] = (out[k >>> 3] >>> 7 - (k & 7) & 1) === 1; k++; }
    }
  }

  // Try every mask and keep the one with the lowest penalty score.
  var MASKS = [
    function (x, y) { return (x + y) % 2 === 0; },
    function (x, y) { return y % 2 === 0; },
    function (x, y) { return x % 3 === 0; },
    function (x, y) { return (x + y) % 3 === 0; },
    function (x, y) { return (Math.floor(x / 3) + Math.floor(y / 2)) % 2 === 0; },
    function (x, y) { return x * y % 2 + x * y % 3 === 0; },
    function (x, y) { return (x * y % 2 + x * y % 3) % 2 === 0; },
    function (x, y) { return ((x + y) % 2 + x * y % 3) % 2 === 0; }
  ];
  function apply(mask) {
    for (var y = 0; y < size; y++) for (var x = 0; x < size; x++)
      if (!fn[y][x] && MASKS[mask](x, y)) m[y][x] = !m[y][x];
  }
  function penalty() {
    var p = 0, dark = 0, x, y;
    for (var pass = 0; pass < 2; pass++) for (y = 0; y < size; y++) {
      var run = 0, prev = null;
      for (x = 0; x < size; x++) {
        var c = pass ? m[x][y] : m[y][x];
        if (c === prev) { run++; if (run === 5) p += 3; else if (run > 5) p++; }
        else { run = 1; prev = c; }
      }
    }
    for (y = 0; y + 1 < size; y++) for (x = 0; x + 1 < size; x++) {
      var c0 = m[y][x];
      if (c0 === m[y][x + 1] && c0 === m[y + 1][x] && c0 === m[y + 1][x + 1]) p += 3;
    }
    for (y = 0; y < size; y++) for (x = 0; x < size; x++) if (m[y][x]) dark++;
    return p + Math.floor(Math.abs(dark * 20 - size * size * 10) / (size * size)) * 10;
  }
  var best = 0, bestScore = Infinity;
  for (var mask = 0; mask < 8; mask++) {
    apply(mask); format(mask);
    var score = penalty();
    if (score < bestScore) { best = mask; bestScore = score; }
    apply(mask);
  }
  apply(best); format(best);
  return m;
}

// The QR code as an SVG string with a quiet zone of `quiet` modules.
function qrSvg(text, quiet) {
  var m = qrEncode(text), n = m.length + quiet * 2, d = '';
  m.forEach(function (row, y) { row.forEach(function (dark, x) {
    if (dark) d += 'M' + (x + quiet) + ' ' + (y + quiet) + 'h1v1h-1z';
  }); });
  return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ' + n + ' ' + n +
    '" shape-rendering="crispEdges"><rect width="100%" height="100%" fill="#fff"/><path d="' + d + '" fill="#000"/></svg>';
}

// --- Remote ---------------------------------------------------------------

var token = localStorage.getItem('token') || '';
var pass = (location.search.match(/[?&]pass=([^&]+)/) || [])[1] || '';
var dimMax = 255;
var holding = false;   // true while the slider is being dragged
var built = false;
var state = null;
var admin = false;

// A sticker link carries ?pass=; everyone else uses the API token.
function url(path) {
  var cred = pass ? 'pass=' + pass : token ? 'token=' + encodeURIComponent(token) : '';
  return cred ? path + (path.indexOf('?') < 0 ? '?' : '&') + cred : path;
}

// Resolves to the JSON body, or rejects with the server's error message.
function req(method, path) {
  return fetch(url(path), { method: method }).then(function (r) {
    return r.json().catch(function () { return {}; }).then(function (body) {
      if (r.ok) return body;
      var e = new Error(body.error || 'HTTP ' + r.status);
      e.status = r.status;
      throw e;
    });
  });
}

// A spent or invalid sticker is a final state, not a connection problem.
function denied(e) {
  if (e.status !== 403 || !pass) return false;
  document.querySelector('main').classList.add('spent');
  showChip('Sticker not valid');
  fail(e.message.charAt(0).toUpperCase() + e.message.slice(1) + '.');
  document.getElementById('link').classList.remove('up');
  return true;
}

function showChip(text) {
  var chip = document.getElementById('chip');
  chip.textContent = text;
  chip.style.display = text ? 'inline-block' : 'none';
}

function fail(msg) {
  var el = document.getElementById('error');
  el.textContent = msg;
  el.style.display = msg ? 'block' : 'none';
  document.getElementById('link').classList.toggle('up', !msg);
}

function call(path) {
  return req('POST', path).then(render).catch(function (e) {
    if (denied(e)) return;
    if (e.status === 401) {
      var t = prompt('API token');
      if (t) { token = t; localStorage.setItem('token', t); }
    }
    fail(e.message);
  });
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

function render(s) {
  state = s;
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
  if (state.pass) {
    var p = state.pass;
    showChip(p.once ? (p.used ? 'One-time pass used' : 'One-time pass') : 'Sticker pass');
    document.querySelector('main').classList.toggle('spent', p.once && p.used);
    if (p.once && p.used) fail('This one-time sticker has been used. Thanks!');
  }
  var isAdmin = state.access === 'admin';
  if (isAdmin && !admin) loadPasses();
  admin = isAdmin;
  document.getElementById('stickers').hidden = !admin;
  document.getElementById('nowarn').hidden = state.auth_required;
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

var spent = false;
function poll() {
  if (spent) return;
  req('GET', '/api/state').then(render).catch(function (e) {
    if (denied(e)) { spent = true; return; }   // no point polling a dead pass
    fail(e.message);
  });
}

// --- Sticker passes (admin only) ----------------------------------------

function el(tag, cls, text) {
  var e = document.createElement(tag);
  if (cls) e.className = cls;
  if (text !== undefined) e.textContent = text;
  return e;
}

function passUrl(code) {
  var host = document.getElementById('s-host').value === 'mdns'
    ? state.hostname + '.local' : state.ip;
  var port = location.port ? ':' + location.port : '';
  return 'http://' + host + port + '/?pass=' + code;
}

function renderPasses(list) {
  var host = document.getElementById('passlist');
  host.innerHTML = '';
  if (!list.passes.length) {
    host.appendChild(el('div', 'empty', 'No stickers printed yet.'));
    return;
  }
  list.passes.slice().reverse().forEach(function (p) {
    var row = el('div', 'pass');
    row.appendChild(el('code', '', p.code));
    row.appendChild(el('span', 'lbl', p.label));
    row.appendChild(el('span', 'tag', p.locked ? 'This location' : 'Anywhere'));
    row.appendChild(el('span', 'tag', p.once ? 'One-time' : 'Reusable'));
    if (p.used) row.appendChild(el('span', 'tag bad', 'Used'));
    else if (!p.here) row.appendChild(el('span', 'tag bad', 'Other location'));
    if (!p.used) {
      var again = el('button', '', 'Reprint');
      again.onclick = function () { printStickers([p]); };
      row.appendChild(again);
    }
    var del = el('button', '', 'Revoke');
    del.onclick = function () {
      if (!confirm('Revoke ' + p.code + '? Its sticker will stop working.')) return;
      req('DELETE', '/api/passes?code=' + p.code).then(renderPasses)
        .catch(function (e) { fail(e.message); });
    };
    row.appendChild(del);
    host.appendChild(row);
  });
}

function loadPasses() {
  req('GET', '/api/passes').then(renderPasses).catch(function (e) { fail(e.message); });
}

function printStickers(list) {
  var sheet = document.getElementById('sheet');
  sheet.innerHTML = '';
  sheet.className = document.getElementById('s-page').checked ? 'perpage' : '';
  sheet.style.setProperty('--s', document.getElementById('s-size').value + 'mm');
  list.forEach(function (p) {
    var st = el('div', 'sticker');
    st.innerHTML = qrSvg(passUrl(p.code), 2);
    if (p.label) st.appendChild(el('div', 't', p.label));
    st.appendChild(el('div', 'c', p.code));
    sheet.appendChild(st);
  });
  window.print();
}

document.getElementById('s-print').onclick = function () {
  var btn = this;
  var n = Math.max(1, Math.min(32, +document.getElementById('s-count').value || 1));
  var q = '/api/passes?count=' + n +
    '&lock=' + (document.getElementById('s-lock').checked ? 1 : 0) +
    '&once=' + (document.getElementById('s-once').checked ? 1 : 0) +
    '&label=' + encodeURIComponent(document.getElementById('s-label').value);
  btn.disabled = true;
  req('POST', q).then(function (list) {
    renderPasses(list);
    if (list.created.length < n) fail('Storage full: only ' + list.created.length + ' sticker(s) made. Revoke old ones to free space.');
    printStickers(list.created);
  }).catch(function (e) { fail(e.message); }).then(function () { btn.disabled = false; });
};

poll();
setInterval(poll, 2000);
</script>
</body>
</html>
)HTML";
