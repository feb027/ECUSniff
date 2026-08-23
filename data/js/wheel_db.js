// Automotive Car Wheel & Camshaft Pattern Database (OEM Library)

const WHEEL_DATABASE = [
    // 1. Populer & Universal
    {
        id: 'bosch_60_2',
        name: 'Bosch Motronic 60-2',
        category: 'Universal',
        desc: 'Standar mobil Eropa & Universal: BMW, Mercedes, VW, Audi, Opel, Hyundai, Volvo',
        ckp: { totalTeeth: 60, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 180, high: true }, { angle: 540, high: false }]
    },
    {
        id: 'toyota_36_2',
        name: 'Toyota / Daihatsu 36-2',
        category: 'Toyota',
        desc: 'Mesin Dual VVT-i: Avanza, Xenia, Vios, Yaris, Rush, Terios, Calya, Innova',
        ckp: { totalTeeth: 36, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 90, high: true }, { angle: 270, high: false }]
    },
    {
        id: 'honda_36_1',
        name: 'Honda 4-Silinder 36-1',
        category: 'Honda',
        desc: 'Mesin i-VTEC: Jazz GK5/GE8, Brio, City, Civic, HR-V, CR-V, K20A, L15A/Z',
        ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 180, high: false }, { angle: 420, high: true }, { angle: 470, high: false }]
    },
    {
        id: 'universal_36_1',
        name: 'Universal Aftermarket 36-1',
        category: 'Universal',
        desc: 'Standar aftermarket ECU: Haltech, Speeduino, Megasquirt, MaxxECU, Link ECU',
        ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 180, high: false }, { angle: 420, high: true }, { angle: 470, high: false }]
    },
    {
        id: 'ford_36_1',
        name: 'Ford 36-1 EDIS',
        category: 'Ford',
        desc: 'Sistem pengapian Ford Duratec, Zetec, dan modul EDIS-4/6',
        ckp: { totalTeeth: 36, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 90, high: true }, { angle: 150, high: false }]
    },
    // 2. Pabrikan Khusus Mobil
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
        id: 'mazda_miata_24_2',
        name: 'Mazda Miata 24-2',
        category: 'Mazda',
        desc: 'Mazda MX-5 Miata NB / 323 / Protege mesin seri BP 1.8L',
        ckp: { totalTeeth: 24, missingTeeth: 2, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 120, high: true }, { angle: 360, high: false }]
    },
    {
        id: 'suzuki_daihatsu_12_1',
        name: 'Suzuki / Daihatsu 12-1',
        category: 'Suzuki/Daihatsu',
        desc: 'Mobil City Car: Suzuki Karimun, Ertiga K14B, Daihatsu Gran Max, Sigra K3-VE',
        ckp: { totalTeeth: 12, missingTeeth: 1, missingPosition: 0, dutyCycle: 0.5, inverted: false },
        cmp: [{ angle: 180, high: true }, { angle: 360, high: false }]
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
    }
];

window.WHEEL_DATABASE = WHEEL_DATABASE;
