#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class RestfulApiEventStreamClient
{
public:
    RestfulApiEventStreamClient(
        std::string backendId,
        std::string host,
        int port,
        std::function<void(const std::string& backendId)> onChangeHint);

    ~RestfulApiEventStreamClient();

    void start();
    void stop();
    bool running() const;

private:
    void runLoop();
    bool connectAndReadOnce();
    bool connectSocket(int& socketFd);
    bool sendRequest(int socketFd);
    bool readLoop(int socketFd);
    void handleSseBlock(const std::string& block);
    bool blockIsChangeEvent(const std::string& block) const;

    std::string backendId_;
    std::string host_;
    int port_;
    std::function<void(const std::string& backendId)> onChangeHint_;

    std::atomic<bool> running_;
    std::thread thread_;
};
