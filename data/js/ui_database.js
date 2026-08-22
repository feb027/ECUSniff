// Wheel Database Controller & Instant Search

let activeCategory = 'Semua';

function initWheelDatabase() {
    const cats = ['Semua', 'Populer', 'Toyota', 'Honda', 'Mitsubishi', 'Subaru', 'Motor', 'Universal'];
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

    const list = (window.WHEEL_DATABASE || []).filter(w => {
        const matchCat = (activeCategory === 'Semua' || w.category === activeCategory);
        const matchText = w.name.toLowerCase().includes(q) || w.desc.toLowerCase().includes(q) || w.category.toLowerCase().includes(q);
        return matchCat && matchText;
    });

    list.forEach(w => {
        const card = document.createElement('div');
        card.className = 'wheel-card';
        card.innerHTML = `
            <div class="wheel-card-top">
                <span class="wheel-name">${w.name}</span>
                <span class="wheel-badge">${w.ckp.totalTeeth}-${w.ckp.missingTeeth} CKP</span>
            </div>
            <p class="wheel-desc">${w.desc}</p>
            <button class="btn-select-wheel" onclick="applyWheelFromDb('${w.id}')">Gunakan Pola Ini</button>
        `;
        grid.appendChild(card);
    });
}

function applyWheelFromDb(wheelId) {
    const w = (window.WHEEL_DATABASE || []).find(x => x.id === wheelId);
    if (!w) return;

    currentPattern.ckp = { ...w.ckp };
    currentPattern.cmp.events = JSON.parse(JSON.stringify(w.cmp));

    updateTunerInputsFromState();
    renderCamEvents();
    if (scope) scope.render(currentPattern);
    sendCommand('set_pattern');

    const navBtns = document.querySelectorAll('.nav-item');
    if (navBtns[0]) switchTab('viewMonitor', navBtns[0], true);
}
