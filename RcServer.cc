#include "RcServer.h"
#include "RcTwinMotorDriver.h"
#include "RcLogger.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <wiringPi.h>

struct Message {
    static const uint8_t CMD_MOVE = 0x10;
    static const uint8_t CMD_UNKNOWN = 0x80;

    uint8_t cmd;
    uint32_t param0;
    uint32_t param1;
};

class MessageTranslator {
public:
    MessageTranslator() {
    }

    bool buildMessage(uint8_t* buffer, size_t size, Message* msg) {
        std::string packet = std::string((const char*)buffer, size);
        std::string::size_type old_rb_len = mReqeustBuffer.length();
        mReqeustBuffer += packet;

        // Find eof
        std::string::size_type newLineLen = 1;
        std::string::size_type newLinePos = packet.rfind("\n");

        // Check whether can translate
        if (newLinePos == std::string::npos) {
            return false;
        }

        newLinePos += old_rb_len;

        // Find the line top
        std::string::size_type lineStart = mReqeustBuffer.rfind("\n", newLinePos-1);
        if (lineStart == std::string::npos) {
            lineStart = 0;
        } else {
            lineStart += newLineLen;
        }

        std::string requestLine = mReqeustBuffer.substr(lineStart, newLinePos - lineStart + newLineLen);
        parseLIne(requestLine, msg);

        // Reset mReqeustBuffer
        std::string::size_type nextLinePos = newLinePos + newLineLen;
        mReqeustBuffer = mReqeustBuffer.substr(nextLinePos);
        return true;
    }

private:
    bool parseLIne(const std::string& line, Message* msg) {
        // Set Initial value
        msg->cmd = Message::CMD_UNKNOWN;

        // Split segments with space
        std::stringstream ss(line);
        std::vector<std::string> elems;
        std::string item;
        while (std::getline(ss, item, ' ')) {
            if (!item.empty()) {
                elems.push_back(item);
            }
        }

        // check whether elements exist
        if (elems.empty()) {
            return false;
        }

        const std::string& cmd = elems[0];
        if (!cmd.compare("MV")) {
            msg->cmd = Message::CMD_MOVE;
            msg->param0 = 0;
            msg->param1 = 0;

            if (elems.size() >= 2) {
                msg->param0 = atoi(elems[1].c_str());

                if (elems.size() >= 3) {
                    msg->param1 = atoi(elems[2].c_str());
                }
            }
        } else {
            LOGW("unknown cmd=%s", cmd.c_str());
        }

        return true;
    }

    std::string mReqeustBuffer;
};

class Timer {
public:
    Timer(float timeout_sec) : mTimeoutSec(timeout_sec) {}

    void start() {
        // zero clear
        memset(&mStartTime, 0, sizeof(mStartTime));

        // get current time
        if (gettimeofday(&mStartTime, NULL) < 0) {
            LOGE("[Timer] error in gettimeofday err=%d", errno);
            memset(&mStartTime, 0, sizeof(mStartTime));
        }
    }

    bool isTimeout() const {
        timeval t = {0};

        // get current time
        if (gettimeofday(&t, NULL) < 0) {
            LOGE("[Timer] error in gettimeofday err=%d", errno);
            return true;
        }

        // Check whether timeout occured
        float diff_time = t.tv_sec - mStartTime.tv_sec + (float)(t.tv_usec - mStartTime.tv_usec) / 1000000;
        return diff_time >= mTimeoutSec;
    }

private:
    timeval mStartTime;
    float mTimeoutSec;
};

class RcClient {
public:
    RcClient(const RcSettings& settings, int socket) : mSocket(socket),
            mTimer(settings.cmd_span), mDriver(17, 27, 23, 22, 1024){
    }

    ~RcClient() {
        if (mSocket >= 0) {
            close(mSocket);
        }
    }

    bool isConstructed() const {
        return true;
    }

    int run() {
        Message msg;
        MessageTranslator t;
        uint8_t buffer[1024];
        const size_t buf_size = sizeof(buffer);
        mTimer.start();

        // Loop as long as error occur
        while (true) {

            ssize_t bytes;
            if ((bytes = recvMessage(buffer, buf_size)) <= 0) {
                break; // Error
            }

            if (t.buildMessage(buffer, bytes, &msg) && msg.cmd != Message::CMD_UNKNOWN) {
                // Check whether timeout occur
                if (mTimer.isTimeout()) {

                    // Process command
                    doCommand(msg);

                    // Re-setup timer
                    mTimer.start();
                }
            }
        }

        PRINT("Finished connecting with client");

        return 0;
    }

private:

    int recvMessage(uint8_t* buffer, size_t size) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(mSocket, &readfds);

        // Check for ready reading
        if (select(mSocket+1, &readfds, NULL, NULL, NULL) <= 0) {
            LOGE("[RcClient] error in select() err=%d", errno);
            return -1;
        }

        ssize_t bytes;
        // Recv command
        if ((bytes = recv(mSocket, buffer, size, 0)) <= 0) {
            LOGE("[RcClient] error in recv() bytes=%d err=%d", bytes, errno);
            return -1;
        }

        return bytes;
    }

    void doCommand(const Message& msg) {
        LOGI("recv cmd: c=%d p0=%d p1=%d", msg.cmd, msg.param0, msg.param1);
        if (msg.cmd == Message::CMD_MOVE) {
            mDriver.setPower(RcTwinMotorDriver::MID0, msg.param0);
            mDriver.setPower(RcTwinMotorDriver::MID1, msg.param1);
            delay(50);
        }
    }

    int mSocket;
    Timer mTimer;
    RcTwinMotorDriver mDriver;
};

RcServer::RcServer(const RcSettings& settings) : mAcceptSocket(-1), mSettings(settings) {
    // Open the server socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("[RcServer] error in socket() err=%d", errno);
        return;
    }

    // initialize sockaddr_in object
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    // initialize socket address
    addr.sin_family          = AF_INET;     // TCP/IP
    addr.sin_addr.s_addr = htonl(settings.ip);   // IP address
    addr.sin_port              = htons(settings.port); // Port number

    // bind socket
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        LOGE("[RcServer] error in bind() err=%d", errno);
        return;
    }

    // listen socket
    if (listen(sock, settings.max_connection) < 0) {
        close(sock);
        LOGE("[RcServer] error in listen() err=%d", errno);
        return;
    }

    // Initialization successsed
    PRINT("RcServer initialized");
    mAcceptSocket = sock;
}

int RcServer::run() {
    PRINT("Run RcServer");

    int sock;
    while ((sock = wait()) >= 0) {

        RcClient client(mSettings, sock);
        if (client.isConstructed()) {
            client.run();
        }
   }

   return 0;
}

RcServer::~RcServer() {
    PRINT("Closing RcServer");
    if (mAcceptSocket >= 0) {
        close(mAcceptSocket);
    }
    PRINT("Closed RcServer");
}

bool RcServer::isConstructed() const {
    if (mAcceptSocket < 0) {
        return false;
    }
    return true;
}

int RcServer::wait() {
    int client_socket = -1;

    for (;;) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // accept socket
        PRINT("Accepting...");
        client_socket = accept(mAcceptSocket, (sockaddr*)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            int err = errno;
            if (err == EINTR) {
                PRINT("[RcServer] occured EINTR in accept()");
                continue; // Retry accept()
            }
            LOGE("[RcServer] error in accept() err=%d", err);
            return -1;
        } else {
            LOGI("Connected with %s", inet_ntoa(client_addr.sin_addr));
            break; // accept() successed
        }
    }

    return client_socket;
}

