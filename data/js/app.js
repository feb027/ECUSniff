// Mobile-First Precision Cockpit Controller & WebSocket Bridge

let ws = null, scope = null, lastSyncedLevel = -1, lastSyncedTab = -1, userActionLockUntil = 0;

let engineState = { isRunning: false, currentRpm: 850, targetRpm: 850, runMode: 0 };
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

function toggleDrawer(open) {
    const d = document.getElementById('navDrawer'), b = document.getElementById('drawerBackdrop');
    if (d) d.classList.toggle('open', open);
    if (b) b.classList.toggle('open', open);
}

function selectDrawerModule(mod) {
    toggleDrawer(false);
    switchMainModule(mod, true);
}

function switchMainModule(mod, userTriggered = true) {
    if (userTriggered) userActionLockUntil = Date.now() + 2000;
    const isGen = (mod === 'generator'), isCap = (mod === 'capture'), isEps = (mod === 'eps'), isSpeedo = (mod === 'speedo');
    const genEl = document.getElementById('moduleGenerator');
    const capEl = document.getElementById('moduleCapture');
    const epsEl = document.getElementById('moduleEps');
    const speedoEl = document.getElementById('moduleSpeedo');
    const navEl = document.getElementById('generatorBottomNav');
    if (genEl) genEl.style.display = isGen ? 'block' : 'none';
    if (capEl) capEl.style.display = isCap ? 'block' : 'none';
    if (epsEl) epsEl.style.display = isEps ? 'block' : 'none';
    if (speedoEl) speedoEl.style.display = isSpeedo ? 'block' : 'none';
    if (navEl) navEl.style.display = isGen ? 'flex' : 'none';

    const itemGen = document.getElementById('drawerItemGen');
    const itemCap = document.getElementById('drawerItemCap');
    const itemEps = document.getElementById('drawerItemEps');
    const itemSpeedo = document.getElementById('drawerItemSpeedo');
    if (itemGen) itemGen.classList.toggle('active', isGen);
    if (itemCap) itemCap.classList.toggle('active', isCap);
    if (itemEps) itemEps.classList.toggle('active', isEps);
    if (itemSpeedo) itemSpeedo.classList.toggle('active', isSpeedo);

    const titleEl = document.getElementById('headerActiveTitle');
    if (titleEl) {
        if (isGen) titleEl.innerText = 'GENERATOR SINYAL';
        else if (isCap) titleEl.innerText = 'SIGNAL CAPTURE / SNIFFER';
        else if (isEps) titleEl.innerText = 'EPS & VSS BENCH TESTER';
        else if (isSpeedo) titleEl.innerText = 'SPEEDOMETER & CLUSTER TESTER';
    }

    if (userTriggered) {
        const lvl = isGen ? 1 : (isCap ? 2 : (isEps ? 3 : 4));
        lastSyncedLevel = lvl;
        sendCommand('set_ui_level', lvl);
    }

    setTimeout(() => {
        if (isGen && scope) scope.render(currentPattern);
        else if (isCap && typeof renderCapturePreview === 'function') renderCapturePreview();
    }, 60);
}

function switchTab(viewId, btn, userTriggered = true) {
    if (userTriggered) userActionLockUntil = Date.now() + 2000;
    document.querySelectorAll('.tab-view').forEach(v => v.classList.remove('active'));
    document.querySelectorAll('.nav-item').forEach(b => b.classList.remove('active'));

    const targetView = document.getElementById(viewId);
    if (targetView) targetView.classList.add('active');
    if (btn) btn.classList.add('active');

    if (userTriggered) {
        if (viewId === 'viewMonitor') sendCommand('set_tab', 1);
        else if (viewId === 'viewTuner') sendCommand('set_tab', 2);
    }
    if (viewId === 'viewMonitor' && scope) setTimeout(() => scope.render(currentPattern), 50);
}

function setZoom(spanDeg, btn) {
    document.querySelectorAll('.btn-tool').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');
    scope.setZoomSpan(spanDeg);
}

function jumpToGap() { scope.jumpToGap(); }

function connectWebSocket() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(`${proto}//${location.host}/ws`);

    ws.onopen = () => {
        const s = document.getElementById('connStatus');
        if (s) { s.className = 'status-pill status-online'; document.getElementById('connText').innerText = 'Online'; }
    };
    ws.onclose = () => {
        const s = document.getElementById('connStatus');
        if (s) { s.className = 'status-pill status-offline'; document.getElementById('connText').innerText = 'Terputus'; }
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

                let patternChanged = false;
                if (msg.ckp) {
                    const c = msg.ckp;
                    if (currentPattern.ckp.totalTeeth !== c.totalTeeth || currentPattern.ckp.missingTeeth !== c.missingTeeth ||
                        Math.abs(currentPattern.ckp.dutyCycle - c.dutyCycle) > 0.01) {
                        currentPattern.ckp = { ...c }; patternChanged = true;
                    }
                }
                if (msg.cmp && Array.isArray(msg.cmp)) {
                    if (msg.cmp.length !== currentPattern.cmp.events.length) {
                        currentPattern.cmp.events = JSON.parse(JSON.stringify(msg.cmp)); patternChanged = true;
                    }
                }
                if (patternChanged) {
                    updateTunerInputsFromState(); renderCamEvents();
                    if (scope) scope.render(currentPattern);
                }
                if (msg.customSlots && typeof setServerCustomSlots === 'function') {
                    setServerCustomSlots(msg.customSlots);
                }
                if (typeof updateCaptureTelemetry === 'function') updateCaptureTelemetry(msg);
                if (msg.eps && typeof updateEpsUi === 'function') updateEpsUi(msg.eps);
                if (typeof msg.uiLevel !== 'undefined') handlePhysicalSync(msg);
            }
        } catch (err) {}
    };
}

function handlePhysicalSync(msg) {
    if (Date.now() < userActionLockUntil) return;
    if (msg.uiLevel === 1 && lastSyncedLevel !== 1) { lastSyncedLevel = 1; switchMainModule('generator', false); }
    else if (msg.uiLevel === 2 && lastSyncedLevel !== 2) { lastSyncedLevel = 2; switchMainModule('capture', false); }
    else if (msg.uiLevel === 3 && lastSyncedLevel !== 3) { lastSyncedLevel = 3; switchMainModule('eps', false); }
    else if (msg.uiLevel === 4 && lastSyncedLevel !== 4) { lastSyncedLevel = 4; switchMainModule('speedo', false); }
    else if (msg.uiLevel === 0) { lastSyncedLevel = 0; }

    if (msg.uiLevel === 1 && msg.tab && msg.tab !== lastSyncedTab) {
        lastSyncedTab = msg.tab;
        const activeView = document.querySelector('.tab-view.active'), isDb = activeView && activeView.id === 'viewDatabase';
        const navBtns = document.querySelectorAll('.nav-item');
        if (msg.tab === 1 && navBtns[0]) switchTab('viewMonitor', navBtns[0], false);
        else if ((msg.tab === 2 || msg.tab === 3) && !isDb && navBtns[2]) switchTab('viewTuner', navBtns[2], false);
    }
}

function sendCommand(cmd, val = 0) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
            cmd, val, name: currentPattern.name || "Pola Kustom",
            ckp: currentPattern.ckp, cmp: currentPattern.cmp.events
        }));
    }
}

function toggleEngine() {
    engineState.isRunning = !engineState.isRunning;
    sendCommand(engineState.isRunning ? 'start' : 'stop');
    updateEngineUi();
}

function onRpmSlider(val) {
    val = parseInt(val); engineState.targetRpm = val;
    document.getElementById('rpmNum').innerText = String(val).padStart(4, '0');
    sendCommand('set_rpm', val);
}

function setRpm(val) {
    document.getElementById('rpmSlider').value = val; onRpmSlider(val);
    document.querySelectorAll('.btn-step').forEach(btn => btn.classList.toggle('active', btn.innerText.includes(String(val))));
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
        btn.className = 'btn-power-full btn-stop'; btn.innerText = 'STOP GENERATOR';
        document.getElementById('rpmNum').innerText = String(engineState.currentRpm).padStart(4, '0');
    } else {
        btn.className = 'btn-power-full btn-start'; btn.innerText = 'START GENERATOR';
        document.getElementById('rpmNum').innerText = String(engineState.targetRpm).padStart(4, '0');
    }
    const badges = ["MODE: FIX", "MODE: CRANK", "MODE: SWEEP"];
    document.getElementById('activeModeBadge').innerText = badges[engineState.runMode] || "MODE: FIX";
    const statusBadge = document.getElementById('genStatusBadge');
    if (statusBadge) {
        statusBadge.innerText = engineState.isRunning ? 'RUNNING' : 'STOPPED';
        statusBadge.className = `mode-badge ${engineState.isRunning ? 'state-complete' : 'state-standby'}`;
    }
}

function triggerCaptureArm() {
    sendCommand('arm_capture');
    const badge = document.getElementById('capStateBadge');
    if (badge) { badge.innerText = 'SIAP MEREKAM'; badge.className = 'mode-badge state-armed'; }
}

function exportCaptureCsv() { window.location.href = '/api/export_csv'; }

function runLoopbackTest() {
    alert("Pastikan jumper terpasang: GPIO 25 (CKP) -> GPIO 34 dan GPIO 26 (CMP) -> GPIO 35.");
    triggerCaptureArm();
}

window.onload = initApp;
