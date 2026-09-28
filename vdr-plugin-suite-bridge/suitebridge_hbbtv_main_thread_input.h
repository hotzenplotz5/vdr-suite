#ifndef VDR_SUITE_BRIDGE_HBBTV_MAIN_THREAD_INPUT_H
#define VDR_SUITE_BRIDGE_HBBTV_MAIN_THREAD_INPUT_H

#include "suitebridge_command_result.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

class SuiteBridgeHbbtvMainThreadInput final {
public:
  using Handler = std::function<SuiteBridgeCommandResult(const std::string &)>;

  explicit SuiteBridgeHbbtvMainThreadInput(
      std::size_t capacity = 16,
      std::chrono::milliseconds waitTimeout = std::chrono::milliseconds(1000));

  SuiteBridgeCommandResult Submit(const std::string &payload);
  void Drain(const Handler &handler);
  void Stop();
  std::size_t Pending() const;

private:
  struct PendingRequest final {
    explicit PendingRequest(std::string requestPayload)
        : payload(std::move(requestPayload))
    {
    }

    std::string payload;
    mutable std::mutex mutex;
    std::condition_variable completed;
    bool done = false;
    bool abandoned = false;
    SuiteBridgeCommandResult result;
  };

  static SuiteBridgeCommandResult Rejected(const char *reason);

  const std::size_t capacity_;
  const std::chrono::milliseconds waitTimeout_;
  mutable std::mutex queueMutex_;
  std::deque<std::shared_ptr<PendingRequest>> queue_;
  bool stopping_ = false;
};

#endif
