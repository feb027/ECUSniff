import { html, useRef, useState, useEffect } from '../preact.js';

export function Dial({ label, value, unit, min, max, step, onChange, disabled, subInfo, displayValue, accentColor, panelClass, compact = false }) {
    const trackRef = useRef(null);
    const isDraggingRef = useRef(false);
    const startPointerXRef = useRef(0);
    const startValRef = useRef(0);
    const hasMovedBeyondDeadbandRef = useRef(false);

    const lastSentValRef = useRef(null);
    const pendingValRef = useRef(null);
    const throttleTimerRef = useRef(null);

    const numMin = Number(min) || 0;
    const numMax = Number(max) || 100;
    const numStep = Number(step) || 1;
    const incomingVal = Number(value) || 0;

    const [dragVal, setDragVal] = useState(incomingVal);
    const [isDragging, setIsDragging] = useState(false);

    useEffect(() => {
        if (!isDraggingRef.current) {
            setDragVal(incomingVal);
        }
    }, [incomingVal]);

    const activeVal = isDragging ? dragVal : incomingVal;
    const dispVal = (displayValue !== undefined) ? displayValue : activeVal;

    const range = numMax - numMin;
    const pct = range > 0 ? Math.max(0, Math.min(100, ((activeVal - numMin) / range) * 100)) : 0;

    let dynamicColor = accentColor;
    if (!accentColor) {
        if (pct < 45) {
            dynamicColor = "#00ff66";
        } else if (pct < 75) {
            dynamicColor = "#ffb700";
        } else {
            dynamicColor = "#ff2d55";
        }
    }

    const calculateValueFromPointer = (e) => {
        if (!trackRef.current) return activeVal;
        const rect = trackRef.current.getBoundingClientRect();
        if (rect.width <= 0) return activeVal;
        
        let percentage = (e.clientX - rect.left) / rect.width;
        percentage = Math.max(0, Math.min(1, percentage));
        
        const rawValue = (percentage * (numMax - numMin)) + numMin;
        const steppedValue = Math.round(rawValue / numStep) * numStep;
        const decimals = (numStep.toString().split('.')[1] || '').length;
        const clamped = Math.max(numMin, Math.min(numMax, Number(steppedValue.toFixed(decimals))));
        return clamped;
    };

    const emitChange = (val, immediate = false) => {
        if (!onChange) return;
        pendingValRef.current = val;

        if (immediate) {
            if (throttleTimerRef.current) {
                clearTimeout(throttleTimerRef.current);
                throttleTimerRef.current = null;
            }
            lastSentValRef.current = val;
            onChange(val);
            return;
        }

        if (!throttleTimerRef.current) {
            lastSentValRef.current = val;
            onChange(val);
            throttleTimerRef.current = setTimeout(() => {
                throttleTimerRef.current = null;
                if (pendingValRef.current !== null && pendingValRef.current !== lastSentValRef.current) {
                    lastSentValRef.current = pendingValRef.current;
                    onChange(pendingValRef.current);
                }
            }, 50);
        }
    };

    const handlePointerDown = (e) => {
        if (disabled) return;
        isDraggingRef.current = true;
        setIsDragging(true);
        startPointerXRef.current = e.clientX;
        startValRef.current = activeVal;
        hasMovedBeyondDeadbandRef.current = false;

        if (trackRef.current) {
            try {
                trackRef.current.setPointerCapture(e.pointerId);
            } catch (err) {}
        }
        setDragVal(startValRef.current);
    };

    const handlePointerMove = (e) => {
        if (disabled || !isDraggingRef.current) return;

        const dx = Math.abs(e.clientX - startPointerXRef.current);
        if (!hasMovedBeyondDeadbandRef.current && dx < 4) {
            return;
        }
        hasMovedBeyondDeadbandRef.current = true;

        const newVal = calculateValueFromPointer(e);
        setDragVal(newVal);
        emitChange(newVal);
    };

    const handlePointerUp = (e) => {
        if (!isDraggingRef.current) return;
        isDraggingRef.current = false;
        setIsDragging(false);
        if (trackRef.current && trackRef.current.hasPointerCapture(e.pointerId)) {
            try {
                trackRef.current.releasePointerCapture(e.pointerId);
            } catch (err) {}
        }
        if (hasMovedBeyondDeadbandRef.current) {
            const finalVal = calculateValueFromPointer(e);
            setDragVal(finalVal);
            emitChange(finalVal, true);
        } else {
            setDragVal(startValRef.current);
        }
    };

    return html`
        <div class="${panelClass || 'panel'}" style="padding: 10px 14px; touch-action: pan-y; -webkit-user-select: none; user-select: none;">
            <div class="panel-header" style="margin-bottom: 2px; font-size: 0.72rem; display: flex; justify-content: space-between; align-items: center;">
                <span style="color: ${dynamicColor}; font-weight: 800; letter-spacing: 0.04em;">${label}</span>
                <span style="font-size: 0.68rem; color: var(--text-muted); font-weight: bold;">${min} - ${max} ${unit}</span>
            </div>

            <div class="huge-value" style="font-size: 1.85rem; line-height: 1.1; margin: 2px 0 4px 0;">
                <span style="color: ${dynamicColor}; font-weight: 900;">${dispVal}</span>
                <span class="value-unit" style="color: ${dynamicColor}; font-size: 0.82rem; margin-left: 4px; font-weight: bold;">${unit}</span>
            </div>

            ${subInfo ? html`
                <div style="font-size: 0.72rem; color: var(--text-muted); text-align: center; margin-top: -4px; margin-bottom: 6px;">
                    ${subInfo}
                </div>
            ` : ''}

            <div 
                class="slider-container" 
                style="opacity: ${disabled ? 0.35 : 1}; pointer-events: ${disabled ? 'none' : 'auto'}; padding: 12px 0; touch-action: pan-y;"
            >
                <div 
                    class="fader-track" 
                    ref=${trackRef}
                    style="height: 22px; border-radius: 11px; background: #0c0e14; border: 1.5px solid #2a313d; position: relative; cursor: pointer; touch-action: none; box-shadow: inset 0 2px 6px rgba(0,0,0,0.8);"
                    onPointerDown=${handlePointerDown}
                    onPointerMove=${handlePointerMove}
                    onPointerUp=${handlePointerUp}
                    onPointerCancel=${handlePointerUp}
                >
                    <div 
                        class="fader-fill" 
                        style="width: ${pct}%; height: 100%; border-radius: 10px; background: linear-gradient(to right, #00ff66 0%, #ffb700 65%, #ff2d55 100%); box-shadow: 0 0 12px ${dynamicColor}; pointer-events: none; transition: width 0.05s ease-out;"
                    ></div>

                    <div 
                        class="fader-thumb" 
                        style="position: absolute; top: 50%; left: ${pct}%; width: 34px; height: 34px; transform: translate(-50%, -50%); border-radius: 50%; background: #ffffff; border: 3.5px solid ${dynamicColor}; box-shadow: 0 0 16px ${dynamicColor}, 0 4px 10px rgba(0,0,0,0.9); pointer-events: none; display: flex; align-items: center; justify-content: center; z-index: 5;"
                    >
                        <div style="width: 10px; height: 10px; border-radius: 50%; background: ${dynamicColor}; box-shadow: 0 0 6px ${dynamicColor};"></div>
                    </div>
                </div>
            </div>
        </div>
    `;
}
