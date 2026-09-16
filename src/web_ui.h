#pragma once

#include <Arduino.h>

namespace drybox_ui {
const char kIndexHtml[] PROGMEM = R"DRYBOX(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta name="theme-color" content="#17201d">
  <title>Drybox</title>
  <style>
    :root {
      --ink: #17201d;
      --muted: #65716c;
      --surface: #ffffff;
      --line: #d9dfdc;
      --temperature: #d9563f;
      --temperature-two: #e79b32;
      --humidity: #128579;
      --pressure: #3973b9;
      --good: #2a7b4f;
      --bad: #b54035;
    }
    * { box-sizing: border-box; letter-spacing: 0; }
    body {
      margin: 0;
      color: var(--ink);
      background-color: #f3f6f4;
      background-image:
        linear-gradient(rgba(23, 32, 29, .035) 1px, transparent 1px),
        linear-gradient(90deg, rgba(23, 32, 29, .035) 1px, transparent 1px);
      background-size: 32px 32px;
      font-family: Bahnschrift, "Aptos Display", "Segoe UI", sans-serif;
    }
    button, select, a { font: inherit; }
    .topbar {
      color: #f8fbf9;
      background: #17201d;
      border-bottom: 4px solid #e0a533;
    }
    .topbar.drying {
      background: linear-gradient(120deg, #8f2117, #d14a25, #f09a2e, #b72f1e, #8f2117);
      background-size: 320% 320%;
      border-bottom-color: #ffb347;
      animation: dryingHeaderGlow 7s ease-in-out infinite;
    }
    @keyframes dryingHeaderGlow {
      0%, 100% { background-position: 0% 50%; }
      50% { background-position: 100% 50%; }
    }
    @media (prefers-reduced-motion: reduce) {
      .topbar.drying { animation: none; background-position: 50% 50%; }
    }
    .topbar-inner {
      width: min(1180px, calc(100% - 32px));
      min-height: 76px;
      margin: 0 auto;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 20px;
    }
    .identity { display: flex; align-items: center; gap: 13px; min-width: 0; }
    .brand-mark {
      width: 38px;
      height: 38px;
      flex: 0 0 38px;
      display: grid;
      place-items: center;
      color: #17201d;
      background: #e0a533;
      border-radius: 4px;
      font-weight: 800;
      font-size: 20px;
    }
    h1 { margin: 0; font-size: 22px; line-height: 1; font-weight: 700; }
    .host { margin-top: 6px; color: #aeb9b4; font: 12px Consolas, monospace; }
    .header-drying-status {
      display: inline-flex;
      align-items: center;
      gap: 7px;
      min-height: 30px;
      padding: 5px 11px;
      color: #dfe7e3;
      border: 1px solid #40504a;
      border-radius: 999px;
      font-size: 12px;
      font-weight: 700;
      white-space: nowrap;
    }
    .header-drying-status::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: #8b9892; }
    .header-drying-status.drying { border-color: #985047; color: #ffd4ce; }
    .header-drying-status.drying::before { background: #e16558; box-shadow: 0 0 0 3px rgba(225, 101, 88, .15); }
    .header-drying-status.safety, .header-drying-status.timer-expired { border-color: #a97532; color: #ffe4ad; }
    .header-drying-status.safety::before, .header-drying-status.timer-expired::before { background: #e0a533; }
    .connection {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      min-height: 30px;
      padding: 5px 10px;
      border: 1px solid #40504a;
      border-radius: 999px;
      color: #dfe7e3;
      font-size: 12px;
      white-space: nowrap;
    }
    .connection-dot { width: 8px; height: 8px; border-radius: 50%; background: #d2a53d; }
    .connection.online .connection-dot { background: #52bd7d; box-shadow: 0 0 0 3px rgba(82, 189, 125, .15); }
    .connection.offline .connection-dot { background: #e16558; }
    main { width: min(1180px, calc(100% - 32px)); margin: 26px auto 48px; }
    .metrics {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 12px;
    }
    .metric {
      position: relative;
      min-width: 0;
      min-height: 142px;
      padding: 18px;
      overflow: hidden;
      background: var(--surface);
      border: 1px solid var(--line);
      border-radius: 6px;
      box-shadow: 0 7px 20px rgba(24, 38, 33, .055);
    }
    .metric::before { content: ""; position: absolute; inset: 0 auto 0 0; width: 4px; background: var(--accent); }
    .metric-label { color: var(--muted); font-size: 12px; font-weight: 700; text-transform: uppercase; }
    .metric-value { margin-top: 18px; font-size: 35px; line-height: 1; font-weight: 700; white-space: nowrap; }
    .metric-unit { margin-left: 4px; color: var(--muted); font-size: 15px; font-weight: 500; }
    .metric-source { margin-top: 12px; color: var(--muted); font-size: 12px; }
    .metric.temperature-pair { grid-column: span 2; }
    .temperature-readings { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 22px; margin-top: 18px; }
    .temperature-reading { min-width: 0; }
    .temperature-reading-label { color: var(--muted); font-size: 11px; font-weight: 700; text-transform: uppercase; }
    .temperature-reading-value { margin-top: 8px; font-size: 35px; line-height: 1; font-weight: 700; white-space: nowrap; }
    .drying-control {
      margin-top: 12px;
      padding: 17px;
      background: var(--surface);
      border: 1px solid var(--line);
      border-left: 4px solid #d2a53d;
      border-radius: 6px;
      box-shadow: 0 7px 20px rgba(24, 38, 33, .045);
    }
    .drying-control-head { display: flex; align-items: baseline; justify-content: space-between; gap: 14px; }
    .drying-control-head h2 { margin: 0; font-size: 20px; }
    .drying-state { color: var(--muted); font-size: 12px; font-weight: 700; text-transform: uppercase; }
    .drying-fields { display: grid; grid-template-columns: minmax(150px, 1fr) minmax(150px, 1fr) auto; gap: 12px; align-items: end; margin-top: 16px; }
    .drying-field { min-width: 0; }
    .drying-field label { display: block; margin-bottom: 6px; color: var(--muted); font-size: 12px; font-weight: 700; }
    .drying-field input {
      width: 100%;
      height: 38px;
      padding: 0 10px;
      color: var(--ink);
      background: #fff;
      border: 1px solid #bcc6c1;
      border-radius: 5px;
      font: inherit;
    }
    .drying-toggle {
      min-width: 132px;
      height: 38px;
      padding: 0 15px;
      color: #fff;
      background: #28764f;
      border: 0;
      border-radius: 5px;
      font-weight: 700;
      cursor: pointer;
    }
    .drying-toggle.stop { background: #b54035; }
    .drying-toggle:disabled { cursor: not-allowed; opacity: .45; }
    .drying-safety { min-height: 18px; margin-top: 12px; color: var(--muted); font-size: 12px; }
    .drying-safety.safe { color: var(--good); }
    .drying-safety.unsafe { color: var(--bad); font-weight: 700; }
    .outputs-head { margin: 28px 0 12px; display: flex; align-items: baseline; justify-content: space-between; gap: 12px; }
    .outputs-head h2 { margin: 0; font-size: 20px; }
    .outputs-head span { color: var(--muted); font-size: 12px; }
    .outputs {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 12px;
    }
    .output {
      min-width: 0;
      padding: 15px 17px;
      background: var(--surface);
      border: 1px solid var(--line);
      border-radius: 6px;
      box-shadow: 0 7px 20px rgba(24, 38, 33, .045);
    }
    .output-label { color: var(--muted); font-size: 12px; font-weight: 700; text-transform: uppercase; }
    .output-value { display: flex; align-items: baseline; gap: 5px; margin-top: 10px; line-height: 1; white-space: nowrap; }
    .output-percent { font-size: 27px; font-weight: 700; }
    .output-unit { color: var(--muted); font-size: 15px; font-weight: 500; }
    .output-raw { color: var(--muted); font-size: 12px; font-weight: 500; }
    .history-head {
      margin: 28px 0 12px;
      display: flex;
      align-items: end;
      justify-content: space-between;
      gap: 16px;
    }
    .history-title h2 { margin: 0; font-size: 20px; }
    .history-meta { min-height: 18px; margin-top: 5px; color: var(--muted); font-size: 12px; }
    .actions { display: flex; align-items: center; gap: 8px; }
    .range-select, .download {
      height: 36px;
      border: 1px solid #bcc6c1;
      border-radius: 5px;
      background: #fff;
      color: var(--ink);
    }
    .range-select { padding: 0 30px 0 10px; }
    .download { display: inline-flex; align-items: center; padding: 0 12px; text-decoration: none; font-size: 13px; font-weight: 700; }
    .download:hover, .range-select:hover { border-color: #6e7d76; }
    .chart-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; }
    .chart-panel {
      min-width: 0;
      padding: 17px;
      background: var(--surface);
      border: 1px solid var(--line);
      border-radius: 6px;
      box-shadow: 0 7px 20px rgba(24, 38, 33, .045);
    }
    .chart-panel.combined-history { grid-column: 1 / -1; }
    .chart-header { display: flex; justify-content: space-between; gap: 12px; align-items: center; margin-bottom: 12px; }
    .chart-header h3 { margin: 0; font-size: 14px; }
    .legend { display: flex; flex-wrap: wrap; justify-content: end; gap: 12px; color: var(--muted); font-size: 11px; }
    .legend-item { display: inline-flex; align-items: center; gap: 5px; white-space: nowrap; }
    .legend-line { width: 14px; height: 3px; border-radius: 2px; background: var(--series); }
    .legend-line.dotted { height: 0; border-top: 2px dotted var(--series); background: transparent; }
    .chart-wrap { position: relative; }
    canvas { display: block; width: 100%; height: 300px; touch-action: pan-y; }
    .chart-tooltip {
      position: absolute;
      z-index: 2;
      min-width: 178px;
      max-width: calc(100% - 16px);
      padding: 9px 11px;
      color: #f7faf8;
      background: rgba(23, 32, 29, .96);
      border: 1px solid #53635c;
      border-radius: 4px;
      box-shadow: 0 8px 18px rgba(10, 18, 15, .2);
      font-size: 11px;
      line-height: 1.45;
      pointer-events: none;
    }
    .chart-tooltip[hidden] { display: none; }
    .chart-tooltip-title { margin-bottom: 4px; color: #b9c6c0; font-weight: 700; }
    .chart-tooltip-row { display: flex; align-items: baseline; gap: 6px; white-space: nowrap; }
    .chart-tooltip-swatch { width: 12px; height: 3px; flex: 0 0 12px; border-radius: 2px; background: var(--series); }
    .storage-band {
      margin-top: 12px;
      padding: 15px 17px;
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 16px;
      color: #dfe7e3;
      background: #22302b;
      border-radius: 6px;
    }
    .storage-label { color: #9caaa4; font-size: 11px; text-transform: uppercase; }
    .storage-value { margin-top: 5px; font-size: 14px; font-weight: 700; overflow-wrap: anywhere; }
    .error {
      display: none;
      margin-bottom: 12px;
      padding: 10px 12px;
      color: #781e18;
      background: #fde9e6;
      border: 1px solid #efb8b0;
      border-radius: 5px;
      font-size: 13px;
    }
    .error.visible { display: block; }
    @media (max-width: 850px) {
      .metrics { grid-template-columns: repeat(2, minmax(0, 1fr)); }
      .outputs { grid-template-columns: repeat(2, minmax(0, 1fr)); }
      .storage-band { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    }
    @media (max-width: 620px) {
      .topbar-inner, main { width: min(100% - 20px, 1180px); }
      .topbar-inner { min-height: 68px; }
      .connection-label { display: none; }
      .header-drying-status { padding: 5px 8px; }
      .metrics, .chart-grid { grid-template-columns: 1fr; }
      .outputs { grid-template-columns: 1fr; }
      .metric.temperature-pair { grid-column: auto; }
      .history-head { align-items: stretch; flex-direction: column; }
      .actions { display: grid; grid-template-columns: 1fr auto; }
      .range-select { width: 100%; }
      canvas { height: 250px; }
      .drying-fields { grid-template-columns: 1fr 1fr; }
      .drying-toggle { grid-column: 1 / -1; width: 100%; }
    }
  </style>
</head>
<body>
  <header class="topbar">
    <div class="topbar-inner">
      <div class="identity">
        <div class="brand-mark">D</div>
        <div><h1>Drybox</h1><div class="host">drybox.local</div></div>
      </div>
      <div id="headerDryingStatus" class="header-drying-status">IDLE</div>
      <div id="connection" class="connection"><span class="connection-dot"></span><span class="connection-label">Connecting</span></div>
    </div>
  </header>
  <main>
    <div id="error" class="error" role="alert"></div>
    <section class="metrics" aria-label="Current readings">
      <article class="metric temperature-pair" style="--accent:var(--temperature)">
        <div class="metric-label">Air temperature</div>
        <div class="temperature-readings">
          <div class="temperature-reading"><div class="temperature-reading-label">AHT20</div><div class="temperature-reading-value"><span id="ahtTemp">--</span><span class="metric-unit">°C</span></div></div>
          <div class="temperature-reading"><div class="temperature-reading-label">BMP280</div><div class="temperature-reading-value"><span id="bmpTemp">--</span><span class="metric-unit">°C</span></div></div>
        </div>
      </article>
      <article class="metric" style="--accent:var(--humidity)"><div class="metric-label">Relative humidity</div><div class="metric-value"><span id="humidity">--</span><span class="metric-unit">% RH</span></div><div class="metric-source">AHT20</div></article>
      <article class="metric" style="--accent:var(--pressure)"><div class="metric-label">Pressure</div><div class="metric-value"><span id="pressure">--</span><span class="metric-unit">hPa</span></div><div class="metric-source">BMP280</div></article>
    </section>

    <section class="drying-control" aria-labelledby="dryingHeading">
      <div class="drying-control-head"><h2 id="dryingHeading">Drying cycle</h2><span id="dryingState" class="drying-state">Idle</span></div>
      <div class="drying-fields">
        <div class="drying-field"><label for="setTemperature">Set temperature (°C)</label><input id="setTemperature" type="number" min="0" max="65" step="0.5" value="45"></div>
        <div class="drying-field"><label for="dryingDuration">Timer (minutes)</label><input id="dryingDuration" type="number" min="1" max="120" step="1" value="30"></div>
        <button id="dryingToggle" class="drying-toggle" type="button">Start drying</button>
      </div>
      <div id="dryingSafety" class="drying-safety">Temperature safety: waiting for both sensors</div>
    </section>

    <section aria-labelledby="outputsHeading">
      <div class="outputs-head"><h2 id="outputsHeading">Outputs</h2><span>PWM duty range 0–255</span></div>
      <div class="outputs">
        <article class="output"><div class="output-label">Heater fan</div><div class="output-value"><span id="heaterFanPwmPercent" class="output-percent">--</span><span class="output-unit">%</span><span id="heaterFanPwmRaw" class="output-raw">(-- / 255)</span></div></article>
        <article class="output"><div class="output-label">Heater</div><div class="output-value"><span id="heaterPwmPercent" class="output-percent">--</span><span class="output-unit">%</span><span id="heaterPwmRaw" class="output-raw">(-- / 255)</span></div></article>
        <article class="output"><div class="output-label">Exhaust fan</div><div class="output-value"><span id="exhaustFanPwmPercent" class="output-percent">--</span><span class="output-unit">%</span><span id="exhaustFanPwmRaw" class="output-raw">(-- / 255)</span></div></article>
      </div>
    </section>

    <section aria-labelledby="historyHeading">
      <div class="history-head">
        <div class="history-title"><h2 id="historyHeading">History</h2><div id="historyMeta" class="history-meta">Loading RAM buffer</div></div>
        <div class="actions">
          <select id="range" class="range-select" aria-label="History range">
            <option value="120">Last hour</option>
            <option value="720" selected>Last 6 hours</option>
            <option value="1440">Last 12 hours</option>
            <option value="4096">Maximum</option>
          </select>
          <a class="download" href="/api/history.csv" download="drybox-history.csv">CSV</a>
        </div>
      </div>

      <div class="chart-grid">
        <article class="chart-panel combined-history">
          <div class="chart-header">
            <h3>Air temperature, humidity &amp; outputs</h3>
            <div class="legend">
              <span class="legend-item"><span class="legend-line" style="--series:var(--temperature)"></span>Air temp avg (°C)</span>
              <span class="legend-item"><span class="legend-line dotted" style="--series:#925247"></span>Set temp (°C)</span>
              <span class="legend-item"><span class="legend-line" style="--series:var(--humidity)"></span>RH (%)</span>
              <span class="legend-item"><span class="legend-line" style="--series:#b85d42"></span>Heater fan (%)</span>
              <span class="legend-item"><span class="legend-line" style="--series:#c78b2c"></span>Heater (%)</span>
              <span class="legend-item"><span class="legend-line" style="--series:#477b70"></span>Exhaust fan (%)</span>
            </div>
          </div>
          <div class="chart-wrap">
            <canvas id="historyChart" aria-label="Combined air temperature, humidity, and output history chart"></canvas>
            <div id="chartTooltip" class="chart-tooltip" role="status" hidden></div>
          </div>
        </article>
      </div>
    </section>

    <section class="storage-band" aria-label="Controller status">
      <div><div class="storage-label">Storage</div><div class="storage-value">RAM only</div></div>
      <div><div class="storage-label">Buffer</div><div id="bufferValue" class="storage-value">--</div></div>
      <div><div class="storage-label">Free heap</div><div id="heapValue" class="storage-value">--</div></div>
      <div><div class="storage-label">Controller</div><div id="networkValue" class="storage-value">--</div></div>
    </section>
  </main>

  <script>
    const state = {
      samples: [],
      status: null,
      historyLimit: 720,
      historyIntervalSeconds: 30,
      historyRequestId: 0,
      hoverIndex: null,
      setTemperatureC: 45,
      setTemperatureDirty: false
    };
    const combinedSeries = [
      { key: 'average_air_temp', label: 'Air temp avg', color: '#d9563f', axis: 'temperature' },
      { key: 'aht20_rh', label: 'RH', color: '#128579', axis: 'percent' },
      { key: 'heater_fan_pwm', label: 'Heater fan', color: '#b85d42', axis: 'percent', pwm: true },
      { key: 'heater_pwm', label: 'Heater', color: '#c78b2c', axis: 'percent', pwm: true },
      { key: 'exhaust_fan_pwm', label: 'Exhaust fan', color: '#477b70', axis: 'percent', pwm: true }
    ];

    const valueText = (value, digits) => Number.isFinite(value) ? value.toFixed(digits) : '--';
    const pwmPercentText = value => Number.isInteger(value) ? (value / 255 * 100).toFixed(1) : '--';
    const pwmRawText = value => Number.isInteger(value) ? `(${value} / 255)` : '(-- / 255)';
    const durationText = seconds => {
      if (!seconds) return '0 min';
      if (seconds < 3600) return `${Math.floor(seconds / 60)} min`;
      return `${(seconds / 3600).toFixed(seconds < 36000 ? 1 : 0)} h`;
    };
    const bytesText = bytes => `${(bytes / 1024).toFixed(bytes < 102400 ? 1 : 0)} KiB`;

    function selectedRangeLabel() {
      const range = document.getElementById('range');
      return range.options[range.selectedIndex]?.textContent || 'Selected range';
    }

    function renderHistoryMeta() {
      const meta = document.getElementById('historyMeta');
      if (!state.status) {
        meta.textContent = 'Loading RAM buffer';
        return;
      }

      const requestedSpan = state.historyLimit * state.historyIntervalSeconds;
      const availableSpan = state.samples.length > 1 ? state.samples[0].age_s : requestedSpan;
      const displayedSpan = Math.min(requestedSpan, availableSpan);
      meta.textContent = `${state.samples.length.toLocaleString()} samples · ${durationText(displayedSpan)} shown · ${selectedRangeLabel()}`;
    }

    function rawSeriesValue(sample, definition) {
      if (definition.key === 'average_air_temp') {
        const temperatures = [sample.aht20_temp, sample.bmp280_temp].filter(Number.isFinite);
        return temperatures.length ? temperatures.reduce((sum, value) => sum + value, 0) / temperatures.length : null;
      }
      return Number.isFinite(sample[definition.key]) ? sample[definition.key] : null;
    }

    function plottedSeriesValue(sample, definition) {
      const rawValue = rawSeriesValue(sample, definition);
      return rawValue === null ? null : definition.pwm ? rawValue / 255 * 100 : rawValue;
    }

    function combinedValueText(sample, definition) {
      const value = rawSeriesValue(sample, definition);
      if (value === null) return '--';
      if (definition.key === 'average_air_temp') return `${value.toFixed(1)} °C`;
      if (definition.pwm) return `${(value / 255 * 100).toFixed(1)}% (${value} / 255)`;
      return `${value.toFixed(1)}%`;
    }

    function setConnection(online, label) {
      const element = document.getElementById('connection');
      element.className = `connection ${online ? 'online' : 'offline'}`;
      element.querySelector('.connection-label').textContent = label;
    }

    function setError(message = '') {
      const element = document.getElementById('error');
      element.textContent = message;
      element.classList.toggle('visible', Boolean(message));
    }

    function formatTimer(seconds) {
      if (!seconds) return '0 min';
      const hours = Math.floor(seconds / 3600);
      const minutes = Math.ceil((seconds % 3600) / 60);
      return hours ? `${hours} h ${minutes} min` : `${minutes} min`;
    }

    function renderDryingState(status) {
      const drying = status.drying || {};
      const topbar = document.querySelector('.topbar');
      const stateLabel = document.getElementById('dryingState');
      const headerState = document.getElementById('headerDryingStatus');
      const safety = document.getElementById('dryingSafety');
      const toggle = document.getElementById('dryingToggle');
      const setTemperature = document.getElementById('setTemperature');
      const duration = document.getElementById('dryingDuration');
      const isDrying = drying.state === 'drying';
      const isSafe = drying.temperatures_safe === true;
      const isLockedOut = drying.state === 'safety' || drying.state === 'timer_expired';
      const normalizedState = drying.state || 'idle';
      const backendSetTemperature = Number(drying.set_temperature_c);
      const displayState = normalizedState === 'drying'
        ? 'DRYING'
        : normalizedState === 'safety'
          ? 'SAFETY'
          : normalizedState === 'timer_expired'
            ? 'TIMER EXPIRED'
            : 'IDLE';

          topbar.classList.toggle('drying', isDrying);
      stateLabel.textContent = isDrying ? `DRYING · ${formatTimer(drying.remaining_s)}` : displayState;
      headerState.textContent = displayState;
      headerState.className = `header-drying-status ${normalizedState.replace('_', '-')}`;
      toggle.textContent = isDrying ? 'Stop drying' : 'Start drying';
      toggle.classList.toggle('stop', isDrying);
      toggle.disabled = !isDrying && (!isSafe || isLockedOut);
      if (Number.isFinite(backendSetTemperature) && (!state.setTemperatureDirty || isDrying || isLockedOut)) {
        state.setTemperatureC = backendSetTemperature;
        state.setTemperatureDirty = false;
        setTemperature.value = backendSetTemperature;
      }
      duration.value = drying.duration_s ? Math.round(drying.duration_s / 60) : 30;
      safety.className = `drying-safety ${isSafe ? 'safe' : 'unsafe'}`;
      safety.textContent = isSafe
        ? `Temperature safety: armed · 0-65 °C · ${formatTimer(drying.remaining_s || drying.duration_s)}`
        : `Safety shutdown: ${drying.safety_reason || 'both temperatures must be within 0-65 °C'}`;
    }

    async function fetchJson(path) {
      const response = await fetch(path, { cache: 'no-store' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
    }

    async function refreshStatus() {
      try {
        const status = await fetchJson('/api/status');
        state.status = status;
        const latest = status.latest || {};
        document.getElementById('ahtTemp').textContent = valueText(latest.aht20_temp, 1);
        document.getElementById('bmpTemp').textContent = valueText(latest.bmp280_temp, 1);
        document.getElementById('humidity').textContent = valueText(latest.aht20_rh, 1);
        document.getElementById('pressure').textContent = valueText(latest.bmp280_pressure, 1);
        document.getElementById('heaterFanPwmPercent').textContent = pwmPercentText(latest.heater_fan_pwm);
        document.getElementById('heaterFanPwmRaw').textContent = pwmRawText(latest.heater_fan_pwm);
        document.getElementById('heaterPwmPercent').textContent = pwmPercentText(latest.heater_pwm);
        document.getElementById('heaterPwmRaw').textContent = pwmRawText(latest.heater_pwm);
        document.getElementById('exhaustFanPwmPercent').textContent = pwmPercentText(latest.exhaust_fan_pwm);
        document.getElementById('exhaustFanPwmRaw').textContent = pwmRawText(latest.exhaust_fan_pwm);
        renderHistoryMeta();
        document.getElementById('bufferValue').textContent = `${bytesText(status.history_bytes)} · ${durationText(status.maximum_history_s)} max`;
        document.getElementById('heapValue').textContent = bytesText(status.free_heap_bytes);
        document.getElementById('networkValue').textContent = `${status.ip} · ${status.rssi_dbm} dBm`;
        renderDryingState(status);
        setConnection(true, 'Live');
        setError();
      } catch (error) {
        setConnection(false, 'Offline');
        setError(`Controller unavailable: ${error.message}`);
      }
    }

    async function refreshHistory() {
      const limit = Number(document.getElementById('range').value);
      const requestId = ++state.historyRequestId;
      state.historyLimit = limit;
      state.hoverIndex = null;
      state.samples = [];
      renderHistoryMeta();
      drawCombinedChart();
      try {
        const history = await fetchJson(`/api/history?limit=${limit}`);
        if (requestId !== state.historyRequestId) return;
        state.historyIntervalSeconds = Number(history.sample_interval_s) || 30;
        state.samples = history.samples || [];
        renderHistoryMeta();
        drawCombinedChart();
      } catch (error) {
        setError(`History unavailable: ${error.message}`);
      }
    }

    function hideChartTooltip() {
      const tooltip = document.getElementById('chartTooltip');
      tooltip.hidden = true;
    }

    function updateChartTooltip(index, x) {
      const tooltip = document.getElementById('chartTooltip');
      const wrapper = document.querySelector('.chart-wrap');
      const sample = state.samples[index];
      const rows = combinedSeries.map(definition => `
        <div class="chart-tooltip-row"><span class="chart-tooltip-swatch" style="--series:${definition.color}"></span><span>${definition.label}: ${combinedValueText(sample, definition)}</span></div>
      `).join('');
      tooltip.innerHTML = `<div class="chart-tooltip-title">-${durationText(sample.age_s)} · sample ${index + 1}</div>${rows}`;
      tooltip.hidden = false;
      const left = Math.min(Math.max(8, x + 14), wrapper.clientWidth - tooltip.offsetWidth - 8);
      tooltip.style.left = `${left}px`;
      tooltip.style.top = '12px';
    }

    function drawCombinedChart() {
      const canvas = document.getElementById('historyChart');
      const rect = canvas.getBoundingClientRect();
      const ratio = Math.min(window.devicePixelRatio || 1, 2);
      canvas.width = Math.max(1, Math.round(rect.width * ratio));
      canvas.height = Math.max(1, Math.round(rect.height * ratio));
      const context = canvas.getContext('2d');
      context.scale(ratio, ratio);

      const width = rect.width;
      const height = rect.height;
      const plot = { left: 54, right: width - 54, top: 16, bottom: height - 32 };
      const temperatureValues = [];
      for (const sample of state.samples) {
        const value = plottedSeriesValue(sample, combinedSeries[0]);
        if (value !== null) temperatureValues.push(value);
      }

      context.clearRect(0, 0, width, height);
      context.font = '11px Bahnschrift, sans-serif';
      context.fillStyle = '#71807a';
      context.strokeStyle = '#e3e8e5';
      context.lineWidth = 1;

      if (!state.samples.length) {
        context.textAlign = 'center';
        context.fillText('Awaiting sensor data', width / 2, height / 2);
        hideChartTooltip();
        return;
      }

      let temperatureMinimum = temperatureValues.length ? Math.min(...temperatureValues) : 0;
      let temperatureMaximum = temperatureValues.length ? Math.max(...temperatureValues) : 50;
      const setTemperature = Number(state.setTemperatureC);
      if (Number.isFinite(setTemperature)) {
        temperatureMinimum = Math.min(temperatureMinimum, setTemperature);
        temperatureMaximum = Math.max(temperatureMaximum, setTemperature);
      }
      const temperatureSpread = temperatureMaximum - temperatureMinimum || Math.max(Math.abs(temperatureMaximum) * .05, 1);
      temperatureMinimum -= temperatureSpread * .15;
      temperatureMaximum += temperatureSpread * .15;

      for (let line = 0; line <= 4; line += 1) {
        const y = plot.top + ((plot.bottom - plot.top) * line / 4);
        context.beginPath();
        context.moveTo(plot.left, y);
        context.lineTo(plot.right, y);
        context.stroke();
        const temperatureLabel = temperatureMaximum - ((temperatureMaximum - temperatureMinimum) * line / 4);
        const percentLabel = 100 - (100 * line / 4);
        context.textAlign = 'right';
        context.fillText(`${temperatureLabel.toFixed(1)}°C`, plot.left - 8, y + 4);
        context.textAlign = 'left';
        context.fillText(`${percentLabel}%`, plot.right + 8, y + 4);
      }

      if (Number.isFinite(setTemperature)) {
        const setTemperatureY = plot.bottom - ((setTemperature - temperatureMinimum) / (temperatureMaximum - temperatureMinimum) * (plot.bottom - plot.top));
        context.beginPath();
        context.strokeStyle = '#925247';
        context.lineWidth = 1.5;
        context.setLineDash([5, 4]);
        context.moveTo(plot.left, setTemperatureY);
        context.lineTo(plot.right, setTemperatureY);
        context.stroke();
        context.setLineDash([]);
      }

      const requestedSpan = state.historyLimit * state.historyIntervalSeconds;
      const availableSpan = state.samples.length > 1 ? state.samples[0].age_s : requestedSpan;
      const displayedSpan = Math.min(requestedSpan, availableSpan);
      context.textAlign = 'left';
      context.fillText(`-${durationText(displayedSpan)}`, plot.left, height - 8);
      context.textAlign = 'right';
      context.fillText('now', plot.right, height - 8);

      const count = Math.max(state.samples.length - 1, 1);
      for (const definition of combinedSeries) {
        context.beginPath();
        context.strokeStyle = definition.color;
        context.lineWidth = 2;
        context.lineJoin = 'round';
        let drawing = false;
        state.samples.forEach((sample, index) => {
          const value = plottedSeriesValue(sample, definition);
          if (value === null) {
            drawing = false;
            return;
          }
          const x = plot.left + ((plot.right - plot.left) * index / count);
          const minimum = definition.axis === 'temperature' ? temperatureMinimum : 0;
          const maximum = definition.axis === 'temperature' ? temperatureMaximum : 100;
          const y = plot.bottom - ((value - minimum) / (maximum - minimum) * (plot.bottom - plot.top));
          if (!drawing) context.moveTo(x, y); else context.lineTo(x, y);
          drawing = true;
        });
        context.stroke();
      }

      if (state.hoverIndex !== null && state.hoverIndex < state.samples.length) {
        const hoverX = plot.left + ((plot.right - plot.left) * state.hoverIndex / count);
        context.beginPath();
        context.strokeStyle = '#8c9a94';
        context.lineWidth = 1;
        context.setLineDash([4, 4]);
        context.moveTo(hoverX, plot.top);
        context.lineTo(hoverX, plot.bottom);
        context.stroke();
        context.setLineDash([]);

        combinedSeries.forEach(definition => {
          const value = plottedSeriesValue(state.samples[state.hoverIndex], definition);
          if (value === null) return;
          const minimum = definition.axis === 'temperature' ? temperatureMinimum : 0;
          const maximum = definition.axis === 'temperature' ? temperatureMaximum : 100;
          const y = plot.bottom - ((value - minimum) / (maximum - minimum) * (plot.bottom - plot.top));
          context.beginPath();
          context.fillStyle = definition.color;
          context.arc(hoverX, y, 4, 0, Math.PI * 2);
          context.fill();
          context.strokeStyle = '#ffffff';
          context.lineWidth = 1.5;
          context.stroke();
        });
        updateChartTooltip(state.hoverIndex, hoverX);
      } else {
        hideChartTooltip();
      }
    }

    const historyChart = document.getElementById('historyChart');

    async function toggleDrying() {
      const drying = state.status?.drying || {};
      const isDrying = drying.state === 'drying';
      const action = isDrying ? 'stop' : 'start';
      const setTemperature = document.getElementById('setTemperature').value;
      const durationMinutes = document.getElementById('dryingDuration').value;
      const query = new URLSearchParams({ action });
      if (!isDrying) {
        query.set('set_temperature', setTemperature);
        query.set('duration_minutes', durationMinutes);
      }

      try {
        const response = await fetch(`/api/drying?${query.toString()}`, { cache: 'no-store' });
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || `HTTP ${response.status}`);
        await refreshStatus();
      } catch (error) {
        setError(`Drying control failed: ${error.message}`);
      }
    }

    document.getElementById('dryingToggle').addEventListener('click', toggleDrying);
    document.getElementById('setTemperature').addEventListener('input', event => {
      const value = Number(event.target.value);
      if (!Number.isFinite(value)) return;
      state.setTemperatureC = value;
      state.setTemperatureDirty = true;
      drawCombinedChart();
    });
    function updateChartHover(event) {
      if (!state.samples.length) return;
      const rect = historyChart.getBoundingClientRect();
      const plotLeft = 54;
      const plotRight = rect.width - 54;
      const x = Math.min(plotRight, Math.max(plotLeft, event.clientX - rect.left));
      state.hoverIndex = Math.round((x - plotLeft) / (plotRight - plotLeft) * (state.samples.length - 1));
      drawCombinedChart();
    }

    historyChart.addEventListener('pointermove', updateChartHover);
    historyChart.addEventListener('pointerdown', updateChartHover);
    historyChart.addEventListener('pointerleave', () => {
      state.hoverIndex = null;
      drawCombinedChart();
    });
    historyChart.addEventListener('pointercancel', () => {
      state.hoverIndex = null;
      drawCombinedChart();
    });
    document.getElementById('range').addEventListener('change', refreshHistory);
    new ResizeObserver(drawCombinedChart).observe(document.querySelector('.chart-grid'));
    refreshStatus();
    refreshHistory();
    setInterval(refreshStatus, 5000);
    setInterval(refreshHistory, 30000);
  </script>
</body>
</html>
)DRYBOX";
}  // namespace drybox_ui
