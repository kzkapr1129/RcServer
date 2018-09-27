#include "RcTwinMotorDriver.h"
#include "RcLogger.h"
#include <algorithm>
#include <softPwm.h>
#include <stdio.h>
#include <wiringPi.h>

int RcTwinMotorDriver::init() {
    int ret;
    if ((ret = wiringPiSetupGpio()) < 0) {
        LOGE("[RcTwinMotorDriver] error in wiringPiSetupGpio: err=%d", ret);
        return -1;
    }

    return 0;
}

RcTwinMotorDriver::RcTwinMotorDriver(int m0_i0_pin, int m0_i1_pin,
        int m1_i0_pin, int m1_i1_pin, int maxpower) {

    // Setup fileds
    mPinPairs[0].pin0 = m0_i0_pin;
    mPinPairs[0].pin1 = m0_i1_pin;
    mPinPairs[1].pin0 = m1_i0_pin;
    mPinPairs[1].pin1 = m1_i1_pin;
    mMaxPower = std::abs(maxpower);

    // Setup the pin mode
    pinMode(mPinPairs[0].pin0, PWM_OUTPUT);
    pinMode(mPinPairs[0].pin1, PWM_OUTPUT);
    pinMode(mPinPairs[1].pin0, PWM_OUTPUT);
    pinMode(mPinPairs[1].pin1, PWM_OUTPUT);

    // Create the soft pwm
    softPwmCreate(mPinPairs[0].pin0, 0, mMaxPower);
    softPwmCreate(mPinPairs[0].pin1, 0, mMaxPower);
    softPwmCreate(mPinPairs[1].pin0, 0, mMaxPower);
    softPwmCreate(mPinPairs[1].pin1, 0, mMaxPower);
}

void RcTwinMotorDriver::setPower(MotorId motorId, int power) {
    if (power != 0) {
        int pin0_power = 0;
        int pin1_power = 0;
        bool isPlus = power > 0;

        if (isPlus) {
            pin0_power = std::max(0, std::min(mMaxPower, power));
        } else {
            pin1_power = std::max(0, std::min(mMaxPower, std::abs(power)));
        }

        softPwmWrite(mPinPairs[motorId].pin0, pin0_power);
        softPwmWrite(mPinPairs[motorId].pin1, pin1_power);

    } else {
        // Set idle mode
        softPwmWrite(mPinPairs[motorId].pin0, 0);
        softPwmWrite(mPinPairs[motorId].pin1, 0);
    }
}

