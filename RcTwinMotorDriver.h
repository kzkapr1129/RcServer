#pragma once

class RcTwinMotorDriver {
public:
    enum MotorId {
        MID0 = 0,
        MID1
    };

    static int init();

    RcTwinMotorDriver(int m0_i0_pin, int m0_i1_pin,
            int m1_i0_pin, int m1_i1_pin,
            int maxpower);

    void setPower(MotorId motorId, int power);

private:
    struct PinPair {
        int pin0;
        int pin1;
    };

    PinPair mPinPairs[2];
    int mMaxPower;
};

