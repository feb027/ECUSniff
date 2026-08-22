// Mobile-First Precision Cockpit Controller & WebSocket Bridge

let ws = null;
let scope = null;
let lastSyncedLevel = -1;
let lastSyncedTab = -1;

let engineState = {
    isRunning: false,
    currentRpm: 850,
    targetRpm: 850,
    runMode: 0
};

let currentPattern = {
    ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
    cmp: { events: [{ angle: 120, high: true }, { angle: 180, high: false }, { angle: 420, high: true }, { angle: 470, high: false }] }
};

function initApp() {
    scope = new ScopeVisualizer('scopeMain', 'scopeMinimap', 'probeReadout');
    scope.render(currentPattern);
    initWheelDatabase();
    renderCamEvents();
    initCaptureModule();
    connectWebSocket();

    window.addEventListener('resize', () => {
        if (scope && currentPattern) scope.render(currentPattern);
        if (typeof renderCapturePreview === 'function') renderCapturePreview();
    });
}

// 1. Slide-Over Drawer Navigation
function toggleDrawer(open) {
    const drawer = document.getElementById('navDrawer');
    const backdrop = document.getElementById('drawerBackdrop');
    if (drawer) drawer.classList.toggle('open', open);
    if (backdrop) backdrop.classList.toggle('open', open);
}

function selectDrawerModule(mod) {
    toggleDrawer(false);
    switchMainModule(mod, true);
}

// 2. Module & Sub-Tab Switcher
function switchMainModule(mod, userTriggered = true) {
    const isGen = (mod === 'generator');
    document.getElementById('moduleGenerator').style.display = isGen ? 'block' : 'none';
    document.getElementById('moduleCapture').style.display = isGen ? 'none' : 'block';
    document.getElementById('generatorBottomNav').style.display = isGen ? 'flex' : 'none';

    const itemGen = document.getElementById('drawerItemGen');
    const itemCap = document.getElementById('drawerItemCap');
    if (itemGen) itemGen.classList.toggle('active', isGen);
    if (itemCap) itemCap.classList.toggle('active', !isGen);

    const titleEl = document.getElementById('headerActiveTitle');
    if (titleEl) {
        titleEl.innerText = isGen ? 'GENERATOR SINYAL' : 'SIGNAL CAPTURE / SNIFFER';
    }

    if (userTriggered) {
        sendCommand('set_ui_level', isGen ? 1 : 2);
    }

    if (isGen && scope) {
        setTimeout(() => scope.render(currentPattern), 50);
    } else if (!isGen && typeof renderCapturePreview === 'function') {
        setTimeout(() => renderCapturePreview(), 50);
    }
}

function switchTab(viewId, btn, userTriggered = true) {
    document.querySelectorAll('.tab-view').forEach(v => v.classList.remove('active'));
    document.querySelectorAll('.nav-item').forEach(b => b.classList.remove('active'));

    const targetView = document.getElementById(viewId);
    if (targetView) targetView.classList.add('active');
    if (btn) btn.classList.add('active');

    if (userTriggered) {
        if (viewId === 'viewMonitor') sendCommand('set_tab', 1);
        else if (viewId === 'viewTuner') sendCommand('set_tab', 2);
    }

    if (viewId === 'viewMonitor' && scope) {
        setTimeout(() => scope.render(currentPattern), 50);
    }
}

function setZoom(spanDeg, btn) {
    document.querySelectorAll('.btn-tool').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');
    scope.setZoomSpan(spanDeg);
}

function jumpToGap() {
    scope.jumpToGap();
}

// 3. WebSocket & Master-Master Real-Time Sync
function connectWebSocket() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(`${proto}//${location.host}/ws`);

    ws.onopen = () => {
        const status = document.getElementById('connStatus');
        status.className = 'status-pill status-online';
        document.getElementById('connText').innerText = 'Online';
    };

    ws.onclose = () => {
        const status = document.getElementById('connStatus');
        status.className = 'status-pill status-offline';
        document.getElementById('connText').innerText = 'Terputus';
        setTimeout(connectWebSocket, 2000);
    };

    ws.onmessage = (e) => {
        try {
            const msg = JSON.parse(e.data);
            if (msg.type === 'telemetry') {
                engineState.currentRpm = msg.rpm || 0;
                engineState.targetRpm = msg.targetRpm || 850;
                engineState.isRunning = !!msg.running;
                engineState.runMode = msg.mode || 0;
                updateEngineUi();

                // Live Sync Pattern from Physical Wheel & Cam
                if (msg.ckp) {
                    const c = msg.ckp;
                    let changed = (currentPattern.ckp.totalTeeth !== c.totalTeeth ||
                                   currentPattern.ckp.missingTeeth !== c.missingTeeth ||
                                   currentPattern.ckp.missingPosition !== c.missingPosition ||
                                   Math.abs(currentPattern.ckp.dutyCycle - c.dutyCycle) > 0.01 ||
                                   currentPattern.ckp.inverted !== c.inverted);
                    if (changed) {
                        currentPattern.ckp = { ...c };
                        updateTunerInputsFromState();
                        if (scope) scope.render(currentPattern);
                    }
                }

                // Update Sniffer Cockpit UI
                if (typeof updateCaptureTelemetry === 'function') {
                    updateCaptureTelemetry(msg);
                }

                // Live Physical Screen & Tab Sync
                if (typeof msg.uiLevel !== 'undefined') {
                    handlePhysicalSync(msg);
                }
            }
        } catch (err) {}
    };
}

function handlePhysicalSync(msg) {
    // 1. Module Level Sync
    if (msg.uiLevel === 1 && lastSyncedLevel !== 1) {
        lastSyncedLevel = 1;
        switchMainModule('generator', false);
    } else if (msg.uiLevel === 2 && lastSyncedLevel !== 2) {
        lastSyncedLevel = 2;
        switchMainModule('capture', false);
    } else if (msg.uiLevel === 0) {
        lastSyncedLevel = 0;
    }

    // 2. Sub-Tab Sync for Generator
    if (msg.uiLevel === 1 && msg.tab && msg.tab !== lastSyncedTab) {
        lastSyncedTab = msg.tab;
        const activeView = document.querySelector('.tab-view.active');
        const isBrowsingDb = activeView && activeView.id === 'viewDatabase';

        const navBtns = document.querySelectorAll('.nav-item');
        if (msg.tab === 1 && navBtns[0]) {
            switchTab('viewMonitor', navBtns[0], false);
        } else if ((msg.tab === 2 || msg.tab === 3) && !isBrowsingDb && navBtns[2]) {
            switchTab('viewTuner', navBtns[2], false);
        }
    }
}

function sendCommand(cmd, val = 0) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd, val, ckp: currentPattern.ckp, cmp: currentPattern.cmp.events }));
    }
}

function toggleEngine() {
    engineState.isRunning = !engineState.isRunning;
    sendCommand(engineState.isRunning ? 'start' : 'stop');
    updateEngineUi();
}

function onRpmSlider(val) {
    val = parseInt(val);
    engineState.targetRpm = val;
    document.getElementById('rpmNum').innerText = String(val).padStart(4, '0');
    sendCommand('set_rpm', val);
}

function setRpm(val) {
    document.getElementById('rpmSlider').value = val;
    onRpmSlider(val);
    document.querySelectorAll('.btn-step').forEach(btn => {
        btn.classList.toggle('active', btn.innerText.includes(String(val)));
    });
}

function onModeSelect(m) {
    engineState.runMode = parseInt(m);
    const badges = ["MODE: FIX", "MODE: CRANK", "MODE: SWEEP"];
    document.getElementById('activeModeBadge').innerText = badges[engineState.runMode] || "MODE: FIX";
    sendCommand('set_mode', engineState.runMode);
}

function updateEngineUi() {
    const btn = document.getElementById('btnPower');
    if (engineState.isRunning) {
        btn.className = 'btn-power-full btn-stop';
        btn.innerText = 'STOP GENERATOR';
        document.getElementById('rpmNum').innerText = String(engineState.currentRpm).padStart(4, '0');
    } else {
        btn.className = 'btn-power-full btn-start';
        btn.innerText = 'START GENERATOR';
        document.getElementById('rpmNum').innerText = String(engineState.targetRpm).padStart(4, '0');
    }
    const badges = ["MODE: FIX", "MODE: CRANK", "MODE: SWEEP"];
    document.getElementById('activeModeBadge').innerText = badges[engineState.runMode] || "MODE: FIX";
}

// 4. Signal Capture Trigger & Actions
function triggerCaptureArm() {
    sendCommand('arm_capture');
    const badge = document.getElementById('capStateBadge');
    if (badge) {
        badge.innerText = 'SIAP MEREKAM';
        badge.className = 'mode-badge state-armed';
    }
}

function exportCaptureCsv() {
    window.location.href = '/api/export_csv';
}

function runLoopbackTest() {
    alert("Pastikan jumper terpasang: GPIO 25 (CKP) -> GPIO 34 dan GPIO 26 (CMP) -> GPIO 35.");
    triggerCaptureArm();
}

window.onload = initApp;
