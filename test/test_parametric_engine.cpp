#include <unity.h>
#include "timing_math.h"
#include "parametric_pattern.h"
#include "signal_sniffer.h"

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
    EcuEngine::RawSignalEdge events[128];
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

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_timing_math_rev_period);
    RUN_TEST(test_timing_math_cycle_period);
    RUN_TEST(test_parametric_wheel_36_1);
    RUN_TEST(test_parametric_wheel_60_2);
    RUN_TEST(test_signal_sniffer_decode_36_1);
    return UNITY_END();
}
