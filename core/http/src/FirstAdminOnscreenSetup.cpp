#include "FirstAdminOnscreenSetup.h"

#include "FirstAdminBootstrapIssuanceService.h"
#include "FirstAdminBootstrapRepository.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

namespace {
bool randomBytes(unsigned char* buffer, std::size_t length)
{
    std::size_t done = 0;
    while (done < length) {
        const ssize_t read = getrandom(buffer + done, length - done, 0);
        if (read > 0) { done += static_cast<std::size_t>(read); continue; }
        if (read < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

void wipe(std::string& value) noexcept
{
    volatile char* ptr = value.empty()
        ? nullptr : const_cast<volatile char*>(value.data());
    for (std::size_t i = 0; i < value.size(); ++i) ptr[i] = 0;
    value.clear();
}

bool readReply(int fd, int expected)
{
    // Bounded greeting/reply parser; no untrusted reply is logged.
    for (int line = 0; line < 10; ++line) {
        std::string response;
        for (int i = 0; i < 512; ++i) {
            char c = 0;
            const ssize_t count = recv(fd, &c, 1, 0);
            if (count != 1) return false;
            if (c == '\n') break;
            response.push_back(c);
        }
        if (response.size() < 4 ||
            response.substr(0, 3) != std::to_string(expected)) return false;
        if (response[3] == ' ') return true;
        if (response[3] != '-') return false;
    }
    return false;
}

bool writeAll(int fd, const std::string& data)
{
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t count = send(
            fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (count > 0) { sent += static_cast<std::size_t>(count); continue; }
        if (count < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

bool localVdrMessage(const std::string& pin)
{
    // SVDRP MESG is a standard local VDR operation. Never connect to
    // external addresses and never accept commands from HTTP payloads.
    unsigned long port = 6419;
    const char* configured = std::getenv("VDR_SUITE_FIRST_ADMIN_OSD_PORT");
    if (configured != nullptr && *configured != '\0') {
        char* end = nullptr;
        errno = 0;
        const unsigned long selected = std::strtoul(configured, &end, 10);
        if (errno != 0 || end == configured || *end != '\0' ||
            selected == 0 || selected > 65535) return false;
        port = selected;
    }
    const int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return false;
    timeval timeout{};
    timeout.tv_sec = 2;
    const bool configuredTimeout =
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0 &&
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bool ok = configuredTimeout &&
        connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0 &&
        readReply(fd, 220);
    if (ok) {
        ok = writeAll(fd, "MESG VDR-Suite Admin-Code: " + pin + "\r\n") &&
            readReply(fd, 250);
    }
    if (ok) (void)writeAll(fd, "QUIT\r\n");
    close(fd);
    return ok;
}
}

FirstAdminOnscreenSetup::FirstAdminOnscreenSetup(
    FirstAdminClaimService& claim,
    FirstAdminBootstrapRepository& repository,
    Presenter presenter, Entropy entropy, Clock clock)
    : claim_(claim), repository_(repository),
      presenter_(presenter ? std::move(presenter) : Presenter(localVdrMessage)),
      entropy_(entropy ? std::move(entropy) : Entropy(randomBytes)),
      clock_(clock ? std::move(clock) : Clock([] {
          return std::chrono::steady_clock::now();
      }))
{
}

FirstAdminOnscreenSetup::~FirstAdminOnscreenSetup()
{
    clearPin();
}

void FirstAdminOnscreenSetup::clearPin() noexcept
{
    wipe(pin_);
}

FirstAdminOnscreenResult FirstAdminOnscreenSetup::start()
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (claim_.claimState() != FirstAdminClaimState::unclaimed)
        return {FirstAdminOnscreenStatus::claimed};
    const auto now = clock_();
    if (started_ && now < nextStart_)
        return {FirstAdminOnscreenStatus::throttled};
    started_ = true;
    nextStart_ = now + std::chrono::seconds(60);
    clearPin();
    failures_ = 0;
    unsigned char bytes[8]{};
    if (!entropy_(bytes, sizeof(bytes)))
        return {FirstAdminOnscreenStatus::notReady};
    // Unbiased 8 decimal digits, approximately 26.6 bits with five tries.
    // Rejection sampling avoids modulo bias.
    std::string candidate;
    candidate.reserve(8);
    for (unsigned char value : bytes) {
        if (value >= 250) {
            unsigned char replacement = 0;
            do {
                if (!entropy_(&replacement, 1)) return {FirstAdminOnscreenStatus::notReady};
            } while (replacement >= 250);
            value = replacement;
        }
        candidate.push_back(static_cast<char>('0' + (value % 10)));
    }
    if (!presenter_(candidate)) {
        wipe(candidate);
        return {FirstAdminOnscreenStatus::noDisplay};
    }
    pin_ = std::move(candidate);
    expires_ = now + std::chrono::minutes(5);
    return {FirstAdminOnscreenStatus::ready};
}

FirstAdminOnscreenResult FirstAdminOnscreenSetup::complete(
    const std::string& code, FirstAdminClaimRequest request)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (claim_.claimState() != FirstAdminClaimState::unclaimed) {
        request.clearSecrets();
        return {FirstAdminOnscreenStatus::claimed};
    }
    const auto now = clock_();
    if (pin_.empty() || now >= expires_) {
        clearPin();
        request.clearSecrets();
        return {FirstAdminOnscreenStatus::expired};
    }
    unsigned int diff = static_cast<unsigned int>(code.size() ^ pin_.size());
    for (std::size_t i = 0; i < pin_.size(); ++i) {
        const unsigned char other = i < code.size()
            ? static_cast<unsigned char>(code[i]) : 0;
        diff |= other ^ static_cast<unsigned char>(pin_[i]);
    }
    if (diff != 0) {
        ++failures_;
        if (failures_ >= 5) clearPin();
        request.clearSecrets();
        return {FirstAdminOnscreenStatus::rejected};
    }
    clearPin(); // Consume before any account mutation, including failures.
    if (request.loginName.empty() ||
        request.displayName.empty() ||
        request.password.size() < 12) {
        request.clearSecrets();
        return {FirstAdminOnscreenStatus::invalid};
    }
    FirstAdminBootstrapIssuanceService issuer(repository_);
    FirstAdminBootstrapIssuanceResult issued = issuer.issue();
    if (issued.status != FirstAdminBootstrapIssuanceStatus::issued ||
        !issued.bootstrap.has_value()) {
        request.clearSecrets();
        return {FirstAdminOnscreenStatus::storageError};
    }
    request.bootstrapId = std::move(issued.bootstrap->bootstrapId);
    request.setupSecret = std::move(issued.bootstrap->setupSecret);
    issued.bootstrap->clearSecret();
    const FirstAdminClaimResult result = claim_.claim(std::move(request));
    return {result.status == FirstAdminClaimStatus::success
            ? FirstAdminOnscreenStatus::ready
            : FirstAdminOnscreenStatus::storageError, result.status};
}
