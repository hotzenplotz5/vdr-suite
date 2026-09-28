#include "suitebridge_hbbtv_main_thread_input.h"

SuiteBridgeHbbtvMainThreadInput::SuiteBridgeHbbtvMainThreadInput(
    std::size_t capacity,
    std::chrono::milliseconds waitTimeout)
    : capacity_(capacity == 0 ? 1 : capacity),
      waitTimeout_(waitTimeout)
{
}

SuiteBridgeCommandResult SuiteBridgeHbbtvMainThreadInput::Rejected(
    const char *reason)
{
  return SuiteBridgeCommandResult{
      true,
      550,
      reason == nullptr ? "hbbtv_input_main_thread_unavailable" : reason};
}

SuiteBridgeCommandResult SuiteBridgeHbbtvMainThreadInput::Submit(
    const std::string &payload)
{
  auto request = std::make_shared<PendingRequest>(payload);

  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (stopping_)
      return Rejected("hbbtv_input_main_thread_stopping");
    if (queue_.size() >= capacity_)
      return Rejected("hbbtv_input_main_thread_queue_full");
    queue_.push_back(request);
  }

  std::unique_lock<std::mutex> lock(request->mutex);
  if (!request->completed.wait_for(
          lock,
          waitTimeout_,
          [&request]() { return request->done; })) {
    request->abandoned = true;
    return Rejected("hbbtv_input_main_thread_timeout");
  }

  return request->result;
}

void SuiteBridgeHbbtvMainThreadInput::Drain(const Handler &handler)
{
  std::deque<std::shared_ptr<PendingRequest>> pending;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    pending.swap(queue_);
  }

  for (const auto &request : pending) {
    {
      std::lock_guard<std::mutex> lock(request->mutex);
      if (request->abandoned) {
        request->done = true;
        request->completed.notify_all();
        continue;
      }
    }

    const SuiteBridgeCommandResult result =
        handler ? handler(request->payload)
                : Rejected("hbbtv_input_main_thread_handler_unavailable");

    {
      std::lock_guard<std::mutex> lock(request->mutex);
      request->result = result;
      request->done = true;
    }
    request->completed.notify_all();
  }
}

void SuiteBridgeHbbtvMainThreadInput::Stop()
{
  std::deque<std::shared_ptr<PendingRequest>> pending;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    stopping_ = true;
    pending.swap(queue_);
  }

  for (const auto &request : pending) {
    {
      std::lock_guard<std::mutex> lock(request->mutex);
      if (request->done)
        continue;
      request->result = Rejected("hbbtv_input_main_thread_stopping");
      request->done = true;
    }
    request->completed.notify_all();
  }
}

std::size_t SuiteBridgeHbbtvMainThreadInput::Pending() const
{
  std::lock_guard<std::mutex> lock(queueMutex_);
  return queue_.size();
}
