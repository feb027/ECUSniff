#ifndef TEST_WHEEL_DATA_ORACLE_H
#define TEST_WHEEL_DATA_ORACLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

enum class OracleBrandCategory : uint8_t {
    ALL = 0,
    TOYOTA_DAIHATSU,
    HONDA,
    MITSUBISHI,
    NISSAN,
    MAZDA,
    SUBARU,
    GM,
    FORD,
    CHRYSLER_JEEP_DODGE,
    EURO_US,
    UNIVERSAL,
    CUSTOM,
    COUNT
};

enum class OracleCycleDegrees : uint16_t {
    CRANK_360 = 360,
    ENGINE_720 = 720
};

struct OracleWheelDefinition {
    uint8_t id;
    const char* friendlyName;
    const char* shortName;
    const char* enumName;
    OracleBrandCategory category;
    OracleCycleDegrees cycleDegrees;
    uint16_t totalEdges;
    float rpmScaler;
    bool hasCkp;
    bool hasCmp1;
    bool hasCmp2;
};

constexpr size_t ORACLE_TOTAL_WHEELS = 70;

const OracleWheelDefinition ORACLE_WHEELS[ORACLE_TOTAL_WHEELS] = {
    { 0, "4 cylinder dizzy", "4-Cyl 7K-E 4A-FE", "DIZZY_FOUR_CYLINDER", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 4, 0.03333f, true, false, false },
    { 1, "6 cylinder dizzy", "6-Cyl 1G-FE", "DIZZY_SIX_CYLINDER", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 6, 0.05f, true, false, false },
    { 2, "8 cylinder dizzy", "8-Cyl Distributor", "DIZZY_EIGHT_CYLINDER", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 8, 0.06667f, true, false, false },
    { 3, "60-2 crank only", "60-2 KIA CKP Only", "SIXTY_MINUS_TWO", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 120, 1.0f, true, false, false },
    { 4, "60-2 crank and cam", "60-2 CKP+CMP", "SIXTY_MINUS_TWO_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 240, 1.0f, true, true, false },
    { 5, "60-2 crank and 'half moon' cam", "60-2 Half-Moon CMP", "SIXTY_MINUS_TWO_WITH_HALFMOON_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 240, 1.0f, true, true, false },
    { 6, "36-1 crank only", "36-1 CKP Only", "THIRTY_SIX_MINUS_ONE", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 72, 0.6f, true, false, false },
    { 7, "24-1 crank only", "24-1 CKP Only", "TWENTY_FOUR_MINUS_ONE", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 48, 0.5f, true, false, false },
    { 8, "4-1 crank wheel with cam", "4-1 CKP+CMP", "FOUR_MINUS_ONE_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 16, 0.06667f, true, true, false },
    { 9, "8-1 crank only (R6)", "8-1 R6 CKP", "EIGHT_MINUS_ONE", OracleBrandCategory::EURO_US, OracleCycleDegrees::CRANK_360, 16, 0.13333f, true, false, false },
    { 10, "6-1 crank with cam", "6-1 CKP+CMP", "SIX_MINUS_ONE_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 36, 0.15f, true, true, false },
    { 11, "12-1 crank with cam", "12-1 CKP+CMP", "TWELVE_MINUS_ONE_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 12, "40-1 crank only (Ford V10)", "40-1 Ford V10", "FOURTY_MINUS_ONE", OracleBrandCategory::FORD, OracleCycleDegrees::CRANK_360, 80, 0.66667f, true, false, false },
    { 13, "Distributor style 4 cyl 50deg off, 40 deg on", "Dist 4-Cyl 50/40", "DIZZY_FOUR_TRIGGER_RETURN", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 9, 0.15f, true, false, false },
    { 14, "odd fire 90 deg pattern 0 and 135 pulses", "Odd Fire 90/135deg", "ODDFIRE_VR", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 24, 0.2f, true, false, false },
    { 15, "GM OptiSpark LT1 360 and 8", "GM OptiSpark LT1", "OPTISPARK_LT1", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 16, "12-3 oddball", "12-3 Oddball", "TWELVE_MINUS_THREE", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 48, 0.4f, true, false, false },
    { 17, "36-2-2-2 H4 Crank only", "36-2-2-2 SWIFT H4", "THIRTY_SIX_MINUS_TWO_TWO_TWO", OracleBrandCategory::EURO_US, OracleCycleDegrees::CRANK_360, 72, 0.6f, true, false, false },
    { 18, "Toyota Avanza 1.3 Crank only", "Old Avanza", "OLD_AVANZA", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 19, "Toyota Avanza 1.5 Crank only", "New Avanza", "NEW_AVANZA", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 20, "Toyota Avanza/Xenia/Terios/Rush ", "Avanza/Xenia/Terios/Rush", "AVANZA_XENIA_TERIOS_RUSH", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 21, "36-2-2-2 H6 Crank only", "36-2-2-2 H6", "THIRTY_SIX_MINUS_TWO_TWO_TWO_H6", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::CRANK_360, 72, 0.6f, true, false, false },
    { 22, "36-2-2-2 Crank and cam", "36-2-2-2 K3-3SZ-EJ", "THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 23, "GM 4200 crank wheel", "GM 4200 CKP", "FOURTY_TWO_HUNDRED_WHEEL", OracleBrandCategory::GM, OracleCycleDegrees::CRANK_360, 72, 0.6f, true, false, false },
    { 24, "Mazda FE3 36-1 with cam", "36-1+CMP Mazda FE3", "THIRTY_SIX_MINUS_ONE_WITH_CAM_FE3", OracleBrandCategory::MAZDA, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 25, "Mitsubishi 6g72 with cam", "Mitsu 6G72+CMP", "SIX_G_SEVENTY_TWO_WITH_CAM", OracleBrandCategory::MITSUBISHI, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 26, "Buell Oddfire CAM wheel", "Buell Oddfire CMP", "BUELL_ODDFIRE_CAM", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 80, 0.33333f, true, false, false },
    { 27, "GM LS1 crank and cam", "GM LS1 CKP+CMP", "GM_LS1_CRANK_AND_CAM", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 720, 6.0f, true, true, true },
    { 28, "GM 58x crank and 4x cam", "GM 58x+4x CMP", "GM_58x_LS_CRANK_4X_CAM", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 240, 1.0f, true, true, false },
    { 29, "Odd Lotus 36-1-1-1-1 flywheel", "36-1-1-1-1 Lotus", "LOTUS_THIRTY_SIX_MINUS_ONE_ONE_ONE_ONE", OracleBrandCategory::EURO_US, OracleCycleDegrees::CRANK_360, 72, 0.6f, true, false, false },
    { 30, "Honda RC51 with cam", "Honda RC51+CMP", "HONDA_RC51_WITH_CAM", OracleBrandCategory::HONDA, OracleCycleDegrees::ENGINE_720, 48, 0.2f, true, true, false },
    { 31, "36-1 crank with 2nd trigger on teeth 33-34", "36-1+2nd Trigger", "THIRTY_SIX_MINUS_ONE_WITH_SECOND_TRIGGER", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 32, "Chrysler NGC 36+2-2 crank, NGC 4-cyl cam", "36+2-2 4-C Chrysler", "CHRYSLER_NGC_THIRTY_SIX_PLUS_TWO_MINUS_TWO_WITH_NGC4_CAM", OracleBrandCategory::CHRYSLER_JEEP_DODGE, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 33, "Chrysler NGC 36-2+2 crank, NGC 6-cyl cam", "36-2+2 6-C Chrysler", "CHRYSLER_NGC_THIRTY_SIX_MINUS_TWO_PLUS_TWO_WITH_NGC6_CAM", OracleBrandCategory::CHRYSLER_JEEP_DODGE, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 34, "Chrysler NGC 36-2+2 crank, NGC 8-cyl cam", "36-2+2 8-C Chrysler", "CHRYSLER_NGC_THIRTY_SIX_MINUS_TWO_PLUS_TWO_WITH_NGC8_CAM", OracleBrandCategory::CHRYSLER_JEEP_DODGE, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 35, "Nissan Livina Juke crank and cam", "Nissan Livina Juke", "NISSAN_LIVINA_JUKE", OracleBrandCategory::NISSAN, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 36, "Weber-Marelli 8 crank+2 cam pattern", "Weber-Marelli 8-C", "WEBER_IAW_WITH_CAM", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 144, 1.2f, true, true, false },
    { 37, "Fiat 1.8 16V crank and cam", "Fiat 1.8 16V C/C", "FIAT_ONE_POINT_EIGHT_SIXTEEN_VALVE_WITH_CAM", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 38, "Nissan 360 CAS with 6 slots", "Nissan 360 CAS 6-C", "THREE_SIXTY_NISSAN_CAS", OracleBrandCategory::NISSAN, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 39, "Mazda CAS 24-2 with single pulse outer ring", "Mazda CAS 24-2", "TWENTY_FOUR_MINUS_TWO_WITH_SECOND_TRIGGER", OracleBrandCategory::MAZDA, OracleCycleDegrees::ENGINE_720, 72, 0.3f, true, true, false },
    { 40, "Yamaha 2002-03 R1 8 even-tooth crank with 1 tooth cam", "Yamaha R1 02-03", "YAMAHA_EIGHT_TOOTH_WITH_CAM", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 64, 0.26667f, true, true, false },
    { 41, "GM 4 even-tooth crank with 1 tooth cam", "GM 4-Tooth+CMP", "GM_FOUR_TOOTH_WITH_CAM", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 8, 0.06666f, true, true, false },
    { 42, "GM 6 even-tooth crank with 1 tooth cam", "GM 6-Tooth+CMP", "GM_SIX_TOOTH_WITH_CAM", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 12, 0.1f, true, true, false },
    { 43, "GM 8 even-tooth crank with 1 tooth cam", "GM 8-Tooth+CMP", "GM_EIGHT_TOOTH_WITH_CAM", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 16, 0.13333f, true, true, false },
    { 44, "Volvo d12[acd] crank with 7 tooth cam", "Volvo D12ACD+CMP", "VOLVO_D12ACD_WITH_CAM", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 480, 4.0f, true, true, false },
    { 45, "Mazda 36-2-2-2 with 6 tooth cam", "36-2-2-2+6T MazdaRX8", "MAZDA_THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_SIX_TOOTH_CAM", OracleBrandCategory::MAZDA, OracleCycleDegrees::ENGINE_720, 360, 1.5f, true, true, false },
    { 46, "Mitsubishi 4g63 aka 4/2 crank and cam", "Mitsu 4G63 4/2", "MITSUBISH_4g63_4_2", OracleBrandCategory::MITSUBISHI, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 47, "Audi 135 tooth crank and cam", "Audi 135+CMP", "AUDI_135_WITH_CAM", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 1080, 1.5f, true, true, false },
    { 48, "Honda D17 Crank (12+1)", "12+1 Honda D17 ", "HONDA_D17_NO_CAM", OracleBrandCategory::HONDA, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, false, false },
    { 49, "Honda Jazz Fit 04-08", "Honda Jazz/Fit 04-08", "HONDA_JAZZ_FIT_04_08", OracleBrandCategory::HONDA, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 50, "Honda Jazz Fit 04-08V2", "Honda Jazz/Fit 04-08V2", "HONDA_JAZZ_FIT_04_08V2", OracleBrandCategory::HONDA, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 51, "Honda Jazz Fit 04-08V3", "Honda Jazz/Fit 04-08V3", "HONDA_JAZZ_FIT_04_08V3", OracleBrandCategory::HONDA, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 52, "Mazda 323 AU version", "Mazda 323 AU", "MAZDA_323_AU", OracleBrandCategory::MAZDA, OracleCycleDegrees::ENGINE_720, 30, 1.0f, true, true, false },
    { 53, "Daihatsu 3+1 distributor (3 cylinders)", "3+1 Daihatsu Taruna", "DAIHATSU_3CYL", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::CRANK_360, 144, 0.8f, true, false, false },
    { 54, "Miata 99-05", "Mazda Miata 99-05", "MIATA_9905", OracleBrandCategory::MAZDA, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 55, "12/1 (12 crank with cam)", "12-1 CKP+CMP", "TWELVE_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 56, "24/1 (24 crank with cam)", "24-1 CKP+CMP", "TWENTY_FOUR_WITH_CAM", OracleBrandCategory::UNIVERSAL, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 57, "Subaru 6/7 crank and cam", "Subaru 6/7 CKP+CMP", "SUBARU_SIX_SEVEN", OracleBrandCategory::SUBARU, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 58, "GM 7X", "GM SAAB 9-7X", "GM_7X", OracleBrandCategory::GM, OracleCycleDegrees::ENGINE_720, 180, 1.502f, true, false, false },
    { 59, "DSM 420a", "Eclipse DSM 420A", "FOUR_TWENTY_A", OracleBrandCategory::MITSUBISHI, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 60, "Ford ST170", "Ford ST170", "FORD_ST170", OracleBrandCategory::FORD, OracleCycleDegrees::ENGINE_720, 720, 3.0f, true, true, false },
    { 61, "Mitsubishi 3A92", "Mitsu 3A92 3-Cyl", "MITSUBISHI_3A92", OracleBrandCategory::MITSUBISHI, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 62, "Toyota 4AGE", "Toyota 4A-GE CAS", "TOYOTA_4AGE_CAS", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::ENGINE_720, 144, 0.333f, true, true, false },
    { 63, "Toyota 4AGZE", "Toyota 4A-GZE", "TOYOTA_4AGZE", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::ENGINE_720, 144, 0.333f, true, true, false },
    { 64, "Suzuki DRZ400", "Suzuki DRZ400", "SUZUKI_DRZ400", OracleBrandCategory::EURO_US, OracleCycleDegrees::CRANK_360, 72, 0.6f, true, true, false },
    { 65, "Jeep 2000", "Jeep 4L 6-C FT86", "JEEP2000", OracleBrandCategory::CHRYSLER_JEEP_DODGE, OracleCycleDegrees::ENGINE_720, 360, 1.5f, true, true, false },
    { 66, "BMW N20", "BMW N20 58x+CMP", "BMW_N20", OracleBrandCategory::EURO_US, OracleCycleDegrees::ENGINE_720, 240, 1.0f, true, true, true },
    { 67, "Dodge Viper V10 1996-2002", "Viper 96-02", "VIPER_96_02", OracleBrandCategory::CHRYSLER_JEEP_DODGE, OracleCycleDegrees::ENGINE_720, 240, 1.0f, true, true, false },
    { 68, "36-2 with 1 tooth cam", "36-2+1T 2JZ TYT 2AZ", "THIRTY_SIX_MINUS_TWO_WITH_ONE_CAM", OracleBrandCategory::TOYOTA_DAIHATSU, OracleCycleDegrees::ENGINE_720, 144, 0.6f, true, true, false },
    { 69, "GM 40 tooth OSS wheel for Transmissions", "GM40 Speedo Trans Sim", "GM_40_OSS", OracleBrandCategory::GM, OracleCycleDegrees::CRANK_360, 80, 1.0f, true, false, false }
};

inline const OracleWheelDefinition* getOracleWheel(size_t index) {
    if (index >= ORACLE_TOTAL_WHEELS) return nullptr;
    return &ORACLE_WHEELS[index];
}

inline const OracleWheelDefinition* findOracleWheelByFriendlyName(const char* name) {
    if (!name || strlen(name) == 0) return nullptr;
    for (size_t i = 0; i < ORACLE_TOTAL_WHEELS; ++i) {
        if (strcmp(ORACLE_WHEELS[i].friendlyName, name) == 0) {
            return &ORACLE_WHEELS[i];
        }
    }
    return nullptr;
}

#endif // TEST_WHEEL_DATA_ORACLE_H
