#include "RcServer.h"
#include "RcTwinMotorDriver.h"
#include <signal.h>

int main() {
    if (RcTwinMotorDriver::init()) {
        return -1;
    }

    // Setup signal
    signal(SIGPIPE, SIG_IGN);

    // Setup RcServer
    RcSettings settings;
    RcServer srv(settings);
    if (!srv.isConstructed()) {
        return -2;
    }

    // Run RcServer
    return srv.run();
}

