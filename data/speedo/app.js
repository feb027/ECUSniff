import { html, render, useState, useEffect, useRef } from './preact.js';
import { DashboardSpeedo } from './components/DashboardSpeedo.js';

function SpeedoApp() {
    const [state, setState] = useState({
        connected: false,
        isRunning: false,
        speedoKmh: 120,
        speedoRpm: 4000,
        speedoMaxRpm: 16000,
        speedoTempPercent: 50,
        speedoFuelPercent: 50,
        speedoEnableKmh: true,
        speedoEnableRpm: true,
        speedoEnableTemp: true,
        speedoEnableFuel: true,
        pulsePerKm: 4000,
        speedoTachoPpr: 2.0,
        speedoPwmFreqHz: 5000,
        speedoGaugeCurve: 0,
        speedoDacRouting: 3,
        speedoDacFuelDetected: false,
        speedoDacTempDetected: false,
        speedoAutoSweep: false,
        sweepTimeSec: 5.0,
        currentSpeedoKmh: 0,
        currentSpeedoRpm: 0,
        currentSpeedoTemp: 0,
        currentSpeedoFuel: 0
    });

    const wsRef = useRef(null);

    useEffect(() => {
        let ws;
        let timer;
        const connectWs = () => {
            const host = window.location.hostname || '192.168.4.1';
            const wsUrl = `ws://${host}/ws`;
            ws = new WebSocket(wsUrl);
            wsRef.current = ws;

            ws.onopen = () => {
                setState(prev => ({ ...prev, connected: true }));
            };

            ws.onclose = () => {
                setState(prev => ({ ...prev, connected: false }));
                timer = setTimeout(connectWs, 2000);
            };

            ws.onmessage = (ev) => {
                try {
                    const data = JSON.parse(ev.data);
                    if (data.speedo) {
                        const sp = data.speedo;
                        setState(prev => ({
                            ...prev,
                            isRunning: sp.isRunning !== undefined ? sp.isRunning : prev.isRunning,
                            speedoKmh: sp.kmh !== undefined ? sp.kmh : prev.speedoKmh,
                            speedoRpm: sp.rpm !== undefined ? sp.rpm : prev.speedoRpm,
                            speedoMaxRpm: sp.maxRpm !== undefined ? sp.maxRpm : prev.speedoMaxRpm,
                            speedoTempPercent: sp.temp !== undefined ? sp.temp : prev.speedoTempPercent,
                            speedoFuelPercent: sp.fuel !== undefined ? sp.fuel : prev.speedoFuelPercent,
                            speedoEnableKmh: sp.enKmh !== undefined ? sp.enKmh : prev.speedoEnableKmh,
                            speedoEnableRpm: sp.enRpm !== undefined ? sp.enRpm : prev.speedoEnableRpm,
                            speedoEnableTemp: sp.enTemp !== undefined ? sp.enTemp : prev.speedoEnableTemp,
                            speedoEnableFuel: sp.enFuel !== undefined ? sp.enFuel : prev.speedoEnableFuel,
                            pulsePerKm: sp.ppk !== undefined ? sp.ppk : prev.pulsePerKm,
                            speedoTachoPpr: sp.ppr !== undefined ? sp.ppr : prev.speedoTachoPpr,
                            speedoPwmFreqHz: sp.pwmFreq !== undefined ? sp.pwmFreq : prev.speedoPwmFreqHz,
                            speedoGaugeCurve: sp.curve !== undefined ? sp.curve : prev.speedoGaugeCurve,
                            speedoDacRouting: sp.dacRouting !== undefined ? sp.dacRouting : prev.speedoDacRouting,
                            speedoDacFuelDetected: sp.dacFuel !== undefined ? sp.dacFuel : prev.speedoDacFuelDetected,
                            speedoDacTempDetected: sp.dacTemp !== undefined ? sp.dacTemp : prev.speedoDacTempDetected,
                            speedoAutoSweep: sp.sweep !== undefined ? sp.sweep : prev.speedoAutoSweep,
                            sweepTimeSec: sp.sweepTime !== undefined ? sp.sweepTime : prev.sweepTimeSec,
                            currentSpeedoKmh: sp.liveKmh !== undefined ? sp.liveKmh : prev.currentSpeedoKmh,
                            currentSpeedoRpm: sp.liveRpm !== undefined ? sp.liveRpm : prev.currentSpeedoRpm,
                            currentSpeedoTemp: sp.liveTemp !== undefined ? sp.liveTemp : prev.currentSpeedoTemp,
                            currentSpeedoFuel: sp.liveFuel !== undefined ? sp.liveFuel : prev.currentSpeedoFuel
                        }));
                    }
                } catch (e) {
                    console.error(e);
                }
            };
        };

        connectWs();
        return () => {
            if (ws) ws.close();
            if (timer) clearTimeout(timer);
        };
    }, []);

    const sendAction = (action, payload) => {
        if (!wsRef.current || wsRef.current.readyState !== WebSocket.OPEN) return;
        let msg = {};
        if (action === 'toggleSpeedoRun') {
            msg = { cmd: 'speedo_toggle' };
        } else if (action === 'setSpeedoKmh') {
            msg = { cmd: 'speedo_set', kmh: payload };
        } else if (action === 'setSpeedoRpm') {
            msg = { cmd: 'speedo_set', rpm: payload };
        } else if (action === 'setSpeedoTemp') {
            msg = { cmd: 'speedo_set', temp: payload };
        } else if (action === 'setSpeedoFuel') {
            msg = { cmd: 'speedo_set', fuel: payload };
        } else if (action === 'toggleSpeedoChannel') {
            msg = { cmd: 'speedo_set_ch', ch: payload.channel, en: payload.value };
        } else if (action === 'setPulsePerKm') {
            msg = { cmd: 'speedo_set_ppk', val: payload };
        } else if (action === 'setTachoPpr') {
            msg = { cmd: 'speedo_set_ppr', val: payload };
        } else if (action === 'setSpeedoMaxRpm') {
            msg = { cmd: 'speedo_set_max_rpm', val: payload };
        } else if (action === 'setSpeedoTempCal') {
            msg = { cmd: 'speedo_set_temp_cal', ...payload };
        } else if (action === 'setSpeedoFuelCal') {
            msg = { cmd: 'speedo_set_fuel_cal', ...payload };
        } else if (action === 'setSpeedoGaugeCurve') {
            msg = { cmd: 'speedo_set_curve', val: payload };
        } else if (action === 'setSpeedoDacRouting') {
            msg = { cmd: 'speedo_set_dac_routing', val: payload };
        } else if (action === 'setSpeedoSweep') {
            msg = { cmd: 'speedo_set_sweep', val: payload ? 1 : 0 };
        } else if (action === 'setSweepTime') {
            msg = { cmd: 'speedo_set_sweep_time', val: payload };
        }
        wsRef.current.send(JSON.stringify(msg));
    };

    return html`
        <div id="app">
            <header>
                <div class="title">SPEEDOMETER & CLUSTER TESTER</div>
                <div class="status-badge">
                    <span class="status-dot ${state.connected ? 'connected' : 'disconnected'}"></span>
                    <span>${state.connected ? 'ONLINE' : 'OFFLINE'}</span>
                </div>
            </header>

            <div class="grid">
                <${DashboardSpeedo} 
                    state=${state}
                    sendAction=${sendAction}
                    modeSelector=${null}
                />
            </div>
        </div>
    `;
}

render(html`<${SpeedoApp} />`, document.getElementById('speedo-root'));
