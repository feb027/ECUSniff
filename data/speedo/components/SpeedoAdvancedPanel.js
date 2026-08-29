import { html } from '../preact.js';
import { Dial } from './Dial.js';

export function SpeedoAdvancedPanel({ state, sendAction, isSweep, isSweepActive, ppkPresets, pprPresets, maxRpm }) {
    const dacRouting = state.speedoDacRouting !== undefined ? state.speedoDacRouting : 3;
    const gaugeCurve = state.speedoGaugeCurve !== undefined ? state.speedoGaugeCurve : 0;
    const dacFuelDetected = state.speedoDacFuelDetected || false;
    const dacTempDetected = state.speedoDacTempDetected || false;
    const ppk = state.pulsePerKm || 4000;
    const ppr = state.speedoTachoPpr || 2.0;

    return html`
        <details class="panel" style="margin-top: var(--space-md); grid-column: 1 / -1;">
            <summary class="panel-header" style="cursor: pointer; user-select: none; padding-bottom: 4px;">
                <span style="font-weight: 700; letter-spacing: 0.08em;">PENGATURAN HARDWARE & KALIBRASI PER CHANNEL ▾</span>
            </summary>
            
            <div style="display: flex; flex-direction: column; gap: var(--space-md); padding-top: var(--space-md);">
                <!-- CARD CH 1: SPEEDOMETER & PPK SETTINGS -->
                <div class="panel-channel panel-channel-kmh" style="display: flex; flex-direction: column; gap: 12px;">
                    <div style="display: flex; align-items: center; gap: 8px;">
                        <span class="channel-tag" style="background: rgba(255,255,255,0.05); color: var(--neon-blue); border: 1px solid rgba(0,210,255,0.4);">CH 1</span>
                        <span style="font-size: 0.85rem; font-weight: 700; color: var(--neon-blue); letter-spacing: 0.05em;">PENGATURAN SPEEDOMETER (KM/H & PPK)</span>
                    </div>
                    <div>
                        <label style="font-size: 0.78rem; font-weight: 700; display: block; margin-bottom: 6px; color: var(--text-muted);">PULSES PER KM (PPK PRESETS):</label>
                        <div class="preset-btn-group">
                            ${ppkPresets.map(p => html`
                                <button class="btn ${ppk === p.val ? 'btn-active' : ''}" onClick=${() => sendAction('setPulsePerKm', p.val)} disabled=${!state.connected || isSweepActive}>${p.label}</button>
                            `)}
                        </div>
                    </div>
                    <div class="responsive-grid-2">
                        <${Dial} label="FINE TUNE PPK" value=${state.pulsePerKm} unit="P/KM" min="500" max="50000" step="10" accentColor="var(--neon-blue)" onChange=${(val) => sendAction('setPulsePerKm', val)} disabled=${!state.connected || isSweepActive} />
                        <${Dial} label="KMH STEP (ENCODER/UI)" value=${state.speedoKmhStep || 10} unit="KM/H" min="1" max="50" step="1" accentColor="var(--neon-blue)" onChange=${(val) => sendAction('setSpeedoKmhStep', val)} disabled=${!state.connected || isSweepActive} />
                    </div>
                </div>

                <!-- CARD CH 2: TACHOMETER (RPM) SETTINGS -->
                <div class="panel-channel panel-channel-rpm" style="display: flex; flex-direction: column; gap: 12px;">
                    <div style="display: flex; align-items: center; gap: 8px;">
                        <span class="channel-tag" style="background: rgba(255,255,255,0.05); color: var(--neon-green); border: 1px solid rgba(0,255,102,0.4);">CH 2</span>
                        <span style="font-size: 0.85rem; font-weight: 700; color: var(--neon-green); letter-spacing: 0.05em;">PENGATURAN TACHOMETER (RPM & PPR)</span>
                    </div>
                    <div>
                        <label style="font-size: 0.78rem; font-weight: 700; display: block; margin-bottom: 6px; color: var(--text-muted);">TACHOMETER PULSES PER REV (PPR / CYLINDERS):</label>
                        <div class="preset-btn-group">
                            ${pprPresets.map(p => html`
                                <button class="btn ${ppr === p.val ? 'btn-active' : ''}" onClick=${() => sendAction('setTachoPpr', p.val)} disabled=${!state.connected || isSweepActive}>${p.label}</button>
                            `)}
                        </div>
                    </div>
                    <div class="responsive-grid-2">
                        <${Dial} label="MAX RPM SCALE (TACHO)" value=${maxRpm} unit="RPM" min="1000" max="20000" step="500" accentColor="var(--neon-green)" onChange=${(val) => sendAction('setSpeedoMaxRpm', val)} disabled=${!state.connected || isSweepActive} />
                        <${Dial} label="RPM STEP (ENCODER/UI)" value=${state.speedoRpmStep || 500} unit="RPM" min="10" max="1000" step="10" accentColor="var(--neon-green)" onChange=${(val) => sendAction('setSpeedoRpmStep', val)} disabled=${!state.connected || isSweepActive} />
                    </div>
                </div>

                <!-- CARD CH 3: TEMPERATURE GAUGE SETTINGS -->
                <div class="panel-channel panel-channel-temp" style="display: flex; flex-direction: column; gap: 12px;">
                    <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 6px;">
                        <div style="display: flex; align-items: center; gap: 8px;">
                            <span class="channel-tag" style="background: rgba(255,255,255,0.05); color: var(--neon-orange); border: 1px solid rgba(255,153,0,0.4);">CH 3</span>
                            <span style="font-size: 0.85rem; font-weight: 700; color: var(--neon-orange); letter-spacing: 0.05em;">KALIBRASI 3-TITIK JARUM SUHU (TEMP)</span>
                        </div>
                        <div style="display: flex; gap: 4px;">
                            <button class="btn" style="padding: 3px 8px; font-size: 0.7rem;" onClick=${() => sendAction('setSpeedoTempCal', { min: 0, mid: 50, max: 100 })} disabled=${!state.connected || isSweepActive}>RESET (0/50/100)</button>
                            <button class="btn" style="padding: 3px 8px; font-size: 0.7rem;" onClick=${() => sendAction('setSpeedoTempCal', { min: 10, mid: 42, max: 85 })} disabled=${!state.connected || isSweepActive}>NTC GAUGE (10/42/85)</button>
                        </div>
                    </div>
                    <div style="display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 8px;">
                        <${Dial} label="MIN (COLD / 0%)" value=${state.speedoTempCalMin !== undefined ? state.speedoTempCalMin : 0} unit="%" min="0" max="100" step="1" accentColor="var(--neon-orange)" onChange=${(val) => sendAction('setSpeedoTempCal', { min: val })} disabled=${!state.connected || isSweepActive} />
                        <${Dial} label="MID (NORM / 50%)" value=${state.speedoTempCalMid !== undefined ? state.speedoTempCalMid : 50} unit="%" min="0" max="100" step="1" accentColor="var(--neon-orange)" onChange=${(val) => sendAction('setSpeedoTempCal', { mid: val })} disabled=${!state.connected || isSweepActive} />
                        <${Dial} label="MAX (HOT / 100%)" value=${state.speedoTempCalMax !== undefined ? state.speedoTempCalMax : 100} unit="%" min="0" max="100" step="1" accentColor="var(--neon-orange)" onChange=${(val) => sendAction('setSpeedoTempCal', { max: val })} disabled=${!state.connected || isSweepActive} />
                    </div>
                </div>

                <!-- CARD CH 4: FUEL GAUGE SETTINGS -->
                <div class="panel-channel panel-channel-fuel" style="display: flex; flex-direction: column; gap: 12px;">
                    <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 6px;">
                        <div style="display: flex; align-items: center; gap: 8px;">
                            <span class="channel-tag" style="background: rgba(255,255,255,0.05); color: var(--neon-yellow); border: 1px solid rgba(255,215,0,0.4);">CH 4</span>
                            <span style="font-size: 0.85rem; font-weight: 700; color: var(--neon-yellow); letter-spacing: 0.05em;">KALIBRASI 3-TITIK JARUM BENSIN (FUEL)</span>
                        </div>
                        <div style="display: flex; gap: 4px;">
                            <button class="btn" style="padding: 3px 8px; font-size: 0.7rem;" onClick=${() => sendAction('setSpeedoFuelCal', { min: 0, mid: 50, max: 100 })} disabled=${!state.connected || isSweepActive}>RESET (0/50/100)</button>
                        </div>
                    </div>
                    <div style="display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 8px;">
                        <${Dial} label="MIN (EMPTY / 0%)" value=${state.speedoFuelCalMin !== undefined ? state.speedoFuelCalMin : 0} unit="%" min="0" max="100" step="1" accentColor="var(--neon-yellow)" onChange=${(val) => sendAction('setSpeedoFuelCal', { min: val })} disabled=${!state.connected || isSweepActive} />
                        <${Dial} label="MID (HALF / 50%)" value=${state.speedoFuelCalMid !== undefined ? state.speedoFuelCalMid : 50} unit="%" min="0" max="100" step="1" accentColor="var(--neon-yellow)" onChange=${(val) => sendAction('setSpeedoFuelCal', { mid: val })} disabled=${!state.connected || isSweepActive} />
                        <${Dial} label="MAX (FULL / 100%)" value=${state.speedoFuelCalMax !== undefined ? state.speedoFuelCalMax : 100} unit="%" min="0" max="100" step="1" accentColor="var(--neon-yellow)" onChange=${(val) => sendAction('setSpeedoFuelCal', { max: val })} disabled=${!state.connected || isSweepActive} />
                    </div>
                </div>

                <!-- CARD 5: SYSTEM & HARDWARE CROSS-ROUTING -->
                <div class="panel" style="display: flex; flex-direction: column; gap: 12px; border-top: 3px solid #666;">
                    <div style="font-size: 0.85rem; font-weight: 700; color: var(--text-primary); letter-spacing: 0.05em;">PENGATURAN ROUTING HARDWARE & RESPON GAUGE</div>
                    <div style="background-color: #1a1a1a; padding: 12px; border-radius: 4px; border: 1px solid var(--border-sharp);">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                            <span style="font-size: 0.8rem; font-weight: 700; letter-spacing: 0.05em; color: var(--text-primary);">JALUR HARDWARE FUEL & TEMP (DUAL MCP4725 / PWM):</span>
                            <div style="display: flex; gap: 6px;">
                                <span class="status-badge" style="font-size: 0.68rem; border-color: ${dacFuelDetected ? 'var(--neon-green)' : 'var(--border-sharp)'}; color: ${dacFuelDetected ? 'var(--neon-green)' : 'var(--text-muted)'};">${dacFuelDetected ? 'DAC 1 (0x60 FUEL): ON' : 'DAC 1 (0x60): OFFLINE'}</span>
                                <span class="status-badge" style="font-size: 0.68rem; border-color: ${dacTempDetected ? 'var(--neon-green)' : 'var(--border-sharp)'}; color: ${dacTempDetected ? 'var(--neon-green)' : 'var(--text-muted)'};">${dacTempDetected ? 'DAC 2 (0x61 TEMP): ON' : 'DAC 2 (0x61): OFFLINE'}</span>
                            </div>
                        </div>
                        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 8px;">
                            <button class="btn ${dacRouting === 3 ? 'btn-active' : ''}" style="padding: 10px 6px; font-size: 0.74rem; font-weight: bold; border-color: ${dacRouting === 3 ? 'var(--neon-green)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoDacRouting', 3)} disabled=${!state.connected || isSweepActive}>DUAL MCP4725 (0x60 FUEL + 0x61 TEMP DC MURNI)</button>
                            <button class="btn ${dacRouting === 1 ? 'btn-active' : ''}" style="padding: 10px 6px; font-size: 0.74rem; font-weight: bold; border-color: ${dacRouting === 1 ? 'var(--neon-green)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoDacRouting', 1)} disabled=${!state.connected || isSweepActive}>SINGLE MCP4725 FUEL (0x60) + PWM TEMP</button>
                            <button class="btn ${dacRouting === 0 ? 'btn-active' : ''}" style="padding: 10px 6px; font-size: 0.74rem; font-weight: bold; border-color: ${dacRouting === 0 ? 'var(--neon-orange)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoDacRouting', 0)} disabled=${!state.connected || isSweepActive}>STANDAR DUAL PWM</button>
                            <button class="btn ${dacRouting === 2 ? 'btn-active' : ''}" style="padding: 10px 6px; font-size: 0.74rem; font-weight: bold; border-color: ${dacRouting === 2 ? 'var(--neon-blue)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoDacRouting', 2)} disabled=${!state.connected || isSweepActive}>SINGLE MCP4725 TEMP (0x61) + PWM FUEL</button>
                        </div>
                    </div>

                    <div style="background-color: #1a1a1a; padding: 12px; border-radius: 4px; border: 1px solid var(--border-sharp);">
                        <label style="font-size: 0.8rem; font-weight: 700; letter-spacing: 0.05em; display: block; margin-bottom: 8px; color: var(--neon-green);">KURVA RESPON JARUM BENSIN & SUHU:</label>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 8px;">
                            <button class="btn ${gaugeCurve === 0 ? 'btn-active' : ''}" style="padding: 10px 4px; font-size: 0.75rem; font-weight: bold; border-color: ${gaugeCurve === 0 ? 'var(--neon-green)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoGaugeCurve', 0)} disabled=${!state.connected || isSweepActive}>NON-LINIER (THERMAL / SQRT)</button>
                            <button class="btn ${gaugeCurve === 1 ? 'btn-active' : ''}" style="padding: 10px 4px; font-size: 0.75rem; font-weight: bold; border-color: ${gaugeCurve === 1 ? 'var(--neon-orange)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoGaugeCurve', 1)} disabled=${!state.connected || isSweepActive}>LINIER 1:1 (STANDAR DC / PWM)</button>
                        </div>
                    </div>

                    <div style="background-color: #1a1a1a; padding: 12px; border-radius: 4px; border: 1px solid var(--border-sharp);">
                        <div style="display: flex; justify-content: space-between; align-items: center;">
                            <div>
                                <div style="font-size: 0.85rem; font-weight: 800; color: var(--neon-purple);">🔄 MODE AUTO SWEEP (SEMUA CHANNEL)</div>
                                <div style="font-size: 0.72rem; color: var(--text-muted); margin-top: 2px;">Sapuan naik-turun otomatis untuk menguji jarum spidometer</div>
                            </div>
                            <button class="btn ${isSweep ? 'btn-active' : ''}" style="padding: 8px 16px; font-size: 0.82rem; font-weight: bold; border-color: ${isSweep ? 'var(--neon-purple)' : 'var(--border-sharp)'};" onClick=${() => sendAction('setSpeedoSweep', !isSweep)} disabled=${!state.connected}>${isSweep ? 'SWEEP: ON' : 'SWEEP: OFF'}</button>
                        </div>
                    </div>

                    <div>
                        <${Dial} label="SWEEP TIME" value=${state.sweepTimeSec || 5} unit="SEC" min="1" max="60" step="1" accentColor="var(--neon-purple)" onChange=${(val) => sendAction('setSweepTime', val)} disabled=${!state.connected} />
                    </div>
                </div>
            </div>
        </details>
    `;
}
