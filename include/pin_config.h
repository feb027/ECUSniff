#pragma once
#include <stdint.h>

/**
 * @file pin_config.h
 * @brief Pinout mapping untuk Automotive ECU Test Platform (ECUSniff)
 */

namespace PinConfig {

// ==========================================
// 1. TFT SPI Display (KMRTM40045-SPI 480x320)
// ==========================================
constexpr int8_t TFT_CS   = 32;  ///< Chip Select
constexpr int8_t TFT_RST  = 17;  ///< Hardware Reset
constexpr int8_t TFT_DC   = 33;  ///< Data / Command
constexpr int8_t TFT_MOSI = 13;  ///< Master Out Slave In (SDI)
constexpr int8_t TFT_SCK  = 19;  ///< Serial Clock
constexpr int8_t TFT_LED  = 16;  ///< Backlight Control (PWM/GPIO)
constexpr int8_t TFT_MISO = -1;  ///< Not connected for display TX

// ==========================================
// 2. Rotary Encoder
// ==========================================
constexpr int8_t ENC_CLK  = 14;  ///< Encoder Clock (A)
constexpr int8_t ENC_DT   = 23;  ///< Encoder Data (B)
constexpr int8_t ENC_SW   = 27;  ///< Push Button Switch

// ==========================================
// 3. HW-504 Analog Joystick (2-Axis + Click)
// ==========================================
constexpr int8_t JOY_VRX  = 36;  ///< Joystick X Axis (ADC1_CH0 - WiFi Safe)
constexpr int8_t JOY_VRY  = 39;  ///< Joystick Y Axis (ADC1_CH3 - WiFi Safe)
constexpr int8_t JOY_SW   = 22;  ///< Joystick Push Button (GPIO Input Pullup)

// ==========================================
// 4. Engine Signal Outputs (Isolated Driver)
// ==========================================
constexpr int8_t SIG_CKP  = 25;  ///< Crankshaft Position Signal (RMT Ch 0)
constexpr int8_t SIG_CMP  = 26;  ///< Camshaft Position Signal 1 (RMT Ch 4)
constexpr int8_t SIG_CMP2 = 18;  ///< Camshaft Position Signal 2

// ==========================================
// 5. Engine Signal Inputs (Isolated Capture IN)
// ==========================================
constexpr int8_t CAP_CKP  = 34;  ///< Crankshaft Capture Input (Input-only)
constexpr int8_t CAP_CMP  = 35;  ///< Camshaft Capture Input 1 (Input-only)
constexpr int8_t CAP_CMP2 = 21;  ///< Camshaft Capture Input 2 (GPIO 21)

} // namespace PinConfig
