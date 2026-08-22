// Tuner, CKP Parameter Editor, and Camshaft Event Table Manager

function updateTunerInputsFromState() {
    const t = document.getElementById('tunerTeeth');
    const m = document.getElementById('tunerMissing');
    const p = document.getElementById('tunerPos');
    const d = document.getElementById('tunerDuty');
    const i = document.getElementById('tunerInvert');
    if (t) t.value = currentPattern.ckp.totalTeeth;
    if (m) m.value = currentPattern.ckp.missingTeeth;
    if (p) p.value = currentPattern.ckp.missingPosition;
    if (d) d.value = Math.round(currentPattern.ckp.dutyCycle * 100);
    if (i) i.checked = currentPattern.ckp.inverted;
}

function applyTunerParams() {
    currentPattern.ckp.totalTeeth = parseInt(document.getElementById('tunerTeeth').value) || 36;
    currentPattern.ckp.missingTeeth = parseInt(document.getElementById('tunerMissing').value) || 0;
    currentPattern.ckp.missingPosition = parseInt(document.getElementById('tunerPos').value) || 0;
    currentPattern.ckp.dutyCycle = (parseInt(document.getElementById('tunerDuty').value) || 50) / 100.0;
    currentPattern.ckp.inverted = document.getElementById('tunerInvert').checked;

    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');
}

function renderCamEvents() {
    const list = document.getElementById('tunerCmpList');
    if (!list) return;
    list.innerHTML = '';
    currentPattern.cmp.events.forEach((ev, i) => {
        const row = document.createElement('div');
        row.className = 'cam-event-row';
        row.innerHTML = `
            <span>Sudut <strong>${ev.angle.toFixed(1)}°</strong> &rarr; <span style="color:${ev.high ? '#10B981' : '#94A3B8'}">${ev.high ? 'HIGH' : 'LOW'}</span></span>
            <button class="btn-del-event" onclick="deleteCamEvent(${i})">&times;</button>
        `;
        list.appendChild(row);
    });
}

function addCamEvent() {
    const input = prompt("Masukkan sudut derajat cam (0 - 720 derajat):", "120");
    if (input === null) return;
    const angle = parseFloat(input);
    if (!isNaN(angle) && angle >= 0 && angle <= 720) {
        const isHigh = confirm("Set level transisi ke HIGH? (Pilih OK untuk HIGH, Cancel untuk LOW)");
        currentPattern.cmp.events.push({ angle, high: isHigh });
        currentPattern.cmp.events.sort((a, b) => a.angle - b.angle);
        renderCamEvents();
        if (scope) scope.render(currentPattern);
        sendCommand('set_pattern');
    }
}

function deleteCamEvent(idx) {
    currentPattern.cmp.events.splice(idx, 1);
    renderCamEvents();
    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');
}

function saveCustomTune() {
    const name = document.getElementById('customProfileName').value.trim() || 'my_tune';
    localStorage.setItem(`ecusniff_${name}`, JSON.stringify(currentPattern));
    alert(`Profil "${name}" berhasil disimpan.`);
}

function downloadTuneJson() {
    const name = document.getElementById('customProfileName').value.trim() || 'ecusniff_tune';
    const blob = new Blob([JSON.stringify(currentPattern, null, 2)], { type: 'application/json' });
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = `${name}.json`;
    link.click();
}
