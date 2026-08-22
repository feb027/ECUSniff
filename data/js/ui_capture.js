// Capture & Sniffer Cockpit Controller, Waveform Visualizer, & Auto-Match Inspector

let capScope = null;
let lastCapturedData = {
    rpm: 0,
    vehicle: "Belum Terdeteksi",
    confidence: 0,
    totalTeeth: 36,
    missingTeeth: 1,
    gapRatio: "2.0x",
    duty: 50,
    camEvents: []
};

function initCaptureModule() {
    capScope = new ScopeVisualizer('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
    renderCapturePreview();
}

function renderCapturePreview() {
    if (!capScope) return;
    const pat = {
        ckp: {
            totalTeeth: lastCapturedData.totalTeeth || 36,
            missingTeeth: lastCapturedData.missingTeeth || 1,
            missingPosition: 0,
            dutyCycle: (lastCapturedData.duty || 50) / 100.0,
            inverted: false
        },
        cmp: {
            events: lastCapturedData.camEvents && lastCapturedData.camEvents.length > 0
                ? lastCapturedData.camEvents
                : [{ angle: 120, high: true }, { angle: 180, high: false }, { angle: 420, high: true }, { angle: 470, high: false }]
        }
    };
    capScope.render(pat);
}

function updateCaptureTelemetry(msg) {
    if (!msg) return;

    // 1. Status Badge
    const badge = document.getElementById('capStateBadge');
    if (badge) {
        const states = ["STANDBY", "ARMED", "RECORDING", "COMPLETE"];
        const cur = states[msg.capState] || "STANDBY";
        badge.innerText = cur;
        badge.className = `mode-badge state-${cur.toLowerCase()}`;
    }

    // 2. RPM & Vehicle Info
    if (msg.capRpm > 0) {
        lastCapturedData.rpm = msg.capRpm;
        const rpmEl = document.getElementById('capRpmDisplay');
        if (rpmEl) rpmEl.innerText = String(msg.capRpm).padStart(4, '0');
    }

    if (msg.capVehicle && msg.capVehicle.length > 0) {
        lastCapturedData.vehicle = msg.capVehicle;
        const vEl = document.getElementById('capVehicleName');
        if (vEl) vEl.innerText = msg.capVehicle;
    }

    // 3. Technical Geometry Metrics
    if (msg.ckp) {
        lastCapturedData.totalTeeth = msg.ckp.totalTeeth || 36;
        lastCapturedData.missingTeeth = msg.ckp.missingTeeth || 1;
        lastCapturedData.duty = Math.round((msg.ckp.dutyCycle || 0.5) * 100);

        const tEl = document.getElementById('metricTeeth');
        const mEl = document.getElementById('metricMissing');
        const dEl = document.getElementById('metricDuty');
        const rEl = document.getElementById('metricGapRatio');

        if (tEl) tEl.innerText = `${lastCapturedData.totalTeeth}T`;
        if (mEl) mEl.innerText = `${lastCapturedData.missingTeeth} Gap`;
        if (dEl) dEl.innerText = `${lastCapturedData.duty}%`;
        if (rEl) rEl.innerText = lastCapturedData.missingTeeth >= 2 ? "3.0x" : "2.0x";
    }

    // 4. Cam Events Timeline
    if (msg.cmp && Array.isArray(msg.cmp)) {
        lastCapturedData.camEvents = msg.cmp;
        renderCamTimeline(msg.cmp);
    }

    // Refresh Scope Waveform
    if (msg.capState === 3 && capScope) {
        renderCapturePreview();
    }
}

function renderCamTimeline(events) {
    const strip = document.getElementById('capCamTimeline');
    if (!strip) return;
    strip.innerHTML = '';

    if (!events || events.length === 0) {
        strip.innerHTML = '<span class="timeline-empty">Menunggu pulsa cam (CMP)...</span>';
        return;
    }

    events.forEach((ev, idx) => {
        const chip = document.createElement('div');
        chip.className = `cam-timeline-chip ${ev.high ? 'chip-high' : 'chip-low'}`;
        chip.innerHTML = `
            <span class="chip-idx">#${idx + 1}</span>
            <span class="chip-angle">${ev.angle.toFixed(1)}°</span>
            <span class="chip-lvl">${ev.high ? 'HIGH' : 'LOW'}</span>
        `;
        strip.appendChild(chip);
    });
}

function setCapZoom(spanDeg, btn) {
    document.querySelectorAll('.btn-cap-tool').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');
    if (capScope) capScope.setZoomSpan(spanDeg);
}

function jumpCapToGap() {
    if (capScope) capScope.jumpToGap();
}

function replayCapturedToGenerator() {
    if (!lastCapturedData.totalTeeth) return;
    currentPattern.ckp.totalTeeth = lastCapturedData.totalTeeth;
    currentPattern.ckp.missingTeeth = lastCapturedData.missingTeeth;
    currentPattern.ckp.dutyCycle = (lastCapturedData.duty || 50) / 100.0;
    if (lastCapturedData.camEvents && lastCapturedData.camEvents.length > 0) {
        currentPattern.cmp.events = JSON.parse(JSON.stringify(lastCapturedData.camEvents));
    }

    updateTunerInputsFromState();
    renderCamEvents();
    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');

    switchMainModule('generator', true);
    const navBtns = document.querySelectorAll('.nav-item');
    if (navBtns[0]) switchTab('viewMonitor', navBtns[0], true);
}
