#pragma once
#include <stdint.h>

/**
 * @file pin_config.h
 * @brief Pinout mapping untuk Automotive ECU Test Platform (ESP32-S3 & ESP32)
 */

namespace PinConfig {

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ESP32S3) || defined(ARDUINO_USB_CDC_ON_BOOT)

// ============================================================================
// TARGET: ESP32-S3 DevKitC-1 (N8R8 / N16R8 Octal PSRAM & USB-CDC Safe)
// ============================================================================

// 1. TFT SPI Display (Hardware FSPI Bus on ESP32-S3)
constexpr int8_t TFT_MOSI = 11;  ///< Hardware FSPI MOSI (Pin 11)
constexpr int8_t TFT_SCK  = 12;  ///< Hardware FSPI SCK  (Pin 12)
constexpr int8_t TFT_MISO = -1;  ///< Hardware FSPI MISO (Pin 13)
constexpr int8_t TFT_CS   = 10;  ///< Chip Select (Pin 10)
constexpr int8_t TFT_DC   = 9;   ///< Data / Command (Pin 9)
constexpr int8_t TFT_RST  = 14;  ///< Hardware Reset (Pin 14)
constexpr int8_t TFT_LED  = 15;  ///< Backlight PWM Control (Pin 15)

// 2. Rotary Encoder (Precision Parameter Value Tuner)
constexpr int8_t ENC_CLK  = 16;  ///< Encoder Clock / A (Pin 16)
constexpr int8_t ENC_DT   = 17;  ///< Encoder Data / B  (Pin 17)
constexpr int8_t ENC_SW   = 18;  ///< Push Button Switch (Pin 18)

// 3. HW-504 Analog Joystick (2-Axis X/Y + Click)
constexpr int8_t JOY_VRX  = 1;   ///< Joystick X Axis (ADC1_CH0 - WiFi Safe)
constexpr int8_t JOY_VRY  = 2;   ///< Joystick Y Axis (ADC1_CH1 - WiFi Safe)
constexpr int8_t JOY_SW   = 42;  ///< Joystick Push Button (GPIO 42 Input Pullup)

// 4. Engine Signal Outputs (Isolated Driver / RMT Ch 0, 1, 2)
constexpr int8_t SIG_CKP  = 4;   ///< Crankshaft Signal OUT (RMT Ch 0)
constexpr int8_t SIG_CMP  = 5;   ///< Camshaft Signal 1 OUT (RMT Ch 1)
constexpr int8_t SIG_CMP2 = 6;   ///< Camshaft Signal 2 OUT (RMT Ch 2)

// 5. Engine Signal Inputs (Isolated Capture IN / Sniffer)
constexpr int8_t CAP_CKP  = 7;   ///< Crankshaft Capture IN (Interrupt Pin 7)
constexpr int8_t CAP_CMP  = 8;   ///< Camshaft Capture 1 IN (Interrupt Pin 8)
constexpr int8_t CAP_CMP2 = 21;  ///< Camshaft Capture 2 IN (Interrupt Pin 21)

// 6. Dedicated EPS / VSS Tester Outputs
constexpr int8_t EPS_VSS  = 38;  ///< Vehicle Speed Sensor (VSS) Pulse OUT
constexpr int8_t EPS_RPM  = 39;  ///< Engine Speed Tachometer Pulse OUT
constexpr int8_t EPS_TRQ1 = 40;  ///< Steering Torque Sensor 1 / PWM OUT
constexpr int8_t EPS_TRQ2 = 41;  ///< Steering Torque Sensor 2 / PWM OUT

#else

// ============================================================================
// TARGET: ESP32 Classic (WEMOS D1 R32 Base)
// ============================================================================

// 1. TFT SPI Display (KMRTM40045-SPI 480x320)
constexpr int8_t TFT_CS   = 32;  ///< Chip Select
constexpr int8_t TFT_RST  = 17;  ///< Hardware Reset
constexpr int8_t TFT_DC   = 33;  ///< Data / Command
constexpr int8_t TFT_MOSI = 13;  ///< Master Out Slave In (SDI)
constexpr int8_t TFT_SCK  = 19;  ///< Serial Clock
constexpr int8_t TFT_LED  = 16;  ///< Backlight Control
constexpr int8_t TFT_MISO = -1;

// 2. Rotary Encoder
constexpr int8_t ENC_CLK  = 14;  ///< Encoder Clock (A)
constexpr int8_t ENC_DT   = 23;  ///< Encoder Data (B)
constexpr int8_t ENC_SW   = 27;  ///< Push Button Switch

// 3. HW-504 Analog Joystick
constexpr int8_t JOY_VRX  = 36;  ///< Joystick X Axis (ADC1_CH0)
constexpr int8_t JOY_VRY  = 39;  ///< Joystick Y Axis (ADC1_CH3)
constexpr int8_t JOY_SW   = 22;  ///< Joystick Push Button

// 4. Engine Signal Outputs
constexpr int8_t SIG_CKP  = 25;  ///< Crankshaft Position Signal
constexpr int8_t SIG_CMP  = 26;  ///< Camshaft Position Signal 1
constexpr int8_t SIG_CMP2 = 18;  ///< Camshaft Position Signal 2

// 5. Engine Signal Inputs
constexpr int8_t CAP_CKP  = 34;  ///< Crankshaft Capture Input
constexpr int8_t CAP_CMP  = 35;  ///< Camshaft Capture Input 1
constexpr int8_t CAP_CMP2 = 21;  ///< Camshaft Capture Input 2

// 6. Dedicated EPS / VSS Tester Outputs
constexpr int8_t EPS_VSS  = 4;   ///< Vehicle Speed Sensor (VSS) Pulse OUT
constexpr int8_t EPS_RPM  = 2;   ///< Engine Speed Tachometer Pulse OUT
constexpr int8_t EPS_TRQ1 = 15;  ///< Steering Torque Sensor 1 / PWM OUT
constexpr int8_t EPS_TRQ2 = 5;   ///< Steering Torque Sensor 2 / PWM OUT

#endif

} // namespace PinConfig
