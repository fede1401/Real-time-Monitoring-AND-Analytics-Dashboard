#include "listener.h"

Listener::Listener(const std ::string &address, const int &port, Protocol prot)
    : address_(address), port_(port)
{
    // Creating socket

    // TCP
    if (prot == Protocol::TCP)
    {
        // creation socket
        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket_ < 0)
        {
            perror("Socket creation failed");
            exit(1);
        }

        // Opzione per riutilizzare l'indirizzo
        int opt = 1;
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        setParameterConnession();
        bindSocket();
        listenSocket();
        acceptSocket();
    }

    // UDP
    else
    {
        serverSocket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (serverSocket_ < 0)
        {
            perror("Socket creation failed");
            exit(1);
        }

        // Opzione per riutilizzare l'indirizzo
        int opt = 1;
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        setParameterConnession();
        bindSocket();
    }

    marshal_ = new Marshal();
    agent_ = new Agent();
}

Listener::~Listener()
{
    close(serverSocket_);

    if (marshal_)
    {
        delete marshal_;
    }

    if (agent_)
    {
        delete agent_;
    }
}

void Listener::setParameterConnession()
{
    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_port = htons(port_);
    // serverAddress_.sin_addr.s_addr = inet_addr(address_.c_str());
    serverAddress_.sin_addr.s_addr = INADDR_ANY; // Ascolta su tutte le interfacce
}

void Listener::bindSocket()
{
    std::cout << "Binding to port " << port_ << std::endl;

    if (bind(serverSocket_, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    std::cout << "Bind successful!" << std::endl;
}

void Listener::listenSocket()
{
    if (listen(serverSocket_, 10) < 0)
    {
        perror("Listen failed");
        exit(1);
    }
    std::cout << "Listening on port " << port_ << std::endl;
}

void Listener::acceptSocket()
{
    clientSocket_ = accept(serverSocket_, nullptr, nullptr);
}

/* ─────────────────────────────────────────────────────────────────
   Main receive loop
───────────────────────────────────────────────────────────────── */
void Listener::receiveData()
{
    while (true)
    {
        memset(buffer_, 0, sizeof(buffer_));
        int bytesRead = recv(clientSocket_, buffer_, sizeof(buffer_) - 1, 0);

        if (bytesRead > 0)
        {
            // set the end of string with '\0'
            buffer_[bytesRead] = '\0';
            std::cout << "Received" << buffer_ << std::endl;

            for (int i = 0; i< bytesRead; i++)
            {
                printf("%02X ", buffer_[i]);
                std::cout << "Received " << buffer_[i] << std::endl;
            }
            std::cout << buffer_ << std::endl;

            std::string bufferString(buffer_);
            Request req = marshal_->unmarshal(bufferString);

            if (req.getMethod() == Methods::GET && req.getPath() == "/")
            {
                std::cout << "[Listener] GET / — collecting system metrics..." << std::endl;

                CPU    infoCPU        = agent_->takeInfoCPU();
                RAM    infoRAM        = agent_->takeInfoRAM();
                std::vector<ActiveConnection> infoConnections = agent_->takeInfoConnection();

                std::string html     = buildDashboardHTML(infoCPU, infoRAM, infoConnections);
                std::string response = buildHTTPResponse(html);

                send(clientSocket_, response.c_str(), response.size(), 0);
                std::cout << "[Listener] Response sent (" << response.size() << " bytes)" << std::endl;
            }
        }
        else if (bytesRead == 0)
        {
            std::cout << "[Listener] Client disconnected, waiting for new connection..." << std::endl;
            acceptSocket();
        }
    }
}

/* ─────────────────────────────────────────────────────────────────
   Build HTTP/1.1 200 OK response
───────────────────────────────────────────────────────────────── */
std::string Listener::buildHTTPResponse(const std::string& html)
{
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\n"
         << "Content-Type: text/html; charset=utf-8\r\n"
         << "Content-Length: " << html.size() << "\r\n"
         << "Connection: keep-alive\r\n"
         << "\r\n"
         << html;
    return resp.str();
}

/* ─────────────────────────────────────────────────────────────────
   Build dashboard HTML with real system data injected
───────────────────────────────────────────────────────────────── */
std::string Listener::buildDashboardHTML(const CPU& cpu,
                                          const RAM& ram,
                                          const std::vector<ActiveConnection>& connections)
{
    // ── connection rows ──────────────────────────────────────────
    std::ostringstream rows;
    for (const auto& c : connections)
    {
        std::string proto      = (c.protocol == Protocol::TCP) ? "TCP" : "UDP";
        std::string protoClass = (c.protocol == Protocol::TCP) ? "proto-tcp" : "proto-udp";

        std::string stateClass = "state-other";
        if      (c.state == "LISTEN")                          stateClass = "state-listen";
        else if (c.state == "ESTABLISHED")                     stateClass = "state-estab";
        else if (c.state.find("WAIT") != std::string::npos)   stateClass = "state-wait";

        rows << "<tr>"
             << "<td><span class='proto-badge " << protoClass << "'>" << proto << "</span></td>"
             << "<td>" << c.serverAddress << "</td>"
             << "<td>" << c.serverPort    << "</td>"
             << "<td>" << c.clientAddress << "</td>"
             << "<td>" << c.clientPort    << "</td>"
             << "<td><span class='state-badge " << stateClass << "'>" << c.state << "</span></td>"
             << "<td class='muted'>" << c.pidProgram << "</td>"
             << "</tr>\n";
    }

    std::string connRows  = rows.str();
    std::string connCount = std::to_string(connections.size());

    // ── full HTML page ───────────────────────────────────────────
    std::ostringstream h;

    // NOTE: delimiter HTML avoids conflicts with CSS var(--x)" sequences
    h << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<meta http-equiv="refresh" content="5"/>
<title>System Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#0f1117;--surface:#161b22;--surface-2:#1e2433;--border:#2a3348;
  --blue:#5794f2;--green:#73bf69;--orange:#f47a42;--red:#f2495c;--purple:#b877d9;
  --text:#d0d3dc;--muted:#6e7891;--bright:#ffffff;--r:8px;--rl:12px;
}
html,body{height:100%}
body{font-family:'Inter','Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--text);font-size:14px;line-height:1.5;overflow-x:hidden}
::-webkit-scrollbar{width:6px}::-webkit-scrollbar-track{background:var(--bg)}::-webkit-scrollbar-thumb{background:var(--border);border-radius:3px}
.layout{display:flex;height:100vh}
.sidebar{width:56px;background:var(--surface);border-right:1px solid var(--border);display:flex;flex-direction:column;align-items:center;padding:16px 0;gap:8px;flex-shrink:0}
.logo{width:36px;height:36px;background:linear-gradient(135deg,var(--blue),var(--purple));border-radius:var(--r);display:flex;align-items:center;justify-content:center;font-size:18px;margin-bottom:12px}
.sdiv{width:32px;height:1px;background:var(--border);margin:4px 0}
.sbtn{width:36px;height:36px;border-radius:var(--r);display:flex;align-items:center;justify-content:center;cursor:pointer;color:var(--muted);font-size:16px;border:none;background:transparent;transition:.2s}
.sbtn:hover{background:var(--surface-2);color:var(--text)}
.sbtn.active{background:rgba(87,148,242,.15);color:var(--blue)}
.main{flex:1;display:flex;flex-direction:column;overflow:hidden}
.topbar{height:52px;background:var(--surface);border-bottom:1px solid var(--border);display:flex;align-items:center;padding:0 20px;gap:12px;flex-shrink:0}
.topbar-title{font-size:16px;font-weight:600;color:var(--bright);flex:1}
.topbar-title span{color:var(--muted);font-weight:400;font-size:13px;margin-left:8px}
.badge{display:inline-flex;align-items:center;gap:5px;padding:3px 10px;border-radius:20px;font-size:12px;font-weight:500}
.badge-green{background:rgba(115,191,105,.15);color:var(--green)}
.badge-blue{background:rgba(87,148,242,.15);color:var(--blue)}
.dot{width:6px;height:6px;border-radius:50%;background:currentColor}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
.pulse{animation:pulse 2s infinite}
.refresh-info{font-size:11px;color:var(--muted)}
.content{flex:1;overflow-y:auto;padding:20px;display:flex;flex-direction:column;gap:20px}
.sec{font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);display:flex;align-items:center;gap:8px;margin-bottom:2px}
.sec::after{content:'';flex:1;height:1px;background:var(--border)}
.panel{background:var(--surface);border:1px solid var(--border);border-radius:var(--rl);overflow:hidden;transition:border-color .2s}
.panel:hover{border-color:#3a4a6a}
.ph{padding:12px 16px 10px;border-bottom:1px solid var(--border);display:flex;align-items:center;gap:8px}
.pt{font-size:13px;font-weight:600;color:var(--bright);flex:1}
.pb{padding:16px}
.g4{display:grid;grid-template-columns:repeat(4,1fr);gap:16px}
.g2{display:grid;grid-template-columns:repeat(2,1fr);gap:16px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--rl);padding:16px;position:relative;overflow:hidden;transition:transform .2s,border-color .2s}
.card:hover{transform:translateY(-2px);border-color:#3a4a6a}
.card::before{content:'';position:absolute;top:0;left:0;right:0;height:2px}
.card.blue::before{background:linear-gradient(90deg,var(--blue),transparent)}
.card.green::before{background:linear-gradient(90deg,var(--green),transparent)}
.card.orange::before{background:linear-gradient(90deg,var(--orange),transparent)}
.card.purple::before{background:linear-gradient(90deg,var(--purple),transparent)}
.slabel{font-size:11px;font-weight:500;text-transform:uppercase;letter-spacing:.06em;color:var(--muted);margin-bottom:6px}
.sval{font-size:26px;font-weight:700;color:var(--bright);line-height:1.1}
.sval.md{font-size:18px}
.ssub{font-size:12px;color:var(--muted);margin-top:4px}
.sicon{position:absolute;right:14px;top:14px;font-size:22px;opacity:.15}
.cpugrid{display:grid;grid-template-columns:1fr 1fr;gap:12px 20px;margin-top:4px}
.il{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:.05em;margin-bottom:2px}
.iv{font-size:14px;font-weight:600;color:var(--bright)}
.chart-wrap{position:relative;height:180px}
.ram-side{display:flex;align-items:center;gap:24px}
.donut-wrap{width:140px;height:140px;flex-shrink:0;position:relative}
.donut-label{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;pointer-events:none}
.dl{font-size:11px;color:var(--muted)}
.dv{font-size:16px;font-weight:700;color:var(--bright)}
.ramrows{display:flex;flex-direction:column;gap:10px;flex:1}
.rr{display:flex;align-items:center;gap:10px}
.rdot{width:8px;height:8px;border-radius:50%;flex-shrink:0}
.rl{flex:1;font-size:12px;color:var(--muted)}
.rv{font-size:13px;font-weight:600;color:var(--bright);min-width:56px;text-align:right}
.prg{margin-top:20px;display:flex;flex-direction:column;gap:12px}
.plabel{display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-bottom:5px}
.ptrack{height:6px;background:var(--surface-2);border-radius:3px;overflow:hidden}
.pfill{height:100%;border-radius:3px;transition:width 1s ease}
.pfill.orange{background:linear-gradient(90deg,var(--orange),#d4633a)}
.pfill.blue{background:linear-gradient(90deg,var(--blue),#4070cc)}
.pfill.green{background:linear-gradient(90deg,var(--green),#5fa85a)}
.tw{overflow-x:auto}
table{width:100%;border-collapse:collapse;font-size:13px}
thead th{text-align:left;padding:8px 12px;font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:.06em;color:var(--muted);border-bottom:1px solid var(--border);white-space:nowrap}
tbody tr{border-bottom:1px solid rgba(42,51,72,.5);transition:background .15s}
tbody tr:last-child{border-bottom:none}
tbody tr:hover{background:rgba(255,255,255,.03)}
tbody td{padding:9px 12px;white-space:nowrap}
td.muted{color:var(--muted)}
.proto-badge{display:inline-flex;padding:2px 8px;border-radius:4px;font-size:11px;font-weight:700}
.proto-tcp{background:rgba(87,148,242,.15);color:var(--blue)}
.proto-udp{background:rgba(244,122,66,.15);color:var(--orange)}
.state-badge{display:inline-flex;padding:2px 8px;border-radius:4px;font-size:11px;font-weight:600}
.state-listen{background:rgba(115,191,105,.12);color:var(--green)}
.state-estab{background:rgba(87,148,242,.12);color:var(--blue)}
.state-wait{background:rgba(244,122,66,.12);color:var(--orange)}
.state-other{background:rgba(110,120,145,.12);color:var(--muted)}
.footer{height:28px;background:var(--surface);border-top:1px solid var(--border);display:flex;align-items:center;padding:0 20px;gap:16px;flex-shrink:0}
.fi{display:flex;align-items:center;gap:5px;font-size:11px;color:var(--muted)}
@keyframes fadeInUp{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}
.card,.panel{animation:fadeInUp .4s ease both}
.card:nth-child(1){animation-delay:.05s}.card:nth-child(2){animation-delay:.1s}
.card:nth-child(3){animation-delay:.15s}.card:nth-child(4){animation-delay:.2s}
@media(max-width:1100px){.g4{grid-template-columns:repeat(2,1fr)}}
@media(max-width:800px){.g4,.g2{grid-template-columns:1fr}.sidebar{display:none}}
</style>
</head>
<body>
<div class="layout">
<nav class="sidebar">
  <div class="logo">&#x26A1;</div>
  <div class="sdiv"></div>
  <button class="sbtn active">&#x25A6;</button>
  <button class="sbtn">&#x25C8;</button>
  <button class="sbtn">&#x25A1;</button>
  <button class="sbtn">&#x27C1;</button>
  <button class="sbtn">&#x2261;</button>
  <div class="sdiv" style="margin-top:auto"></div>
  <button class="sbtn">&#x2699;</button>
</nav>
<div class="main">
  <header class="topbar">
    <span class="topbar-title">System Monitor <span>/ Overview</span></span>
    <span class="badge badge-green"><span class="dot pulse"></span>&nbsp;Live</span>
    <span class="refresh-info" id="clock">--:--:--</span>
    <span class="refresh-info">&#x21BB; Auto-refresh: 5s</span>
  </header>
  <main class="content">
    <div class="sec">Quick Stats</div>
    <div class="g4">
      <div class="card blue">
        <div class="slabel">CPU Cores</div>
        <div class="sval">)HTML";
    h << cpu.cores;
    h << R"HTML(</div>
        <div class="ssub">)HTML";
    h << cpu.architecture;
    h << R"HTML(</div>
        <div class="sicon">&#x25C8;</div>
      </div>
      <div class="card purple">
        <div class="slabel">CPU Vendor</div>
        <div class="sval md">)HTML";
    h << cpu.vendor;
    h << R"HTML(</div>
        <div class="ssub">Processor ID</div>
        <div class="sicon">&#x1F3ED;</div>
      </div>
      <div class="card orange">
        <div class="slabel">RAM Used</div>
        <div class="sval">)HTML";
    h << ram.used;
    h << R"HTML(</div>
        <div class="ssub">of )HTML";
    h << ram.total;
    h << R"HTML( total</div>
        <div class="sicon">&#x25A6;</div>
      </div>
      <div class="card green">
        <div class="slabel">Active Connections</div>
        <div class="sval">)HTML";
    h << connCount;
    h << R"HTML(</div>
        <div class="ssub">TCP + UDP listeners</div>
        <div class="sicon">&#x27C1;</div>
      </div>
    </div>
    <div class="sec">CPU &amp; Memory</div>
    <div class="g2">
      <div class="panel">
        <div class="ph">
          <span>&#x25C8;</span>
          <span class="pt">CPU Information</span>
          <span class="badge badge-blue">)HTML";
    h << cpu.architecture;
    h << R"HTML(</span>
        </div>
        <div class="pb">
          <div class="cpugrid">
            <div><div class="il">Architecture</div><div class="iv">)HTML";
    h << cpu.architecture;
    h << R"HTML(</div></div>
            <div><div class="il">Logical Cores</div><div class="iv">)HTML";
    h << cpu.cores;
    h << R"HTML(</div></div>
            <div><div class="il">Vendor ID</div><div class="iv">)HTML";
    h << cpu.vendor;
    h << R"HTML(</div></div>
            <div><div class="il">Status</div><div class="iv">
              <span class="badge badge-green"><span class="dot pulse"></span>&nbsp;Online</span>
            </div></div>
          </div>
          <div style="margin-top:20px">
            <div class="sec" style="font-size:10px;margin-bottom:10px">CPU Usage &mdash; live simulation</div>
            <div class="chart-wrap"><canvas id="cpuChart"></canvas></div>
          </div>
        </div>
      </div>
      <div class="panel">
        <div class="ph">
          <span>&#x25A6;</span>
          <span class="pt">Memory Usage</span>
        </div>
        <div class="pb">
          <div class="ram-side">
            <div class="donut-wrap">
              <canvas id="ramChart"></canvas>
              <div class="donut-label">
                <div class="dl">Used</div>
                <div class="dv" id="ramPct">-</div>
              </div>
            </div>
            <div class="ramrows">
              <div class="rr">
                <div class="rdot" style="background:#f47a42"></div>
                <div class="rl">Used</div>
                <div class="rv">)HTML";
    h << ram.used;
    h << R"HTML(</div>
              </div>
              <div class="rr">
                <div class="rdot" style="background:#5794f2"></div>
                <div class="rl">Buff / Cache</div>
                <div class="rv">)HTML";
    h << ram.buffCache;
    h << R"HTML(</div>
              </div>
              <div class="rr">
                <div class="rdot" style="background:#73bf69"></div>
                <div class="rl">Available</div>
                <div class="rv">)HTML";
    h << ram.available;
    h << R"HTML(</div>
              </div>
              <div class="rr">
                <div class="rdot" style="background:#2a3348"></div>
                <div class="rl">Total</div>
                <div class="rv">)HTML";
    h << ram.total;
    h << R"HTML(</div>
              </div>
            </div>
          </div>
          <div class="prg">
            <div>
              <div class="plabel"><span>Used</span><span id="usedPct"></span></div>
              <div class="ptrack"><div class="pfill orange" id="barUsed" style="width:0%"></div></div>
            </div>
            <div>
              <div class="plabel"><span>Buff / Cache</span><span id="buffPct"></span></div>
              <div class="ptrack"><div class="pfill blue" id="barBuff" style="width:0%"></div></div>
            </div>
            <div>
              <div class="plabel"><span>Available</span><span id="availPct"></span></div>
              <div class="ptrack"><div class="pfill green" id="barAvail" style="width:0%"></div></div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="sec">Network Connections</div>
    <div class="panel">
      <div class="ph">
        <span>&#x27C1;</span>
        <span class="pt">Active Listeners</span>
        <span class="badge badge-blue">)HTML";
    h << connCount;
    h << R"HTML( entries</span>
      </div>
      <div class="pb" style="padding:0">
        <div class="tw">
          <table>
            <thead>
              <tr>
                <th>Protocol</th><th>Server Address</th><th>Server Port</th>
                <th>Client Address</th><th>Client Port</th><th>State</th><th>PID / Program</th>
              </tr>
            </thead>
            <tbody>)HTML";
    h << connRows;
    h << R"HTML(</tbody>
          </table>
        </div>
      </div>
    </div>
  </main>
  <footer class="footer">
    <div class="fi">
      <span class="dot" style="background:#73bf69"></span>
      127.0.0.1:8080
    </div>
    <div class="fi" style="margin-left:auto">
      Collected at:&nbsp;<span id="ts">-</span>
    </div>
  </footer>
</div>
</div>
<script>
function tick(){document.getElementById('clock').textContent=new Date().toLocaleTimeString('en-GB');}
setInterval(tick,1000); tick();
document.getElementById('ts').textContent=new Date().toLocaleTimeString('en-GB');
)HTML";

    // JS RAM variables — injected as regular C++ strings to avoid raw string delimiter conflicts
    h << "const RAM_USED=\""   << ram.used      << "\";\n"
      << "const RAM_BUFF=\""   << ram.buffCache  << "\";\n"
      << "const RAM_AVAIL=\""  << ram.available  << "\";\n"
      << "const RAM_TOTAL=\""  << ram.total      << "\";\n";

    h << R"HTML(
function parseGiB(s){
  const n=parseFloat(s); if(isNaN(n))return 0;
  const u=s.replace(/[0-9.]/g,'').toLowerCase();
  if(u.includes('gi')||u.includes('g'))return n;
  if(u.includes('mi')||u.includes('m'))return n/1024;
  return n;
}
const total=parseGiB(RAM_TOTAL)||1;
const used=parseGiB(RAM_USED);
const buff=parseGiB(RAM_BUFF);
const avail=parseGiB(RAM_AVAIL);
const pct=Math.round(used/total*100);
document.getElementById('ramPct').textContent=pct+'%';
function setBar(barId,pctId,val){
  const p=(val/total*100).toFixed(1);
  document.getElementById(barId).style.width=p+'%';
  document.getElementById(pctId).textContent=p+'%';
}
setBar('barUsed','usedPct',used);
setBar('barBuff','buffPct',buff);
setBar('barAvail','availPct',avail);
Chart.defaults.color='#6e7891';
Chart.defaults.borderColor='#2a3348';
Chart.defaults.font.family="'Inter','Segoe UI',system-ui,sans-serif";
Chart.defaults.font.size=11;
new Chart(document.getElementById('ramChart'),{
  type:'doughnut',
  data:{
    labels:['Used','Buff/Cache','Available'],
    datasets:[{
      data:[used.toFixed(2),buff.toFixed(2),avail.toFixed(2)],
      backgroundColor:['#f47a42','#5794f2','#73bf69'],
      borderColor:'#161b22',borderWidth:3,hoverOffset:4
    }]
  },
  options:{responsive:true,maintainAspectRatio:false,cutout:'72%',plugins:{legend:{display:false}}}
});
const cpuData=Array.from({length:30},()=>0);
const cpuChart=new Chart(document.getElementById('cpuChart'),{
  type:'line',
  data:{
    labels:Array(30).fill(''),
    datasets:[{
      data:cpuData,
      borderColor:'#5794f2',backgroundColor:'rgba(87,148,242,0.08)',
      borderWidth:2,pointRadius:0,tension:0.4,fill:true
    }]
  },
  options:{
    responsive:true,maintainAspectRatio:false,animation:{duration:300},
    scales:{
      x:{display:false},
      y:{min:0,max:100,grid:{color:'rgba(42,51,72,0.6)'},ticks:{callback:v=>v+'%',maxTicksLimit:5}}
    },
    plugins:{legend:{display:false}}
  }
});
setInterval(()=>{
  cpuChart.data.datasets[0].data.shift();
  cpuChart.data.datasets[0].data.push(Math.random()*40+8);
  cpuChart.update('none');
},1000);
</script>
</body>
</html>)HTML";

    return h.str();
}