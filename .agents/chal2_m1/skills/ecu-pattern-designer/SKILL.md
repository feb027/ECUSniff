# ECU Pattern Designer & Timing Math Guide
Panduan teknis dan formula matematis untuk merancang, memvalidasi, mengonversi, dan mendekode pola sinyal crankshaft (CKP) dan camshaft (CMP/CMP2) untuk simulator dan sniffer ECU.

1. Waktu 1 putaran 360 deg: T_rev = (60 * 10^6) / RPM (us)
2. Waktu 1 siklus 720 deg: T_cycle = 2 * T_rev = (120 * 10^6) / RPM (us)
3. Waktu per derajat: T_deg = 166666.67 / RPM (us/deg)
4. Bitmasking: Bit 0 = CKP (0x01), Bit 1 = CMP1 (0x02), Bit 2 = CMP2 (0x04)
