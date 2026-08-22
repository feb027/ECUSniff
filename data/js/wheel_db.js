// Automotive Wheel & Camshaft Pattern Database (ArduStim & OEM Library)

const WHEEL_DATABASE = [
    // 1. Populer & Universal
    {
        id: 'bosch_60_2',
        name: 'Bosch Motronic 60-2',
        category: 'Populer',
        desc: 'Standar universal Eropa: BMW, VW, Audi, Opel, Peugeot, Hyundai',
        ckp: { totalTeeth: 60, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 180, high: false }]
    },
    {
        id: 'toyota_36_2',
        name: 'Toyota / Daihatsu 36-2',
        category: 'Toyota',
        desc: 'Mesin Dual VVT-i: Avanza, Xenia, Vios, Yaris, Rush, Terios, Calya',
        ckp: { totalTeeth: 36, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 180, high: false }, { angle: 420, high: true }, { angle: 480, high: false }]
    },
    {
        id: 'honda_36_1',
        name: 'Honda 4-Silinder 36-1',
        category: 'Honda',
        desc: 'Mesin i-VTEC: Jazz GK5/GE8, Brio, City, Civic, K20A, L15A/Z',
        ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [
            { angle: 120, high: true }, { angle: 180, high: false },
            { angle: 300, high: true }, { angle: 360, high: false },
            { angle: 480, high: true }, { angle: 540, high: false },
            { angle: 660, high: true }, { angle: 720, high: false }
        ]
    },
    {
        id: 'universal_36_1',
        name: 'Universal Aftermarket 36-1',
        category: 'Universal',
        desc: 'Standar aftermarket ECU: Haltech, Speeduino, Megasquirt, MaxxECU',
        ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 180, high: false }, { angle: 420, high: true }, { angle: 470, high: false }]
    },
    {
        id: 'ford_36_1',
        name: 'Ford 36-1 EDIS',
        category: 'Universal',
        desc: 'Sistem pengapian Ford Duratec, Zetec, dan modul EDIS-4',
        ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 90, high: true }, { angle: 150, high: false }]
    },
    // 2. Pabrikan Khusus
    {
        id: 'mitsu_4g63',
        name: 'Mitsubishi 4G63 CAS',
        category: 'Mitsubishi',
        desc: 'Lancer Evolution 1-9 & Galant VR4 dengan sensor optik/Hall',
        ckp: { totalTeeth: 4, missingTeeth: 0, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 70, high: true }, { angle: 180, high: false }, { angle: 370, high: true }, { angle: 450, high: false }]
    },
    {
        id: 'subaru_36_2_2_2',
        name: 'Subaru 36-2-2-2',
        category: 'Subaru',
        desc: 'Impreza WRX / STI, Forester, Legacy mesin Boxer EJ20 & EJ25',
        ckp: { totalTeeth: 36, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 100, high: true }, { angle: 160, high: false }, { angle: 460, high: true }, { angle: 520, high: false }]
    },
    {
        id: 'toyota_24_1',
        name: 'Toyota 24 / 1 Distributor',
        category: 'Toyota',
        desc: 'Distributor 24 gigi crank + 1 gigi cam (4A-GE, 3S-GTE, 1JZ/2JZ non-VVTi)',
        ckp: { totalTeeth: 24, missingTeeth: 0, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 10, high: true }, { angle: 40, high: false }]
    },
    {
        id: 'honda_24_1',
        name: 'Honda 24 + 1 CAS',
        category: 'Honda',
        desc: 'Distributor internal seri B16A, B18C, B20B, D15B, D16',
        ckp: { totalTeeth: 24, missingTeeth: 0, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 0, high: true }, { angle: 30, high: false }]
    },
    // 3. Sepeda Motor & Mesin 1-2 Silinder
    {
        id: 'yamaha_12_1',
        name: 'Yamaha 12-1 EFI',
        category: 'Motor',
        desc: 'Injeksi Yamaha 1-Silinder: NMAX, Aerox, Vixion, R15, Lexi, Mio M3',
        ckp: { totalTeeth: 12, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 180, high: true }, { angle: 240, high: false }]
    },
    {
        id: 'honda_beat_12_1',
        name: 'Honda Beat / Vario 12-1',
        category: 'Motor',
        desc: 'Injeksi Honda PGM-FI: Beat FI, Vario 125/160, Scoopy, PCX 160, CBR150R',
        ckp: { totalTeeth: 12, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 160, high: true }, { angle: 220, high: false }]
    },
    {
        id: 'suzuki_24_2',
        name: 'Suzuki 24-2 (Hayabusa / GSX)',
        category: 'Motor',
        desc: 'Superbike Suzuki GSX-R1000, Hayabusa GSX-1300R, Satria F150 FI',
        ckp: { totalTeeth: 24, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 180, high: false }]
    },
    {
        id: 'harley_32_2',
        name: 'Harley Davidson 32-2',
        category: 'Motor',
        desc: 'Mesin V-Twin Twin Cam & Milwaukee-Eight EFI',
        ckp: { totalTeeth: 32, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 45, high: true }, { angle: 105, high: false }]
    }
];

window.WHEEL_DATABASE = WHEEL_DATABASE;
