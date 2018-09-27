#pragma once

#include <stdint.h>

struct RcSettings {
    // Default values
    static const uint32_t DEFAULT_IP = 0;
    static const uint16_t DEFAULT_PORT = 9024;
    static const uint8_t DEFAULT_MAX_CONNECTION = 5;
    static const float DEFAULT_CMD_SPAN = 0.5f;

    uint32_t ip;
    uint16_t port;
    uint8_t max_connection;
    float cmd_span;

    RcSettings()  : ip (DEFAULT_IP), port (DEFAULT_PORT),
            max_connection(DEFAULT_MAX_CONNECTION),
            cmd_span(DEFAULT_CMD_SPAN) {}
};

class RcServer {
public:
    RcServer(const RcSettings& settings);
    ~RcServer();
    bool isConstructed() const;

    int run();

private:
    int wait();

    int mAcceptSocket;
    const RcSettings& mSettings;
};
