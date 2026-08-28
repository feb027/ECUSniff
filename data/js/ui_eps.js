// EPS & VSS Bench Tester UI Controller

let isEpsUserDragging = false;

function updateEpsUi(eps) {
    if (!eps) return;

    // 1. Status Badge & Power Button
    const badge = document.getElementById('epsStatusBadge');
    const btnPwr = document.getElementById('btnEpsPower');
    if (badge) {
        badge.textContent = eps.running ? 'RUNNING' : 'STOPPED';
        badge.className = 'mode-badge ' + (eps.running ? 'state-running' : 'state-standby');
    }
    if (btnPwr) {
        btnPwr.textContent = eps.running ? 'HENTIKAN EPS' : 'JALANKAN EPS';
        btnPwr.className = 'btn-power-full ' + (eps.running ? 'btn-stop' : 'btn-start');
    }

    // 2. Speed Display & Slider
    const spdVal = document.getElementById('epsSpeedDisplay');
    const spdFreq = document.getElementById('epsVssFreq');
    const spdSlider = document.getElementById('epsSpeedSlider');
    if (spdVal) spdVal.textContent = Math.round(eps.speed);
    if (spdFreq) spdFreq.textContent = eps.vssFreq.toFixed(1) + ' Hz';
    if (spdSlider && !isEpsUserDragging) spdSlider.value = Math.round(eps.targetSpeed || eps.speed);

    // 3. RPM Display & Slider
    const rpmVal = document.getElementById('epsRpmDisplay');
    const rpmFreq = document.getElementById('epsRpmFreq');
    const rpmSlider = document.getElementById('epsRpmSlider');
    if (rpmVal) rpmVal.textContent = eps.rpm;
    if (rpmFreq) rpmFreq.textContent = eps.rpmFreq.toFixed(1) + ' Hz';
    if (rpmSlider && !isEpsUserDragging) rpmSlider.value = eps.targetRpm || eps.rpm;

    // 4. Steer Torque & Voltages
    const steerSlider = document.getElementById('epsSteerSlider');
    const steerText = document.getElementById('epsSteerText');
    const trq1 = document.getElementById('epsTrq1Volt');
    const trq2 = document.getElementById('epsTrq2Volt');
    if (steerSlider && !isEpsUserDragging) steerSlider.value = Math.round((eps.steer || 0) * 100);
    if (steerText) {
        const val = Math.round((eps.steer || 0) * 100);
        if (val < -5) steerText.textContent = `Kiri ${-val}%`;
        else if (val > 5) steerText.textContent = `Kanan ${val}%`;
        else steerText.textContent = 'Lurus (Center)';
    }
    if (trq1) trq1.textContent = eps.trq1.toFixed(2) + ' V';
    if (trq2) trq2.textContent = eps.trq2.toFixed(2) + ' V';

    // 5. Preset & Sweep
    const presetSel = document.getElementById('epsPresetSelect');
    if (presetSel && !isEpsUserDragging) presetSel.value = eps.preset;
    const sweepBadge = document.getElementById('epsSweepBadge');
    if (sweepBadge) {
        sweepBadge.textContent = eps.sweep ? 'SWEEP: ON' : 'SWEEP: OFF';
        sweepBadge.className = 'mode-badge ' + (eps.sweep ? 'state-running' : 'state-standby');
    }
}

function toggleEpsPower() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'eps_toggle' }));
    }
}

function setEpsPreset(presetIdx) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'eps_preset', val: parseInt(presetIdx) }));
    }
}

function onEpsSpeedChange(val) {
    const spd = parseFloat(val);
    document.getElementById('epsSpeedDisplay').textContent = Math.round(spd);
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'eps_set', speed: spd }));
    }
}

function onEpsRpmChange(val) {
    const rpm = parseInt(val);
    document.getElementById('epsRpmDisplay').textContent = rpm;
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'eps_set', rpm: rpm }));
    }
}

function onEpsSteerChange(val) {
    const steer = parseFloat(val) / 100.0;
    const steerText = document.getElementById('epsSteerText');
    if (steerText) {
        const pct = parseInt(val);
        if (pct < -5) steerText.textContent = `Kiri ${-pct}%`;
        else if (pct > 5) steerText.textContent = `Kanan ${pct}%`;
        else steerText.textContent = 'Lurus (Center)';
    }
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'eps_set', steer: steer }));
    }
}

function centerSteer() {
    const slider = document.getElementById('epsSteerSlider');
    if (slider) slider.value = 0;
    onEpsSteerChange(0);
}

function toggleEpsSweep() {
    const current = document.getElementById('epsSweepBadge').textContent.includes('ON');
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'eps_sweep', val: current ? 0 : 1 }));
    }
}

function setQuickSpeed(spd) {
    const slider = document.getElementById('epsSpeedSlider');
    if (slider) slider.value = spd;
    onEpsSpeedChange(spd);
}
