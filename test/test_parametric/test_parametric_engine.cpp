#include <unity.h>
#include "timing_math.h"
#include "parametric_pattern.h"
#include "signal_sniffer.h"
#include "rpm_controller.h"

void setUp(void) {}
void tearDown(void) {}

void test_timing_math_rev_period(void) {
    TEST_ASSERT_EQUAL_UINT32(100000, EcuEngine::TimingMath::calculateRevPeriodUs(600));
    TEST_ASSERT_EQUAL_UINT32(20000,  EcuEngine::TimingMath::calculateRevPeriodUs(3000));
    TEST_ASSERT_EQUAL_UINT32(10000,  EcuEngine::TimingMath::calculateRevPeriodUs(6000));
    TEST_ASSERT_EQUAL_UINT32(0,      EcuEngine::TimingMath::calculateRevPeriodUs(0));
}

void test_timing_math_cycle_period(void) {
    TEST_ASSERT_EQUAL_UINT32(200000, EcuEngine::TimingMath::calculateCyclePeriodUs(600));
    TEST_ASSERT_EQUAL_UINT32(40000,  EcuEngine::TimingMath::calculateCyclePeriodUs(3000));
    TEST_ASSERT_EQUAL_UINT32(20000,  EcuEngine::TimingMath::calculateCyclePeriodUs(6000));
}

void test_parametric_wheel_36_1(void) {
    EcuEngine::ParametricWheel wheel;
    wheel.totalTeeth = 36;
    wheel.missingTeeth = 1;
    wheel.missingPosition = 0;
    wheel.dutyCycle = 0.5f;

    TEST_ASSERT_TRUE(wheel.isValid());
    TEST_ASSERT_EQUAL_FLOAT(10.0f, wheel.getPitchAngleDeg());
    TEST_ASSERT_EQUAL_UINT16(35, wheel.getActiveTeethCount());
}

void test_parametric_wheel_60_2(void) {
    EcuEngine::ParametricWheel wheel;
    wheel.totalTeeth = 60;
    wheel.missingTeeth = 2;
    wheel.missingPosition = 0;
    wheel.dutyCycle = 0.5f;

    TEST_ASSERT_TRUE(wheel.isValid());
    TEST_ASSERT_EQUAL_FLOAT(6.0f, wheel.getPitchAngleDeg());
    TEST_ASSERT_EQUAL_UINT16(58, wheel.getActiveTeethCount());
}

void test_signal_sniffer_decode_36_1(void) {
    // Synthesize 36-1 @ 3000 RPM (20,000 us per revolution, tooth period = 555 us)
    EcuEngine::RawSignalEdge events[256];
    size_t count = 0;
    uint32_t t = 1000;
    uint32_t toothPeriod = 555;

    // 2 Revolutions (72 teeth total, tooth 0 and tooth 36 missing)
    for (int rev = 0; rev < 2; ++rev) {
        for (int tooth = 0; tooth < 36; ++tooth) {
            if (tooth != 0) { // tooth 0 is missing
                events[count++] = { t, 0, 1 };
                events[count++] = { t + (toothPeriod / 2), 0, 0 };
            }
            t += toothPeriod;
        }
    }

    EcuEngine::SignalSniffer sniffer;
    EcuEngine::SnifferResult res = sniffer.decode(events, count);

    TEST_ASSERT_TRUE(res.success);
    TEST_ASSERT_EQUAL_UINT16(36, res.wheel.totalTeeth);
    TEST_ASSERT_EQUAL_UINT8(1, res.wheel.missingTeeth);
    TEST_ASSERT_INT_WITHIN(50, 3000, res.detectedRpm);
}

void test_rpm_controller_crank_instant(void) {
    EcuEngine::EngineRuntimeState state;
    state.isRunning = true;
    state.runMode = EcuEngine::EngineRunMode::CrankToFix;
    state.targetRpm = 850;
    state.cranking.crankingRpm = 200;
    state.cranking.crankDurationMs = 2000;
    state.cranking.spinUpDurationMs = 400;
    state.cranking.fastFlare = true; // Melesat (instant 0 ms jump to fix)

    EcuEngine::RpmController controller;
    controller.startCranking(state.cranking);

    // Initial
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::SpinUp, (uint8_t)controller.getCrankingStage());
    
    // Halfway spinup (200 ms) -> RPM ~ 100
    controller.update(state, 200);
    TEST_ASSERT_INT_WITHIN(10, 100, state.currentRpm);

    // Complete spinup (another 200 ms -> total 400 ms) -> enters Cranking stage @ 200 RPM
    controller.update(state, 200);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::Cranking, (uint8_t)controller.getCrankingStage());
    TEST_ASSERT_EQUAL_UINT32(200, state.currentRpm);

    // Cranking hold (advance 1900 ms) -> still Cranking @ 200 RPM
    controller.update(state, 1900);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::Cranking, (uint8_t)controller.getCrankingStage());
    TEST_ASSERT_EQUAL_UINT32(200, state.currentRpm);

    // Complete crank duration (+200 ms -> reaches 2100 ms total hold)
    // In Melesat mode, CrankToFix instantly jumps to target RPM (850) and enters PostCrank
    controller.update(state, 200);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::PostCrank, (uint8_t)controller.getCrankingStage());
    TEST_ASSERT_EQUAL_UINT32(850, state.currentRpm);
}

void test_rpm_controller_crank_gradual(void) {
    EcuEngine::EngineRuntimeState state;
    state.isRunning = true;
    state.runMode = EcuEngine::EngineRunMode::CrankToFix;
    state.targetRpm = 1000;
    state.cranking.crankingRpm = 200;
    state.cranking.crankDurationMs = 1000;
    state.cranking.spinUpDurationMs = 200;
    state.cranking.rampDurationMs = 2000; // 2 seconds gradual ramp
    state.cranking.fastFlare = false;    // Gradual

    EcuEngine::RpmController controller;
    controller.startCranking(state.cranking);

    // Spinup 200 ms
    controller.update(state, 200);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::Cranking, (uint8_t)controller.getCrankingStage());
    TEST_ASSERT_EQUAL_UINT32(200, state.currentRpm);

    // Crank hold 1000 ms -> transition to Ramping
    controller.update(state, 1000);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::Ramping, (uint8_t)controller.getCrankingStage());

    // Halfway ramp (1000 ms of 2000 ms) -> RPM should be 200 + (1000 - 200) * 0.5 = 600 RPM
    controller.update(state, 1000);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::Ramping, (uint8_t)controller.getCrankingStage());
    TEST_ASSERT_INT_WITHIN(15, 600, state.currentRpm);

    // Finish ramp (+1000 ms) -> reaches target 1000 RPM and PostCrank
    controller.update(state, 1000);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)EcuEngine::CrankingStage::PostCrank, (uint8_t)controller.getCrankingStage());
    TEST_ASSERT_EQUAL_UINT32(1000, state.currentRpm);
}

static void runParametricTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_timing_math_rev_period);
    RUN_TEST(test_timing_math_cycle_period);
    RUN_TEST(test_parametric_wheel_36_1);
    RUN_TEST(test_parametric_wheel_60_2);
    RUN_TEST(test_signal_sniffer_decode_36_1);
    RUN_TEST(test_rpm_controller_crank_instant);
    RUN_TEST(test_rpm_controller_crank_gradual);
    UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
    delay(2000);
    runParametricTests();
}
void loop() {}
#else
int main(int argc, char **argv) {
    runParametricTests();
    return 0;
}
#endif
