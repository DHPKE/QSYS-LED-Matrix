"""
web_server.py — Port of the ESP32 AsyncWebServer + HTML UI

Provides:
  GET  /            → full web UI (identical look/feel to the ESP32 version)
  GET  /api/config  → JSON: {udp_port, brightness, matrix_width, matrix_height}
  GET  /api/segments → JSON: {segments: [...]}
  POST /api/test    → accepts raw JSON body, dispatches as a command

Uses the built-in http.server so there are zero extra dependencies beyond
Flask if you prefer that (see commented import at the bottom).

Run with sudo (or set cap_net_bind_service) for port 80, or change
WEB_PORT to 8080 in config.py for unprivileged access.
"""

import json
import logging
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

from config import WEB_PORT, UDP_PORT, MATRIX_WIDTH, MATRIX_HEIGHT
from segment_manager import SegmentManager
from udp_handler import UDPHandler, get_brightness

logger = logging.getLogger(__name__)


# ─── HTML template (same layout / JS as the ESP32 version) ─────────────────
# The {{PLACEHOLDERS}} are replaced at request time.

_HTML_TEMPLATE = r"""<!DOCTYPE html>
<html>
<head>
    <title>LED Matrix Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
               background: linear-gradient(135deg, #0f0f1e 0%, #1a1a2e 100%);
               color: #e0e0e0; min-height: 100vh; padding: 20px; }
        .container { max-width: 1200px; margin: 0 auto; }
        .header { text-align: center; margin-bottom: 30px; padding: 30px 20px;
                  background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
                  border-radius: 15px; box-shadow: 0 8px 32px rgba(0,0,0,.3); }
        .header h1 { font-size: 2.5em; color: #fff; margin-bottom: 10px; }
        .header .subtitle { color: #a0c4ff; font-size: 1.1em; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px; }
        .segments-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
        @media (max-width: 768px) { .grid, .segments-grid { grid-template-columns: 1fr; } }
        .card { background: rgba(30,30,46,.9); border-radius: 12px; padding: 20px;
                box-shadow: 0 4px 16px rgba(0,0,0,.2); border: 1px solid rgba(255,255,255,.1); }
        .card.compact { padding: 15px; transition: all .3s; position: relative; }
        .card.compact.inactive { opacity: .4; pointer-events: none; filter: grayscale(.7); }
        .card.compact.inactive::after { content:'INACTIVE'; position:absolute; top:50%; left:50%;
            transform:translate(-50%,-50%); background:rgba(0,0,0,.85); color:#666;
            padding:8px 20px; border-radius:6px; font-weight:700; font-size:.9em;
            letter-spacing:2px; z-index:10; border:2px solid rgba(255,255,255,.15); }
        .card h2 { font-size: 1.2em; margin-bottom: 15px; color: #4a9eff;
                   border-bottom: 2px solid #4a9eff; padding-bottom: 8px; }
        .card.compact h2 { font-size: 1em; margin-bottom: 12px; padding-bottom: 6px; }
        .info-item { display: flex; flex-direction: column; gap: 6px; }
        .info-label { color: #888; font-weight: 600; font-size: .85em; text-transform: uppercase; }
        .info-value input { background: rgba(0,0,0,.3); border: 1px solid rgba(255,255,255,.1);
            border-radius: 4px; color: #e0e0e0; padding: 6px 10px; font-size: .9em; min-width: 140px; }
        .status { padding: 8px 15px; border-radius: 20px; font-size: .9em; font-weight: 600; display: inline-block; }
        .status.ready   { background:#1a472a; color:#4ade80; }
        .status.sending { background:#1e3a8a; color:#60a5fa; }
        .status.error   { background:#7f1d1d; color:#f87171; }
        #preview { border: 2px solid #333; background: #000; border-radius: 8px;
                   image-rendering: pixelated; box-shadow: 0 0 20px rgba(74,158,255,.3); }
        .form-group { margin-bottom: 12px; }
        label { display: block; margin-bottom: 6px; color: #a0a0a0; font-weight: 600;
                font-size: .85em; text-transform: uppercase; letter-spacing: .5px; }
        input[type=text], input[type=number], select {
            width: 100%; padding: 10px 12px; background: rgba(0,0,0,.4);
            border: 1px solid rgba(255,255,255,.1); border-radius: 8px;
            color: #e0e0e0; font-size: .95em; }
        input[type=range] { width:100%; height:8px; }
        button { padding: 10px 16px; border: none; border-radius: 8px; font-size: .9em;
                 font-weight: 600; cursor: pointer; transition: all .3s;
                 text-transform: uppercase; letter-spacing: .5px; }
        .btn-primary   { background: #2563eb; color: white; }
        .btn-primary:hover { background: #1d4ed8; transform: translateY(-2px); }
        .btn-danger    { background: #ef4444; color: white; }
        .btn-danger:hover  { background: #dc2626; transform: translateY(-2px); }
        .btn-clear-all { grid-column: 1/-1; background: #dc2626; color: white; width: 100%; margin-top: 8px; }
        .button-group  { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 8px; margin-top: 12px; }
        .align-group { display: flex; gap: 8px; margin-top: 5px; }
        .align-btn { flex:1; padding:6px; background:rgba(255,255,255,.1);
                     border:2px solid rgba(255,255,255,.2); border-radius:6px;
                     color:#888; cursor:pointer; transition:all .2s; font-size:.85em; }
        .align-btn.active { background:#2563eb; border-color:#2563eb; color:white; }
        .brightness-display { display:flex; justify-content:space-between; align-items:center; margin-top:5px; }
        .brightness-value   { background:rgba(74,158,255,.2); color:#4a9eff;
                               padding:5px 15px; border-radius:20px; font-weight:700; font-size:1.1em; }
        .layout-btn { height:80px; background:rgba(255,255,255,.05); border:2px solid rgba(255,255,255,.1);
                      border-radius:8px; cursor:pointer; transition:all .3s;
                      display:flex; flex-direction:column; align-items:center; justify-content:center; gap:8px; }
        .layout-btn:hover { background:rgba(37,99,235,.2); border-color:#2563eb; transform:translateY(-2px); }
        .layout-visual { display:grid; width:48px; height:32px; gap:2px;
                          background:#000; border-radius:3px; padding:2px; }
        .layout-visual.split-v { grid-template-columns:1fr 1fr; }
        .layout-visual.split-h { grid-template-rows:1fr 1fr; }
        .layout-visual.quad    { grid-template-columns:1fr 1fr; grid-template-rows:1fr 1fr; }
        .layout-visual.full    { padding:0; }
        .layout-segment { background:#2563eb; border-radius:2px; display:flex;
                           align-items:center; justify-content:center;
                           font-size:9px; font-weight:700; color:white; }
        .layout-label { font-size:.75em; color:#888; }
    </style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>LED Matrix Controller</h1>
    <div class="subtitle">RPi Zero 2 W + PoE HAT — 64×32 RGB Display</div>
  </div>

  <div class="grid">
    <div class="card" style="grid-column:1/-1;">
      <div style="display:flex;justify-content:space-between;align-items:flex-start;flex-wrap:wrap;gap:20px;">
        <div>
          <h2 style="margin:0 0 15px 0;">Network Information</h2>
          <div style="display:flex;gap:20px;flex-wrap:wrap;">
            <div class="info-item"><span class="info-label">IP Address</span>
              <div class="info-value"><input type="text" id="ip-address" value="{{IP_ADDRESS}}"></div></div>
            <div class="info-item"><span class="info-label">UDP Port</span>
              <div class="info-value"><input type="number" id="udp-port" value="{{UDP_PORT}}" style="width:100px;"></div></div>
            <div class="info-item"><span class="info-label">Display Size</span>
              <span class="info-value" style="padding:6px 10px;background:rgba(0,0,0,.3);border-radius:4px;border:1px solid rgba(255,255,255,.1);">{{MATRIX_SIZE}}</span></div>
          </div>
        </div>
        <div style="align-self:center;"><span id="status" class="status ready">Ready</span></div>
      </div>
    </div>

    <div class="card" style="grid-column:1/-1;text-align:center;">
      <h2>Live Preview</h2>
      <canvas id="preview" width="64" height="32" style="width:100%;max-width:640px;height:auto;"></canvas>
    </div>

    <div class="card" style="grid-column:1/-1;">
      <h2>Segment Layouts</h2>
      <div style="display:grid;grid-template-columns:repeat(4,1fr);gap:12px;">
        <button class="layout-btn" onclick="applyLayout('split-vertical')">
          <div class="layout-visual split-v"><div class="layout-segment">1</div><div class="layout-segment">2</div></div>
          <span class="layout-label">1 | 2</span></button>
        <button class="layout-btn" onclick="applyLayout('split-horizontal')">
          <div class="layout-visual split-h"><div class="layout-segment">1</div><div class="layout-segment">2</div></div>
          <span class="layout-label">1 / 2</span></button>
        <button class="layout-btn" onclick="applyLayout('quad')">
          <div class="layout-visual quad"><div class="layout-segment">1</div><div class="layout-segment">2</div>
            <div class="layout-segment">3</div><div class="layout-segment">4</div></div>
          <span class="layout-label">2×2</span></button>
        <button class="layout-btn" onclick="applyLayout('fullscreen')">
          <div class="layout-visual full"><div class="layout-segment" style="width:100%;height:100%;border-radius:3px;">ALL</div></div>
          <span class="layout-label">Full</span></button>
      </div>
    </div>
  </div><!-- /grid -->

  <div class="segments-grid" id="segments-grid">
    <!-- Segment cards inserted by JS -->
  </div>

  <div class="card" style="margin-bottom:20px;">
    <h2>Display Settings</h2>
    <div class="form-group">
      <label>Brightness</label>
      <input type="range" id="brightness" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
      <div class="brightness-display">
        <span style="color:#888;">Dim</span>
        <span class="brightness-value" id="brightness-value">128</span>
        <span style="color:#888;">Bright</span>
      </div>
    </div>
    <button class="btn-clear-all" onclick="clearAll()">Clear All Segments</button>
  </div>
</div><!-- /container -->

<script>
const COLORS = [
  ['#FFFFFF','White'],['#FF0000','Red'],['#00FF00','Lime'],['#0000FF','Blue'],
  ['#FFFF00','Yellow'],['#FF00FF','Magenta'],['#00FFFF','Cyan'],['#FFA500','Orange'],
  ['#800080','Purple'],['#008000','Green'],['#FFC0CB','Pink'],['#FFD700','Gold'],
  ['#C0C0C0','Silver'],['#808080','Gray'],['#000000','Black']
];
const DEFAULT_FG = ['#FFFFFF','#00FF00','#FF0000','#FFFF00'];
const canvas = document.getElementById('preview');
const ctx = canvas.getContext('2d');
const segmentAlign = ['C','C','C','C'];
const segmentBounds = [
  {x:0,y:0,width:32,height:32},{x:32,y:0,width:32,height:32},
  {x:0,y:16,width:32,height:16},{x:32,y:16,width:32,height:16}
];

ctx.fillStyle='#000'; ctx.fillRect(0,0,64,32);

function makeOptions(selectedColor) {
  return COLORS.map(([v,n]) =>
    `<option value="${v}"${v===selectedColor?' selected':''}>${n}</option>`).join('');
}

function buildSegmentCards() {
  const grid = document.getElementById('segments-grid');
  grid.innerHTML = '';
  for (let i=0;i<4;i++) {
    const fg = DEFAULT_FG[i] || '#FFFFFF';
    grid.innerHTML += `
    <div class="card compact${i>0?' inactive':''}" id="segment-card-${i}">
      <h2>Segment ${i+1}</h2>
      <div class="form-group"><label>Text</label>
        <input type="text" id="text${i}" placeholder="Enter message..."></div>
      <div class="form-group"><label>Text Color</label>
        <select id="color${i}">${makeOptions(fg)}</select></div>
      <div class="form-group"><label>Background</label>
        <select id="bgcolor${i}">${makeOptions('#000000')}</select></div>
      <div class="form-group"><label>Intensity</label>
        <input type="range" id="intensity${i}" min="0" max="255" value="255"
          oninput="document.getElementById('iv${i}').textContent=this.value">
        <span id="iv${i}" style="color:#888;font-size:.85em;">255</span></div>
      <div class="form-group"><label>Font</label>
        <select id="font${i}">
          <option value="arial">Arial</option><option value="mono">Mono</option></select></div>
      <div class="form-group"><label>Alignment</label>
        <div class="align-group">
          <button class="align-btn" onclick="setAlign(${i},'L',this)">Left</button>
          <button class="align-btn active" onclick="setAlign(${i},'C',this)">Center</button>
          <button class="align-btn" onclick="setAlign(${i},'R',this)">Right</button>
        </div></div>
      <div class="button-group">
        <button class="btn-primary" onclick="sendText(${i})">Display</button>
        <button class="btn-primary" onclick="previewText(${i})">Preview</button>
        <button class="btn-danger"  onclick="clearSegment(${i})">Clear</button>
      </div>
    </div>`;
  }
}

buildSegmentCards();

window.addEventListener('load', () => { pollSegments(); setInterval(pollSegments,1000); });

function pollSegments() {
  fetch('/api/segments').then(r=>r.json()).then(data => {
    if (!data.segments) return;
    const active = data.segments.filter(s=>s.active&&s.w>0&&s.h>0).map(s=>s.id);
    updateSegmentStates(active);
    data.segments.forEach(s => {
      segmentBounds[s.id] = {x:s.x,y:s.y,width:s.w,height:s.h};
      const el = document.getElementById('text'+s.id);
      if (el && document.activeElement!==el) el.value = s.text||'';
    });
    ctx.fillStyle='#000'; ctx.fillRect(0,0,64,32);
    data.segments.forEach(s => {
      if (s.active&&s.w>0&&s.h>0&&s.text)
        drawSegmentOnCanvas(s.id,s.text,s.color||'#FFF',s.bgcolor||'#000',s.align||'C');
    });
  }).catch(()=>{});
}

function updateStatus(msg,type='ready') {
  const el=document.getElementById('status'); el.textContent=msg; el.className='status '+type;
}
function setAlign(seg,align,btn) {
  segmentAlign[seg]=align;
  btn.parentElement.querySelectorAll('.align-btn').forEach(b=>b.classList.remove('active'));
  btn.classList.add('active');
}
function updateSegmentStates(active) {
  for(let i=0;i<4;i++){
    const c=document.getElementById('segment-card-'+i);
    if(c) active.includes(i)?c.classList.remove('inactive'):c.classList.add('inactive');
  }
}
function sendJSON(obj) {
  updateStatus('Sending…','sending');
  fetch('/api/test',{method:'POST',headers:{'Content-Type':'text/plain'},body:JSON.stringify(obj)})
    .then(r=>r.text()).then(()=>updateStatus('OK','ready'))
    .catch(e=>updateStatus('Error: '+e,'error'));
}
function sendText(seg) {
  const text=document.getElementById('text'+seg).value;
  const color=document.getElementById('color'+seg).value.replace('#','');
  const bgcolor=document.getElementById('bgcolor'+seg).value.replace('#','');
  const intensity=parseInt(document.getElementById('intensity'+seg).value);
  const font=document.getElementById('font'+seg).value;
  const align=segmentAlign[seg];
  sendJSON({cmd:'text',seg,text,color,bgcolor,font,size:'auto',align,effect:'none',intensity});
  drawSegmentOnCanvas(seg,text,'#'+color,'#'+bgcolor,align);
}
function clearSegment(seg) {
  sendJSON({cmd:'clear',seg});
  const b=segmentBounds[seg]; ctx.fillStyle='#000'; ctx.fillRect(b.x,b.y,b.width,b.height);
}
function clearAll() {
  sendJSON({cmd:'clear_all'}); ctx.fillStyle='#000'; ctx.fillRect(0,0,64,32);
}
function updateBrightness(v) {
  document.getElementById('brightness-value').textContent=v;
  sendJSON({cmd:'brightness',value:parseInt(v)});
}
function applyLayout(type) {
  let cmds=[],clr=[];
  if(type==='split-vertical') {
    updateSegmentStates([0,1]);
    segmentBounds[0]={x:0,y:0,width:32,height:32}; segmentBounds[1]={x:32,y:0,width:32,height:32};
    segmentBounds[2]=segmentBounds[3]={x:0,y:0,width:0,height:0};
    cmds=[{cmd:'config',seg:0,x:0,y:0,w:32,h:32},{cmd:'config',seg:1,x:32,y:0,w:32,h:32}]; clr=[2,3];
  } else if(type==='split-horizontal') {
    updateSegmentStates([0,1]);
    segmentBounds[0]={x:0,y:0,width:64,height:16}; segmentBounds[1]={x:0,y:16,width:64,height:16};
    segmentBounds[2]=segmentBounds[3]={x:0,y:0,width:0,height:0};
    cmds=[{cmd:'config',seg:0,x:0,y:0,w:64,h:16},{cmd:'config',seg:1,x:0,y:16,w:64,h:16}]; clr=[2,3];
  } else if(type==='quad') {
    updateSegmentStates([0,1,2,3]);
    segmentBounds[0]={x:0,y:0,width:32,height:16}; segmentBounds[1]={x:32,y:0,width:32,height:16};
    segmentBounds[2]={x:0,y:16,width:32,height:16}; segmentBounds[3]={x:32,y:16,width:32,height:16};
    cmds=[{cmd:'config',seg:0,x:0,y:0,w:32,h:16},{cmd:'config',seg:1,x:32,y:0,w:32,h:16},
          {cmd:'config',seg:2,x:0,y:16,w:32,h:16},{cmd:'config',seg:3,x:32,y:16,w:32,h:16}];
  } else if(type==='fullscreen') {
    updateSegmentStates([0]);
    segmentBounds[0]={x:0,y:0,width:64,height:32};
    segmentBounds[1]=segmentBounds[2]=segmentBounds[3]={x:0,y:0,width:0,height:0};
    cmds=[{cmd:'config',seg:0,x:0,y:0,w:64,h:32}]; clr=[1,2,3];
  }
  cmds.forEach(c=>sendJSON(c)); clr.forEach(s=>sendJSON({cmd:'clear',seg:s}));
  ctx.fillStyle='#000'; ctx.fillRect(0,0,64,32);
}
function drawSegmentOnCanvas(seg,text,color,bgcolor,align) {
  const b=segmentBounds[seg]; if(!b||b.width===0||b.height===0) return;
  ctx.fillStyle=bgcolor; ctx.fillRect(b.x,b.y,b.width,b.height);
  if(!text) return;
  const sizes=[24,20,18,16,14,12,10,9,8,6]; let sz=6;
  for(const s of sizes) {
    ctx.font=s+'px Arial';
    if(ctx.measureText(text).width<=b.width-4 && s*1.2<=b.height-2){sz=s;break;}
  }
  ctx.font=sz+'px Arial'; ctx.fillStyle=color; ctx.textBaseline='middle';
  let x;
  if(align==='L'){ctx.textAlign='left';x=b.x+2;}
  else if(align==='R'){ctx.textAlign='right';x=b.x+b.width-2;}
  else{ctx.textAlign='center';x=b.x+b.width/2;}
  ctx.fillText(text,x,b.y+b.height/2);
}
function previewText(seg) {
  drawSegmentOnCanvas(seg,
    document.getElementById('text'+seg).value,
    document.getElementById('color'+seg).value,
    document.getElementById('bgcolor'+seg).value,
    segmentAlign[seg]);
}
</script>
</body>
</html>
"""


class _Handler(BaseHTTPRequestHandler):
    """HTTP request handler. _sm and _udp are injected by WebServer."""

    # These are set by WebServer before the server starts
    _sm:  SegmentManager  = None
    _udp: UDPHandler      = None

    def log_message(self, fmt, *args):
        logger.debug(f"[HTTP] {fmt % args}")

    def _send(self, code: int, content_type: str, body: bytes | str):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlparse(self.path).path

        if path in ("/", "/index.html"):
            import socket as _socket
            try:
                ip = _socket.gethostbyname(_socket.gethostname())
            except Exception:
                ip = "unknown"
            html = _HTML_TEMPLATE \
                .replace("{{IP_ADDRESS}}", ip) \
                .replace("{{UDP_PORT}}",   str(UDP_PORT)) \
                .replace("{{MATRIX_SIZE}}", f"{MATRIX_WIDTH}x{MATRIX_HEIGHT}")
            self._send(200, "text/html", html)

        elif path == "/api/config":
            doc = {
                "udp_port":      UDP_PORT,
                "brightness":    get_brightness(),
                "matrix_width":  MATRIX_WIDTH,
                "matrix_height": MATRIX_HEIGHT,
            }
            self._send(200, "application/json", json.dumps(doc))

        elif path == "/api/segments":
            doc = {"segments": self._sm.snapshot()}
            self._send(200, "application/json", json.dumps(doc))

        else:
            self._send(404, "text/plain", "Not found")

    def do_POST(self):
        path = urlparse(self.path).path

        if path == "/api/test":
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length).decode("utf-8", errors="replace")
            logger.info(f"[HTTP] /api/test  body={body[:120]}")
            self._udp.dispatch(body)
            self._send(200, "text/plain", f"Command dispatched: {body}")
        else:
            self._send(404, "text/plain", "Not found")

    def do_OPTIONS(self):
        # CORS pre-flight
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()


class WebServer:
    """Thin wrapper: creates the HTTPServer in a daemon thread."""

    def __init__(self, segment_manager: SegmentManager,
                 udp_handler: UDPHandler):
        _Handler._sm  = segment_manager
        _Handler._udp = udp_handler
        self._httpd = HTTPServer(("", WEB_PORT), _Handler)
        self._thread = threading.Thread(target=self._httpd.serve_forever,
                                        name="web-server", daemon=True)

    def start(self):
        self._thread.start()
        logger.info(f"[HTTP] Web server listening on port {WEB_PORT}")

    def stop(self):
        self._httpd.shutdown()
