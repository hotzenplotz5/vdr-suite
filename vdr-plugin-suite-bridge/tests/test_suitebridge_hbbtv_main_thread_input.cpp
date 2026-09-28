#include "../suitebridge_hbbtv_main_thread_input.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <string>
#include <thread>

namespace {

bool waitForPending(
    SuiteBridgeHbbtvMainThreadInput &dispatcher,
    std::size_t expected,
    std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (dispatcher.Pending() == expected)
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return dispatcher.Pending() == expected;
}

} // namespace

int main()
{
  using namespace std::chrono_literals;

  const std::thread::id mainThread = std::this_thread::get_id();
  SuiteBridgeHbbtvMainThreadInput dispatcher(4, 500ms);

  auto input = std::async(std::launch::async, [&]() {
    return dispatcher.Submit(
        "INPUT 1 session-a C-1-1051-10301 7 42 OK");
  });

  assert(waitForPending(dispatcher, 1, 100ms));

  bool handledOnMainThread = false;
  dispatcher.Drain([&](const std::string &payload) {
    handledOnMainThread = std::this_thread::get_id() == mainThread;
    assert(payload == "INPUT 1 session-a C-1-1051-10301 7 42 OK");
    return SuiteBridgeCommandResult{
        true,
        250,
        "{\"schemaVersion\":1,\"result\":\"ok\"}"};
  });

  const SuiteBridgeCommandResult inputResult = input.get();
  assert(handledOnMainThread);
  assert(inputResult.handled);
  assert(inputResult.replyCode == 250);
  assert(inputResult.payload.find("\"result\":\"ok\"") != std::string::npos);

  SuiteBridgeHbbtvMainThreadInput timeoutDispatcher(1, 20ms);
  const SuiteBridgeCommandResult timedOut = timeoutDispatcher.Submit(
      "INPUT 1 session-b C-1-1051-10301 7 42 LEFT");
  assert(timedOut.handled);
  assert(timedOut.replyCode == 550);
  assert(timedOut.payload == "hbbtv_input_main_thread_timeout");

  bool staleExecuted = false;
  timeoutDispatcher.Drain([&](const std::string &) {
    staleExecuted = true;
    return SuiteBridgeCommandResult{true, 250, "{}"};
  });
  assert(!staleExecuted);

  SuiteBridgeHbbtvMainThreadInput boundedDispatcher(1, 500ms);
  auto first = std::async(std::launch::async, [&]() {
    return boundedDispatcher.Submit(
        "INPUT 1 session-c C-1-1051-10301 7 42 RIGHT");
  });
  assert(waitForPending(boundedDispatcher, 1, 100ms));
  const SuiteBridgeCommandResult overloaded = boundedDispatcher.Submit(
      "INPUT 1 session-c C-1-1051-10301 7 42 DOWN");
  assert(overloaded.handled);
  assert(overloaded.replyCode == 550);
  assert(overloaded.payload == "hbbtv_input_main_thread_queue_full");
  boundedDispatcher.Drain([](const std::string &) {
    return SuiteBridgeCommandResult{true, 250, "{}"};
  });
  assert(first.get().replyCode == 250);

  SuiteBridgeHbbtvMainThreadInput stopDispatcher(2, 500ms);
  auto stopping = std::async(std::launch::async, [&]() {
    return stopDispatcher.Submit(
        "INPUT 1 session-d C-1-1051-10301 7 42 PAUSE");
  });
  assert(waitForPending(stopDispatcher, 1, 100ms));
  stopDispatcher.Stop();
  const SuiteBridgeCommandResult stopped = stopping.get();
  assert(stopped.handled);
  assert(stopped.replyCode == 550);
  assert(stopped.payload == "hbbtv_input_main_thread_stopping");

  std::puts(
      "SuiteBridge HbbTV input main-thread handoff, timeout, backpressure and stop tests passed");
  return 0;
}
