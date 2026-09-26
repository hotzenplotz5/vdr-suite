#include "../suitebridge_control_plane.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

using namespace vdrsuite::agent::control;

namespace
{

Response transact(
    const std::string& path,
    Operation operation,
    std::uint64_t requestId,
    std::uint64_t deadline,
    const std::string& payload = {})
{
  const int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  assert(fd >= 0);

  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::copy(path.begin(), path.end(), address.sun_path);
  address.sun_path[path.size()] = '\0';
  assert(connect(
      fd,
      reinterpret_cast<const sockaddr*>(&address),
      sizeof(address)) == 0);

  Request request;
  request.operation = operation;
  request.requestId = requestId;
  request.deadlineNanoseconds = deadline;
  request.payload = payload;

  std::vector<std::uint8_t> bytes;
  assert(encodeRequest(request, bytes));
  assert(send(fd, bytes.data(), bytes.size(), 0) ==
      static_cast<ssize_t>(bytes.size()));

  bytes.assign(MaximumResponseFrameBytes + 1, 0);
  const ssize_t received =
      recv(fd, bytes.data(), bytes.size(), MSG_TRUNC);
  assert(received > 0);
  bytes.resize(static_cast<std::size_t>(received));
  close(fd);

  Response response;
  std::string reason;
  assert(decodeResponse(bytes, response, reason));
  return response;
}

}

int main()
{
  const std::string path =
      "/tmp/vdr-suite-control-plane-" +
      std::to_string(getpid()) + ".sock";

  std::mutex mutex;
  std::condition_variable changed;

  bool firstCriticalStarted = false;
  bool releaseFirstCritical = false;
  int criticalExecutions = 0;

  bool interactiveStarted = false;
  bool releaseInteractive = false;

  bool externalPluginStarted = false;
  bool releaseExternalPlugin = false;

  bool backgroundProviderStarted = false;
  bool releaseBackgroundProvider = false;

  SuiteBridgeControlPlane server(path, 1, 1, 1, 1);
  assert(server.Start(
      [&](Operation operation, const std::string&) {
        SuiteBridgeCommandResult result{true, 250, "{}"};

        if (operation == Operation::LiveStatus) {
          std::unique_lock<std::mutex> lock(mutex);
          ++criticalExecutions;
          if (criticalExecutions == 1) {
            firstCriticalStarted = true;
            changed.notify_all();
            changed.wait(lock, [&] { return releaseFirstCritical; });
          }
        }

        if (operation == Operation::OsdSnapshot) {
          std::unique_lock<std::mutex> lock(mutex);
          interactiveStarted = true;
          changed.notify_all();
          changed.wait(lock, [&] { return releaseInteractive; });
          result.replyCode = 900;
        }

        if (operation == Operation::HbbtvPresentation) {
          std::unique_lock<std::mutex> lock(mutex);
          externalPluginStarted = true;
          changed.notify_all();
          changed.wait(lock, [&] { return releaseExternalPlugin; });
        }

        if (operation == Operation::RecordingMetadata) {
          std::unique_lock<std::mutex> lock(mutex);
          backgroundProviderStarted = true;
          changed.notify_all();
          changed.wait(lock, [&] { return releaseBackgroundProvider; });
        }

        return result;
      }));

  const auto expired = transact(
      path,
      Operation::LiveCapability,
      1,
      monotonicNowNanoseconds() - 1);
  assert(expired.result == Result::DeadlineExpired);

  const auto longDeadline =
      monotonicNowNanoseconds() + 2ULL * 1000ULL * 1000ULL * 1000ULL;

  auto firstCritical = std::async(std::launch::async, [&] {
    return transact(
        path,
        Operation::LiveStatus,
        2,
        longDeadline,
        "lease\nepoch");
  });

  {
    std::unique_lock<std::mutex> lock(mutex);
    assert(changed.wait_for(
        lock,
        std::chrono::seconds(1),
        [&] { return firstCriticalStarted; }));
  }

  const auto shortDeadline =
      monotonicNowNanoseconds() + 60ULL * 1000ULL * 1000ULL;

  auto queuedCritical = std::async(std::launch::async, [&] {
    return transact(
        path,
        Operation::LiveStatus,
        3,
        shortDeadline,
        "lease\nepoch");
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  const auto overloaded = transact(
      path,
      Operation::LiveStatus,
      4,
      longDeadline,
      "lease\nepoch");
  assert(overloaded.result == Result::Overloaded);

  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  {
    std::lock_guard<std::mutex> lock(mutex);
    releaseFirstCritical = true;
  }
  changed.notify_all();

  assert(firstCritical.get().result == Result::Success);
  assert(queuedCritical.get().result == Result::DeadlineExpired);
  assert(criticalExecutions == 1);

  auto blockedInteractive = std::async(std::launch::async, [&] {
    return transact(
        path,
        Operation::OsdSnapshot,
        5,
        longDeadline);
  });

  {
    std::unique_lock<std::mutex> lock(mutex);
    assert(changed.wait_for(
        lock,
        std::chrono::seconds(1),
        [&] { return interactiveStarted; }));
  }

  const auto criticalStarted = std::chrono::steady_clock::now();
  const auto criticalWhileInteractiveBlocked = transact(
      path,
      Operation::LiveCapability,
      6,
      longDeadline);
  const auto criticalElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - criticalStarted);

  assert(criticalWhileInteractiveBlocked.result == Result::Success);
  assert(criticalElapsed < std::chrono::milliseconds(150));

  {
    std::lock_guard<std::mutex> lock(mutex);
    releaseInteractive = true;
  }
  changed.notify_all();

  assert(blockedInteractive.get().result == Result::Success);

  auto blockedExternalPlugin = std::async(std::launch::async, [&] {
    return transact(
        path,
        Operation::HbbtvPresentation,
        7,
        longDeadline,
        "META 1 session-a");
  });

  {
    std::unique_lock<std::mutex> lock(mutex);
    assert(changed.wait_for(
        lock,
        std::chrono::seconds(1),
        [&] { return externalPluginStarted; }));
  }

  const auto osdStarted = std::chrono::steady_clock::now();
  const auto osdWhileHbbtvBlocked = transact(
      path,
      Operation::OsdSnapshot,
      8,
      longDeadline);
  const auto osdElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - osdStarted);

  assert(osdWhileHbbtvBlocked.result == Result::Success);
  assert(osdElapsed < std::chrono::milliseconds(150));

  const auto liveStarted = std::chrono::steady_clock::now();
  const auto liveWhileHbbtvBlocked = transact(
      path,
      Operation::LiveCapability,
      9,
      longDeadline);
  const auto liveElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - liveStarted);

  assert(liveWhileHbbtvBlocked.result == Result::Success);
  assert(liveElapsed < std::chrono::milliseconds(150));

  {
    std::lock_guard<std::mutex> lock(mutex);
    releaseExternalPlugin = true;
  }
  changed.notify_all();
  assert(blockedExternalPlugin.get().result == Result::Success);

  auto blockedBackgroundProvider = std::async(std::launch::async, [&] {
    return transact(
        path,
        Operation::RecordingMetadata,
        10,
        longDeadline,
        "recording-key");
  });

  {
    std::unique_lock<std::mutex> lock(mutex);
    assert(changed.wait_for(
        lock,
        std::chrono::seconds(1),
        [&] { return backgroundProviderStarted; }));
  }

  assert(serviceClass(Operation::EpgTypeSnapshot) ==
      ServiceClass::BackgroundProvider);
  auto queuedEpgTypeSnapshot = std::async(std::launch::async, [&] {
    return transact(
        path,
        Operation::EpgTypeSnapshot,
        13,
        longDeadline,
        "100 200 0 64");
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  assert(queuedEpgTypeSnapshot.wait_for(std::chrono::milliseconds(20)) ==
      std::future_status::timeout);

  const auto liveProviderStarted = std::chrono::steady_clock::now();
  const auto liveWhileProviderBlocked = transact(
      path,
      Operation::LiveCapability,
      11,
      longDeadline);
  const auto liveProviderElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - liveProviderStarted);
  assert(liveWhileProviderBlocked.result == Result::Success);
  assert(liveProviderElapsed < std::chrono::milliseconds(150));

  const auto osdProviderStarted = std::chrono::steady_clock::now();
  const auto osdWhileProviderBlocked = transact(
      path,
      Operation::OsdSnapshot,
      12,
      longDeadline);
  const auto osdProviderElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - osdProviderStarted);
  assert(osdWhileProviderBlocked.result == Result::Success);
  assert(osdProviderElapsed < std::chrono::milliseconds(150));

  {
    std::lock_guard<std::mutex> lock(mutex);
    releaseBackgroundProvider = true;
  }
  changed.notify_all();
  assert(blockedBackgroundProvider.get().result == Result::Success);
  assert(queuedEpgTypeSnapshot.get().result == Result::Success);

  const auto metrics = server.SnapshotMetrics();
  assert(metrics.overloaded >= 1);
  assert(metrics.deadlineExpiredBeforeExecution >= 2);
  assert(metrics.queueHighWaterByClass[0] == 1);
  assert(metrics.queueHighWaterByClass[1] == 1);
  assert(metrics.queueHighWaterByClass[2] == 1);
  assert(metrics.queueHighWaterByClass[3] == 1);
  assert(metrics.executedByOperation[2] == 1);
  assert(metrics.executedByOperation[8] == 3);
  assert(metrics.executedByOperation[12] == 1);
  assert(metrics.executedByOperation[18] == 1);
  assert(metrics.executedByOperation[19] == 1);

  server.Stop();
  assert(!server.Running());
  assert(access(path.c_str(), F_OK) != 0);

  std::puts(
      "SuiteBridge critical/interactive/external-plugin/background-provider lane tests passed");
  return 0;
}
