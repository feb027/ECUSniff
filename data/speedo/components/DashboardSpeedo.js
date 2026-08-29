import { html } from '../preact.js';
import { Dial } from './Dial.js';
import { SpeedoAdvancedPanel } from './SpeedoAdvancedPanel.js';

export function DashboardSpeedo({ state, sendAction, modeSelector }) {
    const isSweep = state.speedoAutoSweep === true || state.runMode === 3;
    const isRunning = state.isRunning;
    const isSweepActive = isSweep && isRunning;
    const ppr = state.speedoTachoPpr || 2.0;
    const ppk = state.pulsePerKm || 4000;
    const gaugeCurve = state.speedoGaugeCurve !== undefined ? state.speedoGaugeCurve : 0;
    const dacRouting = state.speedoDacRouting !== undefined ? state.speedoDacRouting : 3;

    const enableKmh = state.speedoEnableKmh !== false;
    const enableRpm = state.speedoEnableRpm !== false;
    const enableTemp = state.speedoEnableTemp !== false;
    const enableFuel = state.speedoEnableFuel !== false;

    const isKmhSweeping = isSweep && isRunning && enableKmh;
    const isRpmSweeping = isSweep && isRunning && enableRpm;
    const isTempSweeping = isSweep && isRunning && enableTemp;
    const isFuelSweeping = isSweep && isRunning && enableFuel;

    const liveKmh = isKmhSweeping ? (state.currentSpeedoKmh !== undefined ? state.currentSpeedoKmh : state.speedoKmh) : (enableKmh ? state.speedoKmh : 0);
    const liveRpm = isRpmSweeping ? (state.currentSpeedoRpm !== undefined ? state.currentSpeedoRpm : state.speedoRpm) : (enableRpm ? state.speedoRpm : 0);
    const liveTemp = isTempSweeping ? (state.currentSpeedoTemp !== undefined ? state.currentSpeedoTemp : state.speedoTempPercent) : (enableTemp ? state.speedoTempPercent : 0);
    const liveFuel = isFuelSweeping ? (state.currentSpeedoFuel !== undefined ? state.currentSpeedoFuel : state.speedoFuelPercent) : (enableFuel ? state.speedoFuelPercent : 0);

    const hzKmh = enableKmh ? ((liveKmh * ppk) / 3600.0).toFixed(1) : '0.0';
    const hzRpm = enableRpm ? ((liveRpm * ppr) / 60.0).toFixed(1) : '0.0';

    const calcEffectiveFrac = (percent) => {
        const frac = Math.max(0, Math.min(1, Number(percent) / 100));
        return (gaugeCurve === 0) ? Math.sqrt(frac) : frac;
    };

    const fuelFrac = calcEffectiveFrac(liveFuel);
    const tempFrac = calcEffectiveFrac(liveTemp);
    const fuelPwmDuty = (fuelFrac * 100).toFixed(1);
    const tempPwmDuty = (tempFrac * 100).toFixed(1);
    const fuelDacVolt = (fuelFrac * 5.0).toFixed(2);
    const tempDacVolt = (tempFrac * 5.0).toFixed(2);

    const isFuelDacActive = (dacRouting === 3 || dacRouting === 1);
    const isTempDacActive = (dacRouting === 3 || dacRouting === 2);
    const maxRpm = Number(state.speedoMaxRpm) || 16000;

    const maxRpmPresets = [
        { label: '6K', val: 6000 }, { label: '7K', val: 7000 },
        { label: '8K', val: 8000 }, { label: '10K', val: 10000 },
        { label: '12K', val: 12000 }, { label: '16K', val: 16000 }
    ];

    const ppkPresets = [
        { label: '2548 (JIS)', val: 2548 }, { label: '4000 (Univ)', val: 4000 },
        { label: '8000 (Euro)', val: 8000 }, { label: '23333 (Modern)', val: 23333 },
        { label: '30000 (Digital/ABS)', val: 30000 }
    ];

    const pprPresets = [
        { label: '1.0 PPR (1-Cyl)', val: 1.0 }, { label: '2.0 PPR (4-Cyl)', val: 2.0 },
        { label: '3.0 PPR (6-Cyl)', val: 3.0 }, { label: '4.0 PPR (8-Cyl)', val: 4.0 },
        { label: '0.5 PPR (Wasted)', val: 0.5 }
    ];

    const renderChannelToggle = (badgeNum, label, channelKey, isEnabled, liveHz, accentColor) => html`
        <div class="channel-toggle-bar" style="border-bottom: 1px solid rgba(255,255,255,0.08); padding-bottom: 10px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; gap: 10px; flex-wrap: wrap;">
            <div style="display: flex; align-items: center; gap: 8px;">
                <span class="channel-tag" style="background: rgba(255,255,255,0.05); color: ${accentColor}; border: 1px solid ${accentColor}50; font-size: 0.78rem; padding: 4px 8px;">${badgeNum}</span>
                <div>
                    <div style="font-size: 0.95rem; font-weight: 800; letter-spacing: 0.05em; color: ${isEnabled ? 'var(--text-primary)' : 'var(--text-muted)'};">${label}</div>
                    ${liveHz && isEnabled ? html`<div style="font-size: 0.75rem; color: ${accentColor}; font-family: monospace; font-weight: 600;">Frekuensi: ${liveHz} Hz [Output Aktif]</div>` : ''}
                </div>
            </div>
            <button class="btn ${isEnabled ? 'btn-active' : ''}" style="padding: 10px 18px; font-size: 0.85rem; font-weight: 800; border-radius: 4px; letter-spacing: 0.06em; min-width: 120px; ${isEnabled ? ('background: ' + accentColor + '; color: #000; border-color: ' + accentColor + '; box-shadow: 0 0 12px ' + accentColor + '60;') : 'color: #888; border-color: #444;'}" onClick=${() => sendAction('toggleSpeedoChannel', { channel: channelKey, value: !isEnabled })} disabled=${!state.connected}>
                ${isEnabled ? '● CH: ON' : '○ CH: OFF'}
            </button>
        </div>
    `;

    return html`
        <div class="panel-main">
            <!-- CH 1: SPEEDOMETER KM/H -->
            <div class="panel-channel panel-channel-kmh" style="display: flex; flex-direction: column; gap: var(--space-sm);">
                ${renderChannelToggle('CH 1', 'SPEED (KM/H)', 'kmh', enableKmh, isRunning && enableKmh ? hzKmh : '0.0', 'var(--neon-blue)')}
                <${Dial} 
                    label=${isSweep ? ("TARGET SPEED (MAKS: " + state.speedoKmh + " KM/H)") : "SPEED (KM/H)"}
                    value=${state.speedoKmh}
                    displayValue=${!enableKmh ? (isRunning ? 0 : state.speedoKmh) : (isKmhSweeping ? liveKmh : state.speedoKmh)}
                    unit="KM/H" min="0" max="300" step=${state.speedoKmhStep || 10} accentColor="var(--neon-blue)"
                    subInfo=${!enableKmh ? "CH 1 OFF (Muted) — Pin KMH diam (0 Hz / 0V)" : (isKmhSweeping ? ("Live Sweep: " + liveKmh + " KM/H (" + hzKmh + " Hz) [Output AKTIF]") : (isRunning ? ("Output: " + hzKmh + " Hz [Output AKTIF]") : "Standby"))}
                    onChange=${(val) => sendAction('setSpeedoKmh', val)} disabled=${!state.connected || isKmhSweeping}
                />
            </div>
        </div>
        
        <div class="panel-side-top" style="display: flex; flex-direction: column; gap: var(--space-md);">
            <!-- CH 2: TACHO RPM PANEL -->
            <div class="panel-channel panel-channel-rpm" style="display: flex; flex-direction: column; gap: var(--space-sm);">
                ${renderChannelToggle('CH 2', 'TACHOMETER (RPM)', 'rpm', enableRpm, isRunning && enableRpm ? hzRpm : '0.0', 'var(--neon-green)')}
                <div style="display: flex; align-items: center; justify-content: space-between; gap: 6px; background: rgba(0,0,0,0.35); padding: 5px 8px; border-radius: 3px; border: 1px solid rgba(255,255,255,0.06);">
                    <span style="font-size: 0.72rem; font-weight: 700; color: var(--text-muted); letter-spacing: 0.05em;">MAX SCALE:</span>
                    <div style="display: flex; gap: 3px; flex-wrap: wrap;">
                        ${maxRpmPresets.map(p => html`
                            <button class="btn ${maxRpm === p.val ? 'btn-active' : ''}" style="padding: 3px 7px; font-size: 0.70rem; border-radius: 2px; ${maxRpm === p.val ? 'border-color: var(--neon-green); color: var(--neon-green); font-weight: bold;' : ''}" onClick=${() => sendAction('setSpeedoMaxRpm', p.val)} disabled=${!state.connected || isRpmSweeping}>${p.label}</button>
                        `)}
                    </div>
                </div>
                <${Dial} 
                    label=${isSweep ? ("TARGET TACHO (MAKS: " + state.speedoRpm + " RPM)") : "TACHO (RPM)"}
                    value=${state.speedoRpm}
                    displayValue=${!enableRpm ? (isRunning ? 0 : state.speedoRpm) : (isRpmSweeping ? liveRpm : state.speedoRpm)}
                    unit="RPM" min="0" max=${maxRpm} step=${state.speedoRpmStep || 500} accentColor="var(--neon-green)"
                    subInfo=${!enableRpm ? "CH 2 OFF (Muted) — Pin RPM diam (0 Hz / 0V)" : (isRpmSweeping ? ("Live Sweep: " + liveRpm + " RPM (" + hzRpm + " Hz) [Output AKTIF]") : (isRunning ? ("Output: " + hzRpm + " Hz (" + ppr + " PPR)") : "Standby"))}
                    onChange=${(val) => sendAction('setSpeedoRpm', val)} disabled=${!state.connected || isRpmSweeping}
                />
            </div>

            <!-- CH 3: TEMPERATURE GAUGE PANEL -->
            <div class="panel-channel panel-channel-temp" style="display: flex; flex-direction: column; gap: var(--space-sm);">
                ${renderChannelToggle('CH 3', 'TEMPERATURE (ECT)', 'temp', enableTemp, null, 'var(--neon-orange)')}
                <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 4px;">
                    <button class="btn ${state.speedoTempPercent === 0 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoTemp', 0)} disabled=${!state.connected || isTempSweeping}>0% C</button>
                    <button class="btn ${state.speedoTempPercent === 50 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoTemp', 50)} disabled=${!state.connected || isTempSweeping}>50% MID</button>
                    <button class="btn ${state.speedoTempPercent === 80 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoTemp', 80)} disabled=${!state.connected || isTempSweeping}>80% HOT</button>
                    <button class="btn ${state.speedoTempPercent === 100 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem; color: var(--neon-red);" onClick=${() => sendAction('setSpeedoTemp', 100)} disabled=${!state.connected || isTempSweeping}>100% MAX</button>
                </div>
                <${Dial} 
                    label=${isSweep ? ("TARGET SUHU (MAKS: " + state.speedoTempPercent + "%)") : "SUHU ENGINE (ECT)"}
                    value=${state.speedoTempPercent}
                    displayValue=${!enableTemp ? (isRunning ? 0 : state.speedoTempPercent) : (isTempSweeping ? liveTemp : state.speedoTempPercent)}
                    unit="%" min="0" max="100" step=${state.speedoTempStep || 5} accentColor="var(--neon-orange)"
                    subInfo=${!enableTemp ? "CH 3 OFF (Muted)" : (isTempDacActive ? ("DAC 2 (0x61) -> " + tempDacVolt + "V DC Murni") : ("PWM -> " + tempPwmDuty + "% Duty"))}
                    onChange=${(val) => sendAction('setSpeedoTemp', val)} disabled=${!state.connected || isTempSweeping}
                />
            </div>

            <!-- CH 4: FUEL GAUGE PANEL -->
            <div class="panel-channel panel-channel-fuel" style="display: flex; flex-direction: column; gap: var(--space-sm);">
                ${renderChannelToggle('CH 4', 'FUEL LEVEL (BENSIN)', 'fuel', enableFuel, null, 'var(--neon-yellow)')}
                <div style="display: grid; grid-template-columns: repeat(5, 1fr); gap: 4px;">
                    <button class="btn ${state.speedoFuelPercent === 0 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoFuel', 0)} disabled=${!state.connected || isFuelSweeping}>E (0%)</button>
                    <button class="btn ${state.speedoFuelPercent === 25 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoFuel', 25)} disabled=${!state.connected || isFuelSweeping}>1/4</button>
                    <button class="btn ${state.speedoFuelPercent === 50 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoFuel', 50)} disabled=${!state.connected || isFuelSweeping}>1/2</button>
                    <button class="btn ${state.speedoFuelPercent === 75 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoFuel', 75)} disabled=${!state.connected || isFuelSweeping}>3/4</button>
                    <button class="btn ${state.speedoFuelPercent === 100 ? 'btn-active' : ''}" style="padding: 6px 2px; font-size: 0.72rem;" onClick=${() => sendAction('setSpeedoFuel', 100)} disabled=${!state.connected || isFuelSweeping}>F (100%)</button>
                </div>
                <${Dial} 
                    label=${isSweep ? ("TARGET BENSIN (MAKS: " + state.speedoFuelPercent + "%)") : "LEVEL BENSIN (FUEL)"}
                    value=${state.speedoFuelPercent}
                    displayValue=${!enableFuel ? (isRunning ? 0 : state.speedoFuelPercent) : (isFuelSweeping ? liveFuel : state.speedoFuelPercent)}
                    unit="%" min="0" max="100" step=${state.speedoFuelStep || 5} accentColor="var(--neon-yellow)"
                    subInfo=${!enableFuel ? "CH 4 OFF (Muted)" : (isFuelDacActive ? ("DAC 1 (0x60) -> " + fuelDacVolt + "V DC Murni") : ("PWM -> " + fuelPwmDuty + "% Duty"))}
                    onChange=${(val) => sendAction('setSpeedoFuel', val)} disabled=${!state.connected || isFuelSweeping}
                />
            </div>
        </div>

        <div class="sticky-run-bar">
            <button class="btn btn-run ${state.isRunning ? 'is-running' : ''}" onClick=${() => sendAction('toggleSpeedoRun')} disabled=${!state.connected}>
                ${state.isRunning ? 'MASTER RUN: ON (AKTIF)' : 'MASTER RUN: OFF (STANDBY)'}
            </button>
        </div>
        
        <${SpeedoAdvancedPanel} state=${state} sendAction=${sendAction} isSweep=${isSweep} isSweepActive=${isSweepActive} ppkPresets=${ppkPresets} pprPresets=${pprPresets} maxRpm=${maxRpm} />
    `;
}
