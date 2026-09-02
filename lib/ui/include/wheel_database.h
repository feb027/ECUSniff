#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../../engine/include/wheel_database.h"
#include "page_dashboard.h"

namespace EcuUi {

// Alias master engine database types and functions for UI subsystem
using BrandCategory = ::BrandCategory;
using WheelCycleDegrees = ::WheelCycleDegrees;
using WheelDefinition = ::WheelDefinition;
namespace WheelDatabase = ::WheelDatabase;

constexpr size_t OEM_DATABASE_COUNT = ::WheelDatabase::TOTAL_WHEELS;

// Legacy preset item array maintained for backward compatibility with PageDashboard
const WheelPresetItem OEM_DATABASE_PRESETS[OEM_DATABASE_COUNT] = {
    { "4 cylinder dizzy", 2, 0, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "6 cylinder dizzy", 3, 0, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "8 cylinder dizzy", 4, 0, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "60-2 crank only", 60, 2, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "60-2 crank and cam", 59, 2, 0, 0.50f, false, 2, {573.0f, 576.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "60-2 crank and 'half moon' cam", 59, 2, 0, 0.50f, false, 2, {258.0f, 618.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "36-1 crank only", 36, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "24-1 crank only", 24, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "4-1 crank wheel with cam", 4, 1, 0, 0.50f, false, 2, {450.0f, 495.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "8-1 crank only (R6)", 8, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "6-1 crank with cam", 6, 1, 0, 0.50f, false, 2, {480.0f, 520.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "12-1 crank with cam", 12, 1, 0, 0.50f, false, 2, {630.0f, 655.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "40-1 crank only (Ford V10)", 40, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Distributor style 4 cyl 50deg off, 40 deg on", 1, 0, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "odd fire 90 deg pattern 0 and 135 pulses", 3, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "GM OptiSpark LT1 360 and 8", 180, 0, 0, 0.50f, false, 4, {86.0f, 100.0f, 176.0f, 180.0f}, {true, false, true, false} },
    { "12-3 oddball", 12, 3, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "36-2-2-2 H4 Crank only", 32, 2, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Toyota Avanza Old 1.3", 31, 2, 0, 0.50f, false, 4, {185.0f, 245.0f, 365.0f, 425.0f}, {true, false, true, false} },
    { "Toyota Avanza New 1.5", 31, 2, 0, 0.50f, false, 2, {365.0f, 425.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Toyota Avanza/Xenia/Rush", 31, 2, 0, 0.50f, false, 4, {65.0f, 125.0f, 245.0f, 305.0f}, {true, false, true, false} },
    { "36-2-2-2 H6 Crank only", 32, 2, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Toyota 36-2-2-2 (K3/3SZ)", 31, 2, 0, 0.50f, false, 4, {15.0f, 20.0f, 225.0f, 230.0f}, {true, false, true, false} },
    { "GM 4200 crank wheel", 13, 6, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Mazda FE3 36-1 with cam", 36, 1, 0, 0.50f, false, 4, {90.0f, 105.0f, 420.0f, 435.0f}, {true, false, true, false} },
    { "Mitsubishi 6g72 with cam", 15, 23, 0, 0.50f, false, 4, {0.0f, 70.0f, 150.0f, 190.0f}, {true, false, true, false} },
    { "Buell Oddfire CAM wheel", 4, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "GM LS1 crank and cam", 26, 3, 0, 0.50f, false, 1, {360.0f, 0.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "GM 58x crank and 4x cam", 59, 2, 0, 0.50f, false, 4, {12.0f, 48.0f, 192.0f, 228.0f}, {true, false, true, false} },
    { "Odd Lotus 36-1-1-1-1 flywheel", 33, 1, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Honda RC51 with cam", 12, 0, 0, 0.50f, false, 4, {165.0f, 180.0f, 465.0f, 480.0f}, {true, false, true, false} },
    { "36-1 crank with 2nd trigger on teeth 33-34", 36, 1, 0, 0.50f, false, 2, {320.0f, 340.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Chrysler NGC 36+2-2 crank, NGC 4-cyl cam", 33, 2, 0, 0.50f, false, 4, {25.0f, 61.0f, 97.0f, 133.0f}, {true, false, true, false} },
    { "Chrysler NGC 36-2+2 crank, NGC 6-cyl cam", 33, 2, 0, 0.50f, false, 4, {0.0f, 2.0f, 13.0f, 23.0f}, {true, false, true, false} },
    { "Chrysler NGC 36-2+2 crank, NGC 8-cyl cam", 33, 2, 0, 0.50f, false, 4, {77.0f, 85.0f, 167.0f, 175.0f}, {true, false, true, false} },
    { "Nissan Livina Juke crank and cam", 33, 2, 0, 0.50f, false, 4, {17.0f, 40.0f, 197.0f, 220.0f}, {true, false, true, false} },
    { "Weber-Marelli 8 crank+2 cam pattern", 4, 0, 0, 0.50f, false, 4, {110.0f, 130.0f, 290.0f, 310.0f}, {true, false, true, false} },
    { "Fiat 1.8 16V crank and cam", 7, 2, 0, 0.50f, false, 4, {0.0f, 40.0f, 60.0f, 230.0f}, {true, false, true, false} },
    { "Nissan 360 CAS with 6 slots", 180, 0, 0, 0.50f, false, 4, {0.0f, 1.0f, 9.0f, 121.0f}, {true, false, true, false} },
    { "Mazda CAS 24-2 with single pulse outer ring", 12, 1, 0, 0.50f, false, 2, {300.0f, 370.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Yamaha 02-03 R1 8 tooth crank w/ 1 tooth cam", 8, 0, 0, 0.50f, false, 2, {371.2f, 416.2f, 0.0f, 0.0f}, {true, false, false, false} },
    { "GM 4 even-tooth crank with 1 tooth cam", 2, 0, 0, 0.50f, false, 1, {360.0f, 0.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "GM 6 even-tooth crank with 1 tooth cam", 3, 0, 0, 0.50f, false, 1, {360.0f, 0.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "GM 8 even-tooth crank with 1 tooth cam", 4, 0, 0, 0.50f, false, 1, {360.0f, 0.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Volvo d12[acd] crank with 7 tooth cam", 55, 2, 0, 0.50f, false, 4, {72.0f, 73.5f, 102.0f, 103.5f}, {true, false, true, false} },
    { "Mazda 36-2-2-2 with 6 tooth cam", 31, 2, 0, 0.50f, false, 4, {72.0f, 92.0f, 132.0f, 152.0f}, {true, false, true, false} },
    { "Mitsubishi 4g63 aka 4/2 crank and cam", 2, 0, 0, 0.50f, false, 4, {0.0f, 55.0f, 270.0f, 340.0f}, {true, false, true, false} },
    { "Audi 135 tooth crank and cam", 135, 0, 0, 0.50f, false, 3, {0.0f, 7.3f, 719.3f, 0.0f}, {true, false, true, false} },
    { "Honda D17 Crank (12+1)", 14, 2, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Honda Jazz Fit 04-08", 14, 2, 0, 0.50f, false, 4, {115.0f, 130.0f, 330.0f, 480.0f}, {true, false, true, false} },
    { "Honda Jazz Fit 04-08V2", 14, 2, 0, 0.50f, false, 4, {0.0f, 180.0f, 350.0f, 365.0f}, {true, false, true, false} },
    { "Honda Jazz Fit 04-08V3", 14, 2, 0, 0.50f, false, 4, {180.0f, 195.0f, 390.0f, 535.0f}, {true, false, true, false} },
    { "Mazda 323 AU version", 2, 0, 0, 0.50f, false, 4, {120.0f, 144.0f, 480.0f, 504.0f}, {true, false, true, false} },
    { "Daihatsu 3+1 distributor (3 cylinders)", 11, 7, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "Miata 99-05", 4, 0, 0, 0.50f, false, 4, {30.0f, 40.0f, 370.0f, 380.0f}, {true, false, true, false} },
    { "12/1 (12 crank with cam)", 12, 0, 0, 0.50f, false, 2, {630.0f, 655.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "24/1 (24 crank with cam)", 24, 0, 0, 0.50f, false, 2, {630.0f, 655.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Subaru 6/7 crank and cam", 7, 2, 0, 0.50f, false, 4, {5.0f, 8.0f, 11.0f, 14.0f}, {true, false, true, false} },
    { "GM 7X", 6, 5, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
    { "DSM 420a", 10, 5, 0, 0.50f, false, 4, {55.0f, 205.0f, 415.0f, 625.0f}, {true, false, true, false} },
    { "Ford ST170", 36, 1, 0, 0.50f, false, 4, {69.0f, 103.0f, 163.0f, 192.0f}, {true, false, true, false} },
    { "Mitsubishi 3A92", 33, 2, 0, 0.50f, false, 4, {100.0f, 105.0f, 340.0f, 345.0f}, {true, false, true, false} },
    { "Toyota 4AGE", 2, 0, 0, 0.50f, false, 2, {10.0f, 20.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Toyota 4AGZE", 6, 0, 0, 0.50f, false, 2, {10.0f, 15.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Suzuki DRZ400", 6, 0, 0, 0.50f, false, 4, {60.0f, 100.0f, 120.0f, 200.0f}, {true, false, true, false} },
    { "Jeep 2000", 13, 2, 0, 0.50f, false, 2, {146.0f, 506.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "BMW N20", 59, 2, 0, 0.50f, false, 4, {90.0f, 138.0f, 270.0f, 450.0f}, {true, false, true, false} },
    { "Dodge Viper V10 1996-2002", 11, 2, 0, 0.50f, false, 2, {0.0f, 360.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "36-2 with 1 tooth cam", 35, 2, 0, 0.50f, false, 2, {70.0f, 90.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "GM 40 tooth OSS wheel for Transmissions", 40, 0, 0, 0.50f, false, 0, {0.0f, 0.0f, 0.0f, 0.0f}, {false, false, false, false} },
};

} // namespace EcuUi
