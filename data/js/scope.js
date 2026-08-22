// Dual-Layer Touch-Pan Mobile Oscilloscope Visualizer

class ScopeVisualizer {
    constructor(mainCanvasId, miniCanvasId, probeId) {
        this.mainCvs = document.getElementById(mainCanvasId);
        this.miniCvs = document.getElementById(miniCanvasId);
        this.mainCtx = this.mainCvs.getContext('2d');
        this.miniCtx = this.miniCvs.getContext('2d');
        this.probe = document.getElementById(probeId);

        this.startDeg = 0;      // Current pan offset (0 to 720)
        this.zoomSpan = 120;    // Default zoom span (e.g. 120 deg = ~12-20 teeth)
        this.currentPattern = null;

        this._setupTouchPan();
    }

    setZoomSpan(spanDeg) {
        this.zoomSpan = Math.max(30, Math.min(720, spanDeg));
        this._clampPan();
        if (this.currentPattern) this.render(this.currentPattern);
    }

    jumpToGap() {
        if (!this.currentPattern) return;
        const p = this.currentPattern.ckp;
        const degPerTooth = 360.0 / p.totalTeeth;
        const gapCenterDeg = (p.missingPosition + p.missingTeeth / 2.0) * degPerTooth;
        this.startDeg = Math.max(0, Math.min(720 - this.zoomSpan, gapCenterDeg - this.zoomSpan / 2.0));
        this.render(this.currentPattern);
    }

    render(pattern) {
        this.currentPattern = pattern;
        this._renderMinimap(pattern);
        this._renderMainScope(pattern);
    }

    _renderMinimap(pattern) {
        const cvs = this.miniCvs;
        const ctx = this.miniCtx;
        const dpr = window.devicePixelRatio || 1;
        const W = cvs.clientWidth || 320;
        const H = 22;

        cvs.width = W * dpr;
        cvs.height = H * dpr;
        ctx.scale(dpr, dpr);

        ctx.fillStyle = '#040508';
        ctx.fillRect(0, 0, W, H);

        // 720 Envelope preview
        const teeth720 = pattern.ckp.totalTeeth * 2;
        const pw = W / teeth720;
        ctx.fillStyle = 'rgba(245, 158, 11, 0.4)';
        for (let t = 0; t < teeth720; ++t) {
            const tidx = t % pattern.ckp.totalTeeth;
            const isMiss = (tidx >= pattern.ckp.missingPosition && tidx < pattern.ckp.missingPosition + pattern.ckp.missingTeeth);
            if (!isMiss) {
                ctx.fillRect(t * pw, 2, Math.max(1, pw * 0.7), H - 4);
            }
        }

        // Viewport Highlight Box
        const hlX = (this.startDeg / 720.0) * W;
        const hlW = (this.zoomSpan / 720.0) * W;
        ctx.strokeStyle = '#38BDF8';
        ctx.lineWidth = 1.5;
        ctx.fillStyle = 'rgba(56, 189, 248, 0.2)';
        ctx.fillRect(hlX, 1, hlW, H - 2);
        ctx.strokeRect(hlX, 1, hlW, H - 2);
    }

    _renderMainScope(pattern) {
        const cvs = this.mainCvs;
        const ctx = this.mainCtx;
        const dpr = window.devicePixelRatio || 1;
        const W = cvs.clientWidth || 320;
        const H = 190;

        cvs.width = W * dpr;
        cvs.height = H * dpr;
        ctx.scale(dpr, dpr);

        const minDeg = this.startDeg;
        const maxDeg = minDeg + this.zoomSpan;

        ctx.fillStyle = '#07090E';
        ctx.fillRect(0, 0, W, H);

        // 1. Degree Gridlines
        const gridStep = this.zoomSpan <= 60 ? 10 : (this.zoomSpan <= 180 ? 30 : 90);
        const firstGrid = Math.ceil(minDeg / gridStep) * gridStep;
        ctx.lineWidth = 1;

        for (let deg = firstGrid; deg <= maxDeg; deg += gridStep) {
            const x = ((deg - minDeg) / this.zoomSpan) * W;
            ctx.strokeStyle = (deg % 360 === 0) ? 'rgba(255, 255, 255, 0.3)' : 'rgba(255, 255, 255, 0.08)';
            ctx.beginPath();
            ctx.moveTo(x, 0); ctx.lineTo(x, H); ctx.stroke();

            ctx.fillStyle = '#64748B';
            ctx.font = '10px monospace';
            ctx.fillText(`${deg}°`, Math.min(x + 2, W - 28), H - 6);
        }

        // Trace separator
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
        ctx.beginPath(); ctx.moveTo(0, H / 2); ctx.lineTo(W, H / 2); ctx.stroke();

        // 2. CKP Trace & Tooth Badges
        const totalTeeth720 = pattern.ckp.totalTeeth * 2;
        const degPerTooth = 720.0 / totalTeeth720;
        const ckpLowY = H * 0.42;
        const ckpHighY = H * 0.14;

        ctx.strokeStyle = '#F59E0B';
        ctx.lineWidth = 2.2;
        ctx.beginPath();

        let started = false;
        for (let t = 0; t < totalTeeth720; ++t) {
            const tStartDeg = t * degPerTooth;
            const tHighEnd = tStartDeg + (degPerTooth * pattern.ckp.dutyCycle);
            const tEndDeg = (t + 1) * degPerTooth;

            if (tEndDeg < minDeg || tStartDeg > maxDeg) continue;

            const tidx = t % pattern.ckp.totalTeeth;
            const isMiss = (tidx >= pattern.ckp.missingPosition && tidx < pattern.ckp.missingPosition + pattern.ckp.missingTeeth);

            const x1 = ((tStartDeg - minDeg) / this.zoomSpan) * W;
            const xm = ((tHighEnd - minDeg) / this.zoomSpan) * W;
            const x2 = ((tEndDeg - minDeg) / this.zoomSpan) * W;

            const yLow = pattern.ckp.inverted ? ckpHighY : ckpLowY;
            const yHigh = pattern.ckp.inverted ? ckpLowY : ckpHighY;

            if (!started) { ctx.moveTo(x1, yLow); started = true; }

            if (isMiss) {
                ctx.lineTo(x2, yLow);
                // Label Gap
                ctx.fillStyle = '#E11D48';
                ctx.font = 'bold 9px monospace';
                ctx.fillText('GAP', x1 + 2, yLow - 8);
            } else {
                ctx.lineTo(x1, yHigh); ctx.lineTo(xm, yHigh); ctx.lineTo(xm, yLow); ctx.lineTo(x2, yLow);
                // Tooth number label if space permits
                if (this.zoomSpan <= 240) {
                    ctx.fillStyle = '#F59E0B';
                    ctx.font = '9px monospace';
                    ctx.fillText(`T${tidx + 1}`, x1 + 2, yHigh - 4);
                }
            }
        }
        ctx.stroke();

        // 3. CMP Trace
        const cmpLowY = H * 0.90;
        const cmpHighY = H * 0.62;

        ctx.strokeStyle = '#10B981';
        ctx.lineWidth = 2.2;
        ctx.beginPath();

        let curHigh = pattern.cmp.events.length > 0 && pattern.cmp.events[0].high ? false : true;
        ctx.moveTo(0, curHigh ? cmpHighY : cmpLowY);

        pattern.cmp.events.forEach(ev => {
            const evX = ((ev.angle - minDeg) / this.zoomSpan) * W;
            const targetY = ev.high ? cmpHighY : cmpLowY;
            ctx.lineTo(evX, curHigh ? cmpHighY : cmpLowY);
            ctx.lineTo(evX, targetY);
            curHigh = ev.high;
        });
        ctx.lineTo(W, curHigh ? cmpHighY : cmpLowY);
        ctx.stroke();

        // Trace Legend tags
        ctx.font = 'bold 10px monospace';
        ctx.fillStyle = '#F59E0B';
        ctx.fillText('CH1: CKP (GPIO 25)', 8, 16);
        ctx.fillStyle = '#10B981';
        ctx.fillText('CH2: CMP (GPIO 26)', 8, H / 2 + 16);
    }

    _setupTouchPan() {
        let isDragging = false;
        let startX = 0;
        let origStartDeg = 0;

        const onStart = (clientX) => {
            isDragging = true;
            startX = clientX;
            origStartDeg = this.startDeg;
        };

        const onMove = (clientX) => {
            if (!isDragging) return;
            const dx = clientX - startX;
            const W = this.mainCvs.clientWidth || 320;
            const degDelta = (dx / W) * this.zoomSpan;
            this.startDeg = origStartDeg - degDelta;
            this._clampPan();
            if (this.currentPattern) this.render(this.currentPattern);

            if (this.probe) {
                const centerDeg = this.startDeg + this.zoomSpan / 2.0;
                this.probe.innerText = `Sudut Tampilan: ${this.startDeg.toFixed(0)}° - ${(this.startDeg + this.zoomSpan).toFixed(0)}° (Pusat: ${centerDeg.toFixed(0)}°)`;
            }
        };

        const onEnd = () => { isDragging = false; };

        // Mouse events
        this.mainCvs.addEventListener('mousedown', (e) => onStart(e.clientX));
        window.addEventListener('mousemove', (e) => onMove(e.clientX));
        window.addEventListener('mouseup', onEnd);

        // Touch events
        this.mainCvs.addEventListener('touchstart', (e) => {
            if (e.touches.length > 0) onStart(e.touches[0].clientX);
        }, { passive: true });
        window.addEventListener('touchmove', (e) => {
            if (e.touches.length > 0) onMove(e.touches[0].clientX);
        }, { passive: true });
        window.addEventListener('touchend', onEnd);
    }

    _clampPan() {
        this.startDeg = Math.max(0, Math.min(720 - this.zoomSpan, this.startDeg));
    }
}

window.ScopeVisualizer = ScopeVisualizer;
