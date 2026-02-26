#include <Arduino.h>
#ifndef SYSTEM_WEBPAGE_HOME
#define SYSTEM_WEBPAGE_HOME

static const char kHomePageHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Nomad Device Monitor</title>
  <style>
    :root { --bg:#0f172a; --card:#111827; --muted:#94a3b8; --text:#e5e7eb; --accent:#22c55e; }
    body { margin:0; font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; background: radial-gradient(circle at top, #1f2937, var(--bg)); color:var(--text); }
    .wrap { max-width: 920px; margin: 0 auto; padding: 20px; }
    h1 { margin: 0 0 12px; font-size: 1.25rem; }
    .toolbar, .grid { display:grid; gap:12px; }
    .toolbar { grid-template-columns: 1fr auto auto; align-items:end; margin-bottom: 12px; }
    .grid { grid-template-columns: repeat(auto-fit, minmax(220px,1fr)); }
    .card { background: rgba(17,24,39,.9); border:1px solid rgba(148,163,184,.2); border-radius: 12px; padding: 14px; }
    .label { color:var(--muted); font-size:.8rem; margin-bottom:6px; }
    .value { font-size:1.15rem; font-weight:600; }
    select, button { background:#0b1220; color:var(--text); border:1px solid #334155; border-radius:8px; padding:8px 10px; }
    button { cursor:pointer; }
    .ok { color: var(--accent); }
    .warn { color: #f59e0b; }
    .footer { color: var(--muted); margin-top: 12px; font-size: .8rem; }
  </style>
</head>
<body>
  <div class="wrap">
    <h1>Nomad Device Monitor</h1>
    <div class="toolbar">
      <div class="card">
        <div class="label">Data Refresh Interval</div>
        <div class="value">
          <select id="refreshSec">
            <option value="5">5 sec</option>
            <option value="10">10 sec</option>
            <option value="15">15 sec</option>
            <option value="20">20 sec</option>
            <option value="30" selected>30 sec</option>
            <option value="45">45 sec</option>
            <option value="60">60 sec</option>
          </select>
        </div>
      </div>
      <button id="refreshNow">Refresh Now</button>
      <div class="card"><div class="label">Next Refresh</div><div class="value" id="nextRefresh">-</div></div>
    </div>

    <div class="grid">
      <div class="card"><div class="label">Current</div><div class="value" id="currentA">-</div></div>
      <div class="card"><div class="label">Amp Sensor Voltage</div><div class="value" id="ampVoltageV">-</div></div>
      <div class="card"><div class="label">Temperature</div><div class="value" id="tempC">-</div></div>
      <div class="card"><div class="label">Humidity</div><div class="value" id="humidityPct">-</div></div>
      <div class="card"><div class="label">Wi-Fi / IP</div><div class="value" id="wifi">-</div></div>
      <div class="card"><div class="label">Uptime</div><div class="value" id="uptime">-</div></div>
      <div class="card"><div class="label">CPU</div><div class="value" id="cpu">-</div></div>
      <div class="card"><div class="label">Heap Usage</div><div class="value" id="heap">-</div></div>
      <div class="card"><div class="label">Heap Min / MaxAlloc</div><div class="value" id="heap2">-</div></div>
      <div class="card"><div class="label">Sketch Usage</div><div class="value" id="flash">-</div></div>
    </div>
    <div class="footer" id="meta">Loading...</div>
  </div>
  <script>
    const els = Object.fromEntries([...document.querySelectorAll('[id]')].map(el => [el.id, el]));
    let timerId = null;
    let nextRefreshAt = 0;

    function fmtBytes(n) {
      if (!Number.isFinite(n)) return '-';
      const units = ['B', 'KB', 'MB'];
      let i = 0, v = n;
      while (v >= 1024 && i < units.length - 1) { v /= 1024; i++; }
      return `${v.toFixed(v >= 100 ? 0 : v >= 10 ? 1 : 2)} ${units[i]}`;
    }

    function fmtDuration(sec) {
      sec = Math.max(0, Math.floor(sec || 0));
      const d = Math.floor(sec / 86400);
      const h = Math.floor((sec % 86400) / 3600);
      const m = Math.floor((sec % 3600) / 60);
      const s = sec % 60;
      return (d ? `${d}d ` : '') + `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(s).padStart(2,'0')}`;
    }

    function updateCountdown() {
      if (!nextRefreshAt) { els.nextRefresh.textContent = '-'; return; }
      const sec = Math.max(0, Math.ceil((nextRefreshAt - Date.now()) / 1000));
      els.nextRefresh.textContent = `${sec}s`;
    }

    function scheduleRefresh() {
      if (timerId) clearTimeout(timerId);
      const secs = Math.max(5, Math.min(60, Number(els.refreshSec.value) || 30));
      const ms = secs * 1000;
      nextRefreshAt = Date.now() + ms;
      timerId = setTimeout(fetchStatus, ms);
      updateCountdown();
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status', { cache: 'no-store' });
        const d = await res.json();
        els.currentA.textContent = `${d.current_a.toFixed(3)} A`;
        els.ampVoltageV.textContent = `${d.amp_voltage_v.toFixed(4)} V`;
        els.tempC.textContent = d.aht_valid ? `${d.temp_c.toFixed(2)} C` : 'AHT unavailable';
        els.humidityPct.textContent = d.aht_valid ? `${d.humidity_pct.toFixed(2)} %` : 'AHT unavailable';
        els.wifi.textContent = `${d.wifi_connected ? 'Connected' : 'Disconnected'} / ${d.ip}`;
        els.uptime.textContent = fmtDuration(d.uptime_s);
        els.cpu.textContent = `${d.cpu_mhz} MHz | loop busy ${d.main_loop_busy_pct.toFixed(1)}%`;
        els.heap.textContent = `${fmtBytes(d.heap_used)} / ${fmtBytes(d.heap_total)} (${d.heap_used_pct.toFixed(1)}%)`;
        els.heap2.textContent = `min free ${fmtBytes(d.heap_min_free)} | max alloc ${fmtBytes(d.heap_max_alloc)}`;
        els.flash.textContent = `${fmtBytes(d.sketch_used)} / ${fmtBytes(d.sketch_total)} (${d.sketch_used_pct.toFixed(1)}%)`;
        els.meta.textContent = `Samples: ${d.sample_count} | Last sample: ${d.last_sample_ms} ms | Updated: ${new Date().toLocaleString()}`;
      } catch (e) {
        els.meta.textContent = `Refresh error: ${e}`;
      } finally {
        scheduleRefresh();
      }
    }

    els.refreshSec.addEventListener('change', scheduleRefresh);
    els.refreshNow.addEventListener('click', fetchStatus);
    setInterval(updateCountdown, 1000);
    fetchStatus();
  </script>
</body>
</html>
)HTML";

#endif // !SYSTEM_WEBPAGE_HOME
