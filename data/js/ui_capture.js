// Capture & Sniffer Cockpit Controller, Waveform Visualizer, & Auto-Match Inspector

let capScope = null;
let lastCapturedData = {
    hasData: false,
    rpm: 0,
    vehicle: "Belum Terdeteksi",
    confidence: 0,
    totalTeeth: 0,
    missingTeeth: 0,
    gapRatio: "--",
    duty: 0,
    camEvents: []
};

function initCaptureModule() {
    capScope = new ScopeVisualizer('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
    renderCapturePreview();
}

function renderCapturePreview() {
    if (!capScope) return;
    if (!lastCapturedData.hasData || lastCapturedData.totalTeeth === 0) {
        _drawStandbyGrid('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
        return;
    }

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
                ? lastCapturedData.camEvents : []
        }
    };
    capScope.render(pat);
}

function _drawStandbyGrid(mainId, miniId, probeId) {
    const mainCanvas = document.getElementById(mainId);
    const miniCanvas = document.getElementById(miniId);
    const probe = document.getElementById(probeId);

    if (mainCanvas) {
        const ctx = mainCanvas.getContext('2d');
        const w = mainCanvas.width = mainCanvas.clientWidth * (window.devicePixelRatio || 1);
        const h = mainCanvas.height = mainCanvas.clientHeight * (window.devicePixelRatio || 1);
        ctx.fillStyle = '#05080E';
        ctx.fillRect(0, 0, w, h);

        ctx.strokeStyle = '#111827';
        ctx.lineWidth = 1;
        for (let x = 0; x < w; x += 40) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke(); }
        for (let y = 0; y < h; y += 25) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke(); }

        ctx.strokeStyle = '#374151';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(0, h * 0.4); ctx.lineTo(w, h * 0.4);
        ctx.moveTo(0, h * 0.8); ctx.lineTo(w, h * 0.8);
        ctx.stroke();

        ctx.fillStyle = '#6B7280';
        ctx.font = '12px sans-serif';
        ctx.textAlign = 'center';
        ctx.fillText('STANDBY — Tekan "Mulai Capture" untuk Merekam Sinyal Mobil', w / 2, h / 2 + 4);
    }

    if (miniCanvas) {
        const ctx = miniCanvas.getContext('2d');
        const w = miniCanvas.width = miniCanvas.clientWidth * (window.devicePixelRatio || 1);
        const h = miniCanvas.height = miniCanvas.clientHeight * (window.devicePixelRatio || 1);
        ctx.fillStyle = '#070B12';
        ctx.fillRect(0, 0, w, h);
    }

    if (probe) {
        probe.innerText = "Pin: GPIO 34 (CKP IN) & GPIO 35 (CMP IN) — Filter Derau 5µs Aktif";
    }
}

function updateCaptureTelemetry(msg) {
    if (!msg) return;

    const badge = document.getElementById('capStateBadge');
    const btnArm = document.getElementById('btnArmCapture');

    if (badge) {
        const stateLabels = ["STANDBY", "SIAP MEREKAM", "MEREKAM...", "SELESAI"];
        const stateClasses = ["standby", "armed", "recording", "complete"];
        const curIdx = msg.capState || 0;
        badge.innerText = stateLabels[curIdx] || "STANDBY";
        badge.className = `mode-badge state-${stateClasses[curIdx] || "standby"}`;

        if (btnArm) {
            if (curIdx === 1) {
                btnArm.innerText = "Batalkan Rekam";
                btnArm.className = "btn-power-full btn-stop";
            } else if (curIdx === 2) {
                btnArm.innerText = "Sedang Merekam...";
                btnArm.className = "btn-power-full btn-stop";
            } else {
                btnArm.innerText = "Mulai Capture";
                btnArm.className = "btn-power-full btn-start";
            }
        }
    }

    if (msg.capState === 3) {
        if (msg.capRpm > 0) {
            lastCapturedData.rpm = msg.capRpm;
            lastCapturedData.hasData = true;
            const rpmEl = document.getElementById('capRpmDisplay');
            if (rpmEl) rpmEl.innerText = String(msg.capRpm).padStart(4, '0');
        }

        if (msg.capVehicle && msg.capVehicle.length > 0 && msg.capVehicle !== "Belum Terdeteksi") {
            lastCapturedData.vehicle = msg.capVehicle;
            const vEl = document.getElementById('capVehicleName');
            if (vEl) vEl.innerText = msg.capVehicle;
        }

        if (msg.capCkp) {
            lastCapturedData.totalTeeth = msg.capCkp.totalTeeth || 0;
            lastCapturedData.missingTeeth = msg.capCkp.missingTeeth || 0;
            lastCapturedData.duty = Math.round((msg.capCkp.dutyCycle || 0.5) * 100);

            const tEl = document.getElementById('metricTeeth');
            const mEl = document.getElementById('metricMissing');
            const dEl = document.getElementById('metricDuty');
            const rEl = document.getElementById('metricGapRatio');

            if (tEl) tEl.innerText = `${lastCapturedData.totalTeeth}T`;
            if (mEl) mEl.innerText = `${lastCapturedData.missingTeeth} Gap`;
            if (dEl) dEl.innerText = `${lastCapturedData.duty}%`;
            if (rEl) rEl.innerText = lastCapturedData.missingTeeth >= 2 ? "3.0x" : "2.0x";
        }

        if (msg.capCmp && Array.isArray(msg.capCmp)) {
            lastCapturedData.camEvents = msg.capCmp;
            renderCamTimeline(msg.capCmp);
        }

        if (capScope && lastCapturedData.totalTeeth > 0) {
            renderCapturePreview();
        }
    } else if (msg.capState === 0 && !lastCapturedData.hasData) {
        _drawStandbyGrid('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
    }
}

function renderCamTimeline(events) {
    const strip = document.getElementById('capCamTimeline');
    if (!strip) return;
    strip.innerHTML = '';

    if (!events || events.length === 0) {
        strip.innerHTML = '<span class="timeline-empty">Belum ada pulsa cam yang direkam</span>';
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

function saveCapturedToDatabase() {
    if (!lastCapturedData.hasData || lastCapturedData.totalTeeth === 0) {
        alert("Belum ada data capture yang terekam. Silakan tekan 'Mulai Capture' terlebih dahulu.");
        return;
    }

    const defaultName = lastCapturedData.vehicle !== "Belum Terdeteksi" 
        ? `${lastCapturedData.vehicle} (Capture)` 
        : `Rekaman ${lastCapturedData.totalTeeth}-${lastCapturedData.missingTeeth} (${lastCapturedData.rpm} RPM)`;

    const name = prompt("Beri nama pola kendaraan hasil capture ini:", defaultName);
    if (!name || name.trim() === '') return;

    let custom = getCustomWheels();
    const newWheel = {
        id: `custom_${Date.now()}`,
        name: name.trim(),
        category: 'Kustom / Rekaman',
        desc: `Hasil capture: ${lastCapturedData.totalTeeth}-${lastCapturedData.missingTeeth} CKP @ ${lastCapturedData.rpm} RPM (${(lastCapturedData.camEvents || []).length} Pulsa Cam)`,
        ckp: {
            totalTeeth: lastCapturedData.totalTeeth,
            missingTeeth: lastCapturedData.missingTeeth,
            missingPosition: 0,
            dutyCycle: (lastCapturedData.duty || 50) / 100.0,
            inverted: false
        },
        cmp: JSON.parse(JSON.stringify(lastCapturedData.camEvents || []))
    };

    custom.unshift(newWheel);
    localStorage.setItem('ecusniff_custom_wheels', JSON.stringify(custom));
    filterWheelDb();

    alert(`Pola "${newWheel.name}" berhasil disimpan ke Database Roda! Anda dapat membukanya kapan saja di tab 'DATABASE RODA'.`);
}

function replayCapturedToGenerator() {
    if (!lastCapturedData.totalTeeth) {
        alert("Belum ada data capture yang terekam. Silakan tekan 'Mulai Capture' terlebih dahulu.");
        return;
    }
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
