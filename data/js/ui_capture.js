// Capture Module UI & Diagnostic Inspector Controller

let capScope = null;
let lastCapturedData = {
    totalTeeth: 0, missingTeeth: 0, duty: 50, rpm: 0,
    vehicle: "Belum Terdeteksi", camEvents: [], hasData: false
};

function initCaptureModule() {
    capScope = new ScopeVisualizer('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
    _drawStandbyGrid('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
}

function renderCapturePreview() {
    if (!capScope) return;
    if (lastCapturedData.totalTeeth > 0) {
        capScope.render({
            ckp: {
                totalTeeth: lastCapturedData.totalTeeth, missingTeeth: lastCapturedData.missingTeeth,
                missingPosition: 0, dutyCycle: (lastCapturedData.duty || 50) / 100.0, inverted: false
            },
            cmp: { events: lastCapturedData.camEvents || [] }
        });
    } else {
        _drawStandbyGrid('capScopeMain', 'capScopeMinimap', 'capProbeReadout');
    }
}

function _drawStandbyGrid(mainId, miniId, probeId) {
    const cvs = document.getElementById(mainId), mini = document.getElementById(miniId), probe = document.getElementById(probeId);
    if (cvs && cvs.getContext) {
        const ctx = cvs.getContext('2d'), w = cvs.clientWidth || 320, h = cvs.clientHeight || 110;
        cvs.width = w; cvs.height = h; ctx.fillStyle = '#05080E'; ctx.fillRect(0, 0, w, h);
        ctx.strokeStyle = '#1E293B'; ctx.lineWidth = 1; ctx.setLineDash([4, 4]);
        for (let x = 0; x < w; x += 40) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke(); }
        for (let y = 0; y < h; y += 25) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke(); }
        ctx.setLineDash([]); ctx.fillStyle = '#64748B'; ctx.font = '11px monospace'; ctx.textAlign = 'center';
        ctx.fillText('PRE-FLIGHT DIAGNOSTIK HARDWARE AKTIF', w / 2, h / 2 + 4);
    }
    if (mini && mini.getContext) {
        const mCtx = mini.getContext('2d'), mw = mini.clientWidth || 320, mh = mini.clientHeight || 26;
        mini.width = mw; mini.height = mh; mCtx.fillStyle = '#070B12'; mCtx.fillRect(0, 0, mw, mh);
    }
    if (probe) probe.innerText = "Pin: GPIO 34 (CKP IN) & GPIO 35 (CMP IN) — Live Health Inspector Aktif";
}

function updateCaptureTelemetry(msg) {
    if (!msg) return;
    const badge = document.getElementById('capStateBadge'), btnArm = document.getElementById('btnArmCapture'), probe = document.getElementById('capProbeReadout');
    const diagCkp = document.getElementById('diagCkpVal'), diagCmp = document.getElementById('diagCmpVal');
    const diagCmp2 = document.getElementById('diagCmp2Val'), diagJitter = document.getElementById('diagJitterVal');

    if (msg.health) {
        const h = msg.health;
        if (probe) probe.innerText = `CKP: ${h.ckpOk ? 'OK' : 'DISCONNECTED'} | CMP: ${h.cmp1Ok ? 'OK' : 'IDLE'} | ${h.msg}`;
        if (diagCkp) {
            diagCkp.innerText = h.ckpOk ? `OK (${String(h.liveRpm || 0).padStart(4, '0')} RPM / ${h.liveTeeth || 0} gigi)` : 'TERPUTUS / TIDAK ADA';
            diagCkp.style.color = h.ckpOk ? '#10B981' : '#EF4444';
        }
        if (diagCmp) {
            diagCmp.innerText = h.cmp1Ok ? 'OK (LOCKED / AKTIF)' : 'IDLE / BELUM TERSAMBUNG';
            diagCmp.style.color = h.cmp1Ok ? '#10B981' : '#9CA3AF';
        }
        if (diagCmp2) {
            diagCmp2.innerText = h.cmp2Ok ? 'OK (LOCKED)' : 'STANDBY / NON-AKTIF';
            diagCmp2.style.color = h.cmp2Ok ? '#10B981' : '#6B7280';
        }
        if (diagJitter) {
            const jVal = (typeof h.jitter !== 'undefined') ? h.jitter : 0.2;
            diagJitter.innerText = `Jitter: ${jVal.toFixed(1)}% [${jVal < 2.0 ? 'BERSIH' : 'TINGGI'}]`;
            diagJitter.style.color = jVal < 2.0 ? '#10B981' : '#F59E0B';
        }
    }

    if (badge) {
        const curIdx = msg.capState || 0;
        if (curIdx === 1) { badge.innerText = "MENUNGGU 0° GAP..."; badge.className = "mode-badge state-armed"; }
        else if (curIdx === 2) { badge.innerText = "MEREKAM 720°..."; badge.className = "mode-badge state-recording"; }
        else if (curIdx === 3) { badge.innerText = "CAPTURE SELESAI"; badge.className = "mode-badge state-complete"; }
        else {
            if (msg.health && msg.health.quality === 3) { badge.innerText = "720° PHASE LOCKED"; badge.className = "mode-badge state-armed"; }
            else if (msg.health && msg.health.quality === 2) { badge.innerText = "MENYINKRONKAN..."; badge.className = "mode-badge state-standby"; }
            else if (msg.health && msg.health.quality === 1) { badge.innerText = "DERAU TINGGI"; badge.className = "mode-badge state-stop"; }
            else { badge.innerText = "TIDAK ADA SINYAL"; badge.className = "mode-badge state-standby"; }
        }

        if (btnArm) {
            if (curIdx === 1) { btnArm.innerText = "Batalkan Rekam"; btnArm.className = "btn-power-full btn-stop"; }
            else if (curIdx === 2) { btnArm.innerText = "Sedang Merekam..."; btnArm.className = "btn-power-full btn-stop"; }
            else {
                btnArm.innerText = (msg.health && msg.health.quality === 3) ? "Rekam (Phase-Locked)" : "Mulai Capture";
                btnArm.className = (msg.health && msg.health.quality === 3) ? "btn-power-full btn-start" : "btn-power-full btn-standby";
            }
        }
    }

    if (msg.capState === 3) {
        if (msg.capRpm > 0) {
            lastCapturedData.rpm = msg.capRpm; lastCapturedData.hasData = true;
            const rpmEl = document.getElementById('capRpmDisplay'); if (rpmEl) rpmEl.innerText = String(msg.capRpm).padStart(4, '0');
        }
        if (msg.capVehicle && msg.capVehicle.length > 0 && msg.capVehicle !== "Belum Terdeteksi") {
            lastCapturedData.vehicle = msg.capVehicle;
            const vEl = document.getElementById('capVehicleName'); if (vEl) vEl.innerText = msg.capVehicle;
        }
        if (msg.capCkp) {
            lastCapturedData.totalTeeth = msg.capCkp.totalTeeth || 0; lastCapturedData.missingTeeth = msg.capCkp.missingTeeth || 0;
            lastCapturedData.duty = Math.round((msg.capCkp.dutyCycle || 0.5) * 100);
            const tEl = document.getElementById('metricTeeth'), mEl = document.getElementById('metricMissing');
            const dEl = document.getElementById('metricDuty'), rEl = document.getElementById('metricGapRatio');
            if (tEl) tEl.innerText = `${lastCapturedData.totalTeeth}T`; if (mEl) mEl.innerText = `${lastCapturedData.missingTeeth} Gap`;
            if (dEl) dEl.innerText = `${lastCapturedData.duty}%`; if (rEl) rEl.innerText = lastCapturedData.missingTeeth >= 2 ? "3.0x" : "2.0x";
        }
        if (msg.capCmp && Array.isArray(msg.capCmp)) {
            lastCapturedData.camEvents = msg.capCmp; renderCamTimeline(msg.capCmp);
        }
        if (capScope && lastCapturedData.totalTeeth > 0) renderCapturePreview();
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
        chip.innerHTML = `<span class="chip-idx">#${idx + 1}</span><span class="chip-angle">${ev.angle.toFixed(1)}°</span><span class="chip-lvl">${ev.high ? 'HIGH' : 'LOW'}</span>`;
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
        alert("Belum ada data capture yang terekam. Silakan tekan 'Mulai Capture' terlebih dahulu."); return;
    }
    const defaultName = lastCapturedData.vehicle !== "Belum Terdeteksi" ? `${lastCapturedData.vehicle} (Capture)` : `Rekaman ${lastCapturedData.totalTeeth}-${lastCapturedData.missingTeeth} (${lastCapturedData.rpm} RPM)`;
    const name = prompt("Beri nama pola kendaraan hasil capture ini:", defaultName);
    if (!name || name.trim() === '') return;

    let custom = getCustomWheels();
    const newWheel = {
        id: `custom_${Date.now()}`, name: name.trim(), category: 'Kustom / Rekaman',
        desc: `Hasil capture: ${lastCapturedData.totalTeeth}-${lastCapturedData.missingTeeth} CKP @ ${lastCapturedData.rpm} RPM`,
        ckp: { totalTeeth: lastCapturedData.totalTeeth, missingTeeth: lastCapturedData.missingTeeth, missingPosition: 0, dutyCycle: (lastCapturedData.duty || 50) / 100.0, inverted: false },
        cmp: JSON.parse(JSON.stringify(lastCapturedData.camEvents || []))
    };
    custom.unshift(newWheel);
    localStorage.setItem('ecusniff_custom_wheels', JSON.stringify(custom));
    filterWheelDb();

    currentPattern.name = newWheel.name; currentPattern.ckp = { ...newWheel.ckp }; currentPattern.cmp.events = JSON.parse(JSON.stringify(newWheel.cmp));
    sendCommand('set_pattern');
    alert(`Pola "${newWheel.name}" berhasil disimpan ke Flash ESP32 & Database Roda!`);
}

function replayCapturedToGenerator() {
    if (!lastCapturedData.totalTeeth) {
        alert("Belum ada data capture yang terekam. Silakan tekan 'Mulai Capture' terlebih dahulu."); return;
    }
    currentPattern.name = `Captured: ${lastCapturedData.totalTeeth}-${lastCapturedData.missingTeeth}`;
    currentPattern.ckp.totalTeeth = lastCapturedData.totalTeeth; currentPattern.ckp.missingTeeth = lastCapturedData.missingTeeth;
    currentPattern.ckp.dutyCycle = (lastCapturedData.duty || 50) / 100.0;
    if (lastCapturedData.camEvents && lastCapturedData.camEvents.length > 0) {
        currentPattern.cmp.events = JSON.parse(JSON.stringify(lastCapturedData.camEvents));
    }
    updateTunerInputsFromState(); renderCamEvents();
    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');
    switchMainModule('generator', true);
    const navBtns = document.querySelectorAll('.nav-item');
    if (navBtns[0]) switchTab('viewMonitor', navBtns[0], true);
}

function exportCapturedJson() {
    if (!lastCapturedData.hasData || lastCapturedData.totalTeeth === 0) {
        alert("Belum ada data capture yang terekam."); return;
    }
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(lastCapturedData, null, 2));
    const dlAnchorElem = document.createElement('a');
    dlAnchorElem.setAttribute("href", dataStr);
    dlAnchorElem.setAttribute("download", `ecusniff_capture_${lastCapturedData.totalTeeth}_${lastCapturedData.missingTeeth}.json`);
    dlAnchorElem.click();
}
