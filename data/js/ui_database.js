// Wheel Database Controller & Instant Search

let activeCategory = 'Semua';
let serverCustomSlots = [];

function setServerCustomSlots(slots) {
    if (!Array.isArray(slots)) return;
    serverCustomSlots = slots;
    filterWheelDb();
}

function getCustomWheels() {
    try {
        const raw = localStorage.getItem('ecusniff_custom_wheels');
        return raw ? JSON.parse(raw) : [];
    } catch (e) {
        return [];
    }
}

function getAllWheels() {
    const builtin = window.WHEEL_DATABASE || [];
    const localCustom = getCustomWheels();
    const serverCustom = serverCustomSlots.map(s => ({
        id: `slot_${s.slot}`,
        slotIdx: s.slot,
        name: s.name || `Capture ${s.slot + 1}`,
        category: 'Kustom / Rekaman',
        desc: `Pola tersimpan di Flash NVS (Slot #${s.slot + 1}) — ${s.teeth}-${s.mteeth} CKP`,
        isNvsSlot: true,
        ckp: { totalTeeth: s.teeth, missingTeeth: s.mteeth, missingPosition: 0, dutyCycle: s.duty || 0.5, inverted: false },
        cmp: []
    }));
    return [...serverCustom, ...localCustom, ...builtin];
}

function initWheelDatabase() {
    const cats = ['Semua', 'Kustom / Rekaman', 'Universal', 'Toyota', 'Honda', 'Mitsubishi', 'Subaru', 'Ford', 'Mazda'];
    const catContainer = document.getElementById('catScroll');
    if (!catContainer) return;
    catContainer.innerHTML = '';

    cats.forEach(c => {
        const chip = document.createElement('button');
        chip.className = `cat-chip ${c === activeCategory ? 'active' : ''}`;
        chip.innerText = c;
        chip.onclick = () => {
            activeCategory = c;
            document.querySelectorAll('.cat-chip').forEach(b => b.classList.remove('active'));
            chip.classList.add('active');
            filterWheelDb();
        };
        catContainer.appendChild(chip);
    });

    filterWheelDb();
}

function filterWheelDb() {
    const searchEl = document.getElementById('wheelSearch');
    const q = (searchEl ? searchEl.value : '').toLowerCase().trim();
    const grid = document.getElementById('wheelGrid');
    if (!grid) return;
    grid.innerHTML = '';

    const all = getAllWheels();
    const list = all.filter(w => {
        const matchCat = (activeCategory === 'Semua' || w.category === activeCategory);
        const matchText = w.name.toLowerCase().includes(q) || (w.desc && w.desc.toLowerCase().includes(q)) || (w.category && w.category.toLowerCase().includes(q));
        return matchCat && matchText;
    });

    if (list.length === 0) {
        grid.innerHTML = '<div style="color: var(--text-muted); font-size: 0.75rem; text-align: center; padding: 16px;">Tidak ada pola mobil yang cocok.</div>';
        return;
    }

    list.forEach(w => {
        const isLocal = w.id && String(w.id).startsWith('custom_');
        const isNvs = !!w.isNvsSlot;
        const card = document.createElement('div');
        card.className = 'wheel-card';
        card.innerHTML = `
            <div class="wheel-card-top">
                <span class="wheel-name" style="${isNvs ? 'color: #38BDF8;' : ''}">${w.name}</span>
                <div style="display: flex; gap: 4px; align-items: center;">
                    <span class="wheel-badge">${w.ckp.totalTeeth}-${w.ckp.missingTeeth} CKP</span>
                    ${isNvs ? `<button class="btn-action-sm" style="padding: 2px 6px; font-size: 0.65rem;" onclick="renameNvsSlot(${w.slotIdx}, '${w.name}')" title="Ubah Nama">✏️</button>` : ''}
                    ${isNvs ? `<button class="btn-del-event" style="font-size: 0.85rem;" onclick="deleteNvsSlot(${w.slotIdx})" title="Hapus Slot">&times;</button>` : ''}
                    ${isLocal ? `<button class="btn-del-event" style="font-size: 0.85rem;" onclick="deleteCustomWheel('${w.id}')" title="Hapus">&times;</button>` : ''}
                </div>
            </div>
            <p class="wheel-desc">${w.desc || 'Pola rekaman'}</p>
            <button class="btn-select-wheel" onclick="applyWheelFromDb('${w.id}')">Gunakan Pola Ini</button>
        `;
        grid.appendChild(card);
    });
}

function renameNvsSlot(slot, curName) {
    const newName = prompt("Beri nama baru untuk slot rekaman ini:", curName);
    if (!newName || newName.trim() === '') return;
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'rename_preset', slot: parseInt(slot), name: newName.trim() }));
    }
}

function deleteNvsSlot(slot) {
    if (!confirm("Hapus slot rekaman ini dari Flash ESP32?")) return;
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd: 'delete_preset', slot: parseInt(slot) }));
    }
}

function deleteCustomWheel(id) {
    if (!confirm("Hapus pola rekaman kustom ini dari browser?")) return;
    let custom = getCustomWheels().filter(w => w.id !== id);
    localStorage.setItem('ecusniff_custom_wheels', JSON.stringify(custom));
    filterWheelDb();
}

function applyWheelFromDb(wheelId) {
    const all = getAllWheels();
    const w = all.find(x => x.id === wheelId);
    if (!w) return;

    currentPattern.name = w.name;
    currentPattern.ckp = { ...w.ckp };
    if (w.cmp && w.cmp.length > 0) currentPattern.cmp.events = JSON.parse(JSON.stringify(w.cmp));

    updateTunerInputsFromState();
    renderCamEvents();
    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');

    const navBtns = document.querySelectorAll('.nav-item');
    if (navBtns[0]) switchTab('viewMonitor', navBtns[0], true);
}
