// This file stores the web page content in a format the ESP8266 can serve.
// It's included by the main .ino file.

const char HTML_CONTENT[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dr. Water - Live System Monitor</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap');
        :root {
            --accent-color: #007AFF; --background-start: #f0f2f5; --background-end: #d6e0f0;
            --glass-bg: rgba(255, 255, 255, 0.6); --text-color: #1d1d1f; --text-secondary: #6e6e73;
            --border-color: rgba(0, 0, 0, 0.1); --shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.1);
            --status-ok: #34C759; --status-warning: #FF9500; --status-replace: #FF3B30;
        }
        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background: linear-gradient(135deg, var(--background-start), var(--background-end));
            display: flex; justify-content: center; align-items: flex-start; min-height: 100vh;
            margin: 0; padding: 2rem; color: var(--text-color); box-sizing: border-box; overflow-y: auto;
        }
        .container { width: 100%; max-width: 1000px; display: flex; flex-direction: column; gap: 2rem; }
        .glass-card {
            background: var(--glass-bg); backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px);
            border-radius: 20px; border: 1px solid var(--border-color); box-shadow: var(--shadow);
            padding: 1.5rem 2rem;
        }
        .header { display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem; }
        .header h1 { font-size: 1.75rem; font-weight: 700; margin: 0; display: flex; align-items: center; gap: 0.75rem; }
        .header h1 svg { color: var(--accent-color); }
        .controls { display: flex; align-items: center; gap: 1rem; }
        button {
            font-family: 'Inter', sans-serif; font-size: 0.9rem; font-weight: 500; padding: 0.6rem 1.2rem;
            border-radius: 10px; border: none; cursor: pointer; transition: all 0.2s ease;
        }
        .status-indicator { width: 10px; height: 10px; border-radius: 50%; transition: background-color 0.3s ease; }
        .status-indicator.disconnected { background-color: var(--status-replace); }
        .status-indicator.connected { background-color: var(--status-ok); }
        .admin-btn { background-color: #e5e5e7; color: var(--text-color); }
        .admin-btn:hover { background-color: #dcdce0; }
        .main-metrics { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1.5rem; }
        .metric-item { display: flex; flex-direction: column; gap: 0.5rem; }
        .metric-item .label { font-size: 0.9rem; color: var(--text-secondary); font-weight: 500; }
        .metric-item .value { font-size: 2.25rem; font-weight: 600; line-height: 1.1; }
        .metric-item .unit { font-size: 1rem; color: var(--text-secondary); font-weight: 500; margin-left: 0.25rem; }
        .cartridges-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1.5rem; }
        .cartridge-card {
            background: rgba(255, 255, 255, 0.8); border-radius: 15px; padding: 1.25rem;
            display: flex; flex-direction: column; gap: 1rem; border: 1px solid var(--border-color);
        }
        .cartridge-header { display: flex; justify-content: space-between; align-items: center; }
        .cartridge-title { font-size: 1.1rem; font-weight: 600; }
        .status-badge { font-size: 0.8rem; font-weight: 600; padding: 0.3rem 0.7rem; border-radius: 20px; color: white; }
        .status-ok { background-color: var(--status-ok); }
        .status-warning { background-color: var(--status-warning); }
        .status-replace { background-color: var(--status-replace); }
        .progress-bar-container { width: 100%; height: 8px; background-color: #e9ecef; border-radius: 4px; overflow: hidden; }
        .progress-bar { height: 100%; width: 100%; background-color: var(--status-ok); border-radius: 4px; transition: width 0.5s ease, background-color 0.5s ease; }
        .cartridge-stats { display: flex; justify-content: space-between; font-size: 0.85rem; color: var(--text-secondary); }
        .modal-overlay {
            position: fixed; top: 0; left: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.4);
            display: flex; justify-content: center; align-items: center; opacity: 0; visibility: hidden;
            transition: opacity 0.3s ease, visibility 0.3s ease; z-index: 1000;
        }
        .modal-overlay.visible { opacity: 1; visibility: visible; }
        .modal-content {
            background: var(--glass-bg); backdrop-filter: blur(25px); -webkit-backdrop-filter: blur(25px);
            border-radius: 20px; border: 1px solid var(--border-color); box-shadow: var(--shadow);
            padding: 2rem; width: 90%; max-width: 400px; transform: scale(0.95); transition: transform 0.3s ease;
        }
        .modal-overlay.visible .modal-content { transform: scale(1); }
        .modal-content h2 { margin-top: 0; text-align: center; }
        .form-group { margin-bottom: 1rem; }
        .form-group label { display: block; margin-bottom: 0.5rem; color: var(--text-secondary); font-weight: 500; }
        .form-group input {
            width: 100%; padding: 0.75rem; border: 1px solid var(--border-color); border-radius: 10px;
            font-size: 1rem; box-sizing: border-box; background: rgba(255,255,255,0.5);
        }
        .modal-actions { display: flex; justify-content: flex-end; gap: 1rem; margin-top: 1.5rem; }
        .admin-controls { display: none; flex-direction: column; gap: 1rem; }
        .admin-controls.visible { display: flex; }
        .admin-actions { display: flex; flex-wrap: wrap; gap: 1rem; align-items: center; }
        .admin-actions input { padding: 0.6rem; border: 1px solid var(--border-color); border-radius: 10px; width: 100px; }
        .connect-btn { background: var(--accent-color); color: white; }
    </style>
</head>
<body>
    <div class="container">
        <div class="glass-card header">
            <h1>
                <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"></path></svg>
                Dr. Water Live Monitor
            </h1>
            <div class="controls">
                <span id="connectionStatus" class="status-indicator disconnected"></span>
                <button id="adminBtn" class="admin-btn">Admin Login</button>
            </div>
        </div>
        <div class="glass-card main-metrics">
            <div class="metric-item">
                <span class="label">Total Volume</span>
                <span class="value" id="totalVolume">0.00<span class="unit">L</span></span>
            </div>
            <div class="metric-item">
                <span class="label">Current Flow</span>
                <span class="value" id="currentSpeed">0.00<span class="unit">L/min</span></span>
            </div>
            <div class="metric-item">
                <span class="label">Highest Flow</span>
                <span class="value" id="highestSpeed">0.00<span class="unit">L/min</span></span>
            </div>
        </div>
        <div class="glass-card admin-controls" id="adminControls">
             <h3>Technician Controls</h3>
             <div class="admin-actions">
                 <button id="hardResetBtn" class="admin-btn" style="background-color: var(--status-replace); color: white;">Hard Reset System</button>
                 <div>
                    <input type="number" id="cartridgeResetInput" placeholder="e.g., 3" min="1" max="7">
                    <button id="cartridgeResetBtn" class="admin-btn">Reset Cartridge</button>
                 </div>
             </div>
        </div>
        <div class="cartridges-grid" id="cartridgesGrid"></div>
    </div>
    <div class="modal-overlay" id="loginModal">
        <div class="modal-content">
            <h2>Technician Login</h2>
            <div class="form-group">
                <label for="userId">User ID</label>
                <input type="text" id="userId" value="drwtr01">
            </div>
            <div class="form-group">
                <label for="password">Password</label>
                <input type="password" id="password" value="1234">
            </div>
            <p id="loginError" style="color: var(--status-replace); font-size: 0.9rem; text-align: center; display: none;">Authentication Failed.</p>
            <div class="modal-actions">
                <button id="cancelLoginBtn" class="admin-btn">Cancel</button>
                <button id="submitLoginBtn" class="connect-btn">Login</button>
            </div>
        </div>
    </div>
    <script>
        const adminBtn = document.getElementById('adminBtn');
        const loginModal = document.getElementById('loginModal');
        const cancelLoginBtn = document.getElementById('cancelLoginBtn');
        const submitLoginBtn = document.getElementById('submitLoginBtn');
        const adminControls = document.getElementById('adminControls');
        const hardResetBtn = document.getElementById('hardResetBtn');
        const cartridgeResetBtn = document.getElementById('cartridgeResetBtn');
        const cartridgeResetInput = document.getElementById('cartridgeResetInput');
        const connectionStatus = document.getElementById('connectionStatus');
        const totalVolumeEl = document.getElementById('totalVolume');
        const currentSpeedEl = document.getElementById('currentSpeed');
        const highestSpeedEl = document.getElementById('highestSpeed');
        const cartridgesGrid = document.getElementById('cartridgesGrid');
        let isAdmin = false;
        const NUM_CARTRIDGES = 7;

        function initCartridges() {
            cartridgesGrid.innerHTML = '';
            for (let i = 1; i <= NUM_CARTRIDGES; i++) {
                cartridgesGrid.innerHTML += `
                    <div class="cartridge-card">
                        <div class="cartridge-header">
                            <span class="cartridge-title">Cartridge #${i}</span>
                            <span class="status-badge" id="status-${i}">--</span>
                        </div>
                        <div class="progress-bar-container"><div class="progress-bar" id="progress-${i}" style="width: 0%;"></div></div>
                        <div class="cartridge-stats">
                            <span id="used-${i}">Used: -- L</span>
                            <span id="remaining-${i}">Remaining: -- L</span>
                        </div>
                    </div>`;
            }
        }

        async function fetchData() {
            try {
                const response = await fetch('/data');
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                const data = await response.json();
                updateConnectionState(true);
                parseAndDisplayData(data);
            } catch (error) {
                console.error('Failed to fetch data:', error);
                updateConnectionState(false);
            }
        }

        function updateConnectionState(isConnected) {
            connectionStatus.className = isConnected 
                ? 'status-indicator connected' 
                : 'status-indicator disconnected';
        }

        function parseAndDisplayData(data) {
            totalVolumeEl.innerHTML = `${data.totalVolume.toFixed(2)}<span class="unit">L</span>`;
            currentSpeedEl.innerHTML = `${data.currentSpeed.toFixed(2)}<span class="unit">L/min</span>`;
            highestSpeedEl.innerHTML = `${data.highestSpeed.toFixed(2)}<span class="unit">L/min</span>`;

            data.cartridges.forEach((cartridge, index) => {
                const i = index + 1;
                const { status, used, remaining } = cartridge;
                const total = used + remaining;
                const percentage = total > 0 ? (used / total) * 100 : 0;
                
                const statusEl = document.getElementById(`status-${i}`);
                const progressEl = document.getElementById(`progress-${i}`);
                
                statusEl.textContent = status;
                statusEl.className = 'status-badge';
                progressEl.style.backgroundColor = '';
                
                switch (status.toLowerCase()) {
                    case 'ok': statusEl.classList.add('status-ok'); progressEl.style.backgroundColor = 'var(--status-ok)'; break;
                    case 'warning': statusEl.classList.add('status-warning'); progressEl.style.backgroundColor = 'var(--status-warning)'; break;
                    case 'replace': statusEl.classList.add('status-replace'); progressEl.style.backgroundColor = 'var(--status-replace)'; break;
                }
                progressEl.style.width = `${percentage}%`;
                document.getElementById(`used-${i}`).textContent = `Used: ${used.toFixed(2)} L`;
                document.getElementById(`remaining-${i}`).textContent = `Remaining: ${remaining.toFixed(2)} L`;
            });
        }

        async function sendCommand(command) {
            if (!isAdmin) {
                alert('Admin login required.');
                return;
            }
            try {
                const userId = document.getElementById('userId').value;
                const password = document.getElementById('password').value;
                const response = await fetch('/reset', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: `cmd=${command}&user=${userId}&pass=${password}`
                });
                const responseText = await response.text();
                if (response.ok) {
                    alert('Command successful: ' + responseText);
                    fetchData(); // Refresh data immediately
                } else {
                    alert('Command failed: ' + responseText);
                }
            } catch (error) {
                console.error('Error sending command:', error);
                alert('Failed to send command. Check connection.');
            }
        }

        adminBtn.addEventListener('click', () => loginModal.classList.add('visible'));
        cancelLoginBtn.addEventListener('click', () => loginModal.classList.remove('visible'));
        submitLoginBtn.addEventListener('click', () => {
            const userId = document.getElementById('userId').value;
            const password = document.getElementById('password').value;
            if (userId === 'drwtr01' && password === '1234') {
                isAdmin = true;
                loginModal.classList.remove('visible');
                adminControls.classList.add('visible');
                adminBtn.style.display = 'none';
                document.getElementById('loginError').style.display = 'none';
            } else {
                document.getElementById('loginError').style.display = 'block';
            }
        });
        
        hardResetBtn.addEventListener('click', () => sendCommand('h'));
        cartridgeResetBtn.addEventListener('click', () => {
            const num = cartridgeResetInput.value;
            if (num && num >= 1 && num <= 7) {
                sendCommand(`c=${num}`);
                cartridgeResetInput.value = '';
            } else {
                alert('Please enter a valid cartridge number (1-7).');
            }
        });

        initCartridges();
        setInterval(fetchData, 2000); // Fetch data every 2 seconds
        fetchData(); // Initial fetch
    </script>
</body>
</html>
)rawliteral";
