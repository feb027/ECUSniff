// Wheel Database Controller & Instant Search

let activeCategory = 'Semua';

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
    const custom = getCustomWheels();
    return [...custom, ...builtin];
}

function initWheelDatabase() {
    const cats = ['Semua', 'Kustom / Rekaman', 'Populer', 'Toyota', 'Honda', 'Mitsubishi', 'Subaru', 'Motor', 'Universal'];
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
        grid.innerHTML = '<div style="color: var(--text-muted); font-size: 0.75rem; text-align: center; padding: 16px;">Tidak ada pola roda yang cocok.</div>';
        return;
    }

    list.forEach(w => {
        const isCustom = w.id && String(w.id).startsWith('custom_');
        const card = document.createElement('div');
        card.className = 'wheel-card';
        card.innerHTML = `
            <div class="wheel-card-top">
                <span class="wheel-name">${w.name}</span>
                <div style="display: flex; gap: 4px; align-items: center;">
                    <span class="wheel-badge">${w.ckp.totalTeeth}-${w.ckp.missingTeeth} CKP</span>
                    ${isCustom ? `<button class="btn-del-event" style="font-size: 0.85rem;" onclick="deleteCustomWheel('${w.id}')" title="Hapus Pola Ini">&times;</button>` : ''}
                </div>
            </div>
            <p class="wheel-desc">${w.desc || 'Pola rekaman kustom'}</p>
            <button class="btn-select-wheel" onclick="applyWheelFromDb('${w.id}')">Gunakan Pola Ini</button>
        `;
        grid.appendChild(card);
    });
}

function deleteCustomWheel(id) {
    if (!confirm("Hapus pola rekaman kustom ini dari database?")) return;
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
    currentPattern.cmp.events = JSON.parse(JSON.stringify(w.cmp));

    updateTunerInputsFromState();
    renderCamEvents();
    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');

    const navBtns = document.querySelectorAll('.nav-item');
    if (navBtns[0]) switchTab('viewMonitor', navBtns[0], true);
}
