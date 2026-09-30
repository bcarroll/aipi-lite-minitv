#ifndef AIPI_LITE_WEB_UI_H
#define AIPI_LITE_WEB_UI_H

#include <Arduino.h>

static const char kMiniTvDashboardHtml[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="color-scheme" content="dark light">
  <title>AI-PI Lite MiniTV</title>
  <style>
    :root { color-scheme: dark; font: 16px/1.5 system-ui, sans-serif; background: #111827; color: #f9fafb; }
    * { box-sizing: border-box; }
    body { margin: 0; padding: 1rem; }
    main { max-width: 42rem; margin: 0 auto; }
    h1, h2 { line-height: 1.2; }
    section { margin-block: 1rem; padding: 1rem; border: 1px solid #9ca3af; border-radius: .5rem; }
    .controls { display: flex; flex-wrap: wrap; gap: .75rem; }
    button, input { font: inherit; }
    button { min-height: 2.75rem; padding: .5rem .9rem; color: #fff; background: #1f2937; border: 2px solid #d1d5db; border-radius: .35rem; }
    button:hover { background: #374151; }
    :focus-visible { outline: 3px solid #fbbf24; outline-offset: 3px; }
    label { display: block; margin-block: .75rem .25rem; }
    input[type="range"] { width: min(100%, 24rem); min-height: 2rem; accent-color: #fbbf24; }
    dl { display: grid; grid-template-columns: minmax(8rem, auto) 1fr; gap: .35rem 1rem; }
    dt { font-weight: 700; }
    dd { margin: 0; overflow-wrap: anywhere; }
    #status { min-height: 1.5em; padding: .5rem; border-left: .25rem solid #fbbf24; }
    @media (max-width: 30rem) { dl { grid-template-columns: 1fr; gap: 0; } dd { margin-bottom: .5rem; } }
  </style>
</head>
<body>
  <main>
    <h1>AI-PI Lite MiniTV</h1>
    <p id="status" role="status" aria-live="polite">Connecting to the player…</p>
    <section aria-labelledby="playback-heading">
      <h2 id="playback-heading">Playback</h2>
      <dl>
        <dt>Channel</dt><dd id="channel">Unknown</dd>
        <dt>State</dt><dd id="playback-state">Unknown</dd>
        <dt>Wi-Fi</dt><dd id="wifi-state">Unknown</dd>
      </dl>
      <div class="controls">
        <button type="button" data-action="previous">Previous channel</button>
        <button type="button" data-action="next">Next channel</button>
        <button type="button" data-action="mute" aria-pressed="false">Mute</button>
      </div>
      <label for="volume">Volume</label>
      <input id="volume" type="range" min="0" max="100" value="50" aria-valuetext="50 percent">
      <label for="brightness">Brightness</label>
      <input id="brightness" type="range" min="0" max="100" value="80" aria-valuetext="80 percent">
    </section>
    <section aria-labelledby="library-heading">
      <h2 id="library-heading">Library</h2>
      <p>Local videos are stored on the MicroPython data drive. Network playlist entries must point to direct MJPEG and MP3 resources.</p>
      <p id="library-state">Library management is not available in this firmware build yet.</p>
    </section>
  </main>
  <script>
    const statusText = document.getElementById('status');
    const text = (id, value) => { document.getElementById(id).textContent = value || 'Unknown'; };
    async function refreshStatus() {
      try {
        const response = await fetch('/api/status', { cache: 'no-store' });
        if (!response.ok) throw new Error('Status request failed');
        const data = await response.json();
        text('channel', data.channelTitle);
        text('playback-state', data.playbackState);
        text('wifi-state', data.wifiState);
        if (typeof data.muted === 'boolean') {
          const muteButton = document.querySelector('[data-action="mute"]');
          muteButton.setAttribute('aria-pressed', String(data.muted));
          muteButton.textContent = data.muted ? 'Unmute' : 'Mute';
        }
        statusText.textContent = 'Player status updated.';
      } catch (_) {
        statusText.textContent = 'Player status is unavailable.';
      }
    }
    async function sendAction(action, value) {
      try {
        const response = await fetch('/api/playback', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action, value })
        });
        if (!response.ok) throw new Error('Playback request failed');
        statusText.textContent = 'Playback command sent.';
        await refreshStatus();
      } catch (_) {
        statusText.textContent = 'Playback command could not be sent.';
      }
    }
    document.querySelectorAll('button[data-action]').forEach((button) => {
      button.addEventListener('click', () => sendAction(button.dataset.action));
    });
    document.getElementById('volume').addEventListener('change', (event) => {
      const value = Number(event.target.value);
      event.target.setAttribute('aria-valuetext', `${value} percent`);
      sendAction('volume', value);
    });
    document.getElementById('brightness').addEventListener('change', (event) => {
      const value = Number(event.target.value);
      event.target.setAttribute('aria-valuetext', `${value} percent`);
      sendAction('brightness', value);
    });
    refreshStatus();
  </script>
</body>
</html>)HTML";

#endif  // AIPI_LITE_WEB_UI_H
