#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, content: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    if not content.endswith("\n"):
        content += "\n"
    target.write_text(content, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{path}: expected exactly one occurrence, found {count}: {old!r}"
        )
    write(path, text.replace(old, new, 1))


write(
    "core/agent/include/SuiteBridgeEmbeddedAgentRuntime.h",
    r'''#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeObservation.h"
#include "SuiteBridgeObservationWorker.h"
#include "SuiteBridgeSvdrpTransport.h"

#include <memory>
#include <string>

namespace vdrsuite::agent
{

struct SuiteBridgeEmbeddedAgentConfig
{
    std::string backendId = "default";
    bool enabled = false;
    SuiteBridgeSvdrpTransportConfig transport;
    SuiteBridgeObservationConfig observation;
};

struct SuiteBridgeEmbeddedAgentHealth
{
    std::string backendId;
    bool configured = false;
    bool running = false;
    SuiteBridgeObservationSnapshot observation;
};

class SuiteBridgeEmbeddedAgentRuntime
{
public:
    explicit SuiteBridgeEmbeddedAgentRuntime(
        SuiteBridgeEmbeddedAgentConfig config = {});

    SuiteBridgeEmbeddedAgentRuntime(
        SuiteBridgeEmbeddedAgentConfig config,
        std::unique_ptr<ISuiteBridgeLocalTransport> transport);

    ~SuiteBridgeEmbeddedAgentRuntime();

    void start();
    void stop();

    bool running() const;
    SuiteBridgeEmbeddedAgentHealth health() const;

private:
    static std::string boundedBackendId(
        const std::string& backendId);

    void initializeWorker();

    SuiteBridgeEmbeddedAgentConfig config_;
    std::unique_ptr<ISuiteBridgeLocalTransport> transport_;
    std::unique_ptr<SuiteBridgeObservationWorker> worker_;
};

}
''',
)

write(
    "core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp",
    r'''#include "SuiteBridgeEmbeddedAgentRuntime.h"

#include <algorithm>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

constexpr std::size_t MaximumBackendIdLength = 128;

}

SuiteBridgeEmbeddedAgentRuntime::SuiteBridgeEmbeddedAgentRuntime(
    SuiteBridgeEmbeddedAgentConfig config)
    : config_(std::move(config))
{
    config_.backendId = boundedBackendId(config_.backendId);

    if (config_.enabled)
    {
        transport_ = std::make_unique<SuiteBridgeSvdrpTransport>(
            config_.transport);
    }

    initializeWorker();
}

SuiteBridgeEmbeddedAgentRuntime::SuiteBridgeEmbeddedAgentRuntime(
    SuiteBridgeEmbeddedAgentConfig config,
    std::unique_ptr<ISuiteBridgeLocalTransport> transport)
    : config_(std::move(config)),
      transport_(std::move(transport))
{
    config_.backendId = boundedBackendId(config_.backendId);
    initializeWorker();
}

SuiteBridgeEmbeddedAgentRuntime::~SuiteBridgeEmbeddedAgentRuntime()
{
    stop();
}

void SuiteBridgeEmbeddedAgentRuntime::start()
{
    if (worker_)
    {
        worker_->start();
    }
}

void SuiteBridgeEmbeddedAgentRuntime::stop()
{
    if (worker_)
    {
        worker_->stop();
    }
}

bool SuiteBridgeEmbeddedAgentRuntime::running() const
{
    return worker_ && worker_->running();
}

SuiteBridgeEmbeddedAgentHealth
SuiteBridgeEmbeddedAgentRuntime::health() const
{
    SuiteBridgeEmbeddedAgentHealth value;
    value.backendId = config_.backendId;
    value.configured = config_.enabled;
    value.running = running();

    if (worker_)
    {
        value.observation = worker_->snapshot();
    }
    else
    {
        value.observation.state =
            SuiteBridgeObservationState::NotConfigured;
        value.observation.diagnostic = config_.enabled
            ? "Suite Bridge transport unavailable"
            : "Suite Bridge disabled";
        value.observation.started = false;
        value.observation.mutationsEnabled = false;
    }

    return value;
}

std::string SuiteBridgeEmbeddedAgentRuntime::boundedBackendId(
    const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId.substr(
        0,
        std::min(backendId.size(), MaximumBackendIdLength));
}

void SuiteBridgeEmbeddedAgentRuntime::initializeWorker()
{
    if (!config_.enabled || !transport_)
    {
        return;
    }

    worker_ = std::make_unique<SuiteBridgeObservationWorker>(
        *transport_,
        config_.observation);
}

}
''',
)

write(
    "core/agent/tests/test_suite_bridge_embedded_agent_runtime.cpp",
    r'''#include "SuiteBridgeEmbeddedAgentRuntime.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace vdrsuite::agent;
using namespace std::chrono_literals;

namespace
{

class ThreadSafeFakeTransport final : public ISuiteBridgeLocalTransport
{
public:
    explicit ThreadSafeFakeTransport(
        std::vector<SuiteBridgeCommandReply> replies)
        : replies_(std::move(replies))
    {
    }

    SuiteBridgeCommandReply execute(
        const SuiteBridgeLocalCommand command) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        commands_.push_back(command);
        changed_.notify_all();

        if (nextReply_ >= replies_.size())
        {
            SuiteBridgeCommandReply value;
            value.transportStatus = SuiteBridgeTransportStatus::Failed;
            value.diagnostic = "fixture exhausted";
            return value;
        }

        return replies_[nextReply_++];
    }

    bool waitForCalls(
        const std::size_t count,
        const std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return changed_.wait_for(
            lock,
            timeout,
            [this, count]() {
                return commands_.size() >= count;
            });
    }

    std::vector<SuiteBridgeLocalCommand> commands() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return commands_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<SuiteBridgeCommandReply> replies_;
    std::vector<SuiteBridgeLocalCommand> commands_;
    std::size_t nextReply_ = 0;
};

SuiteBridgeCommandReply reply(
    const int code,
    std::string payload)
{
    SuiteBridgeCommandReply value;
    value.transportStatus = SuiteBridgeTransportStatus::Success;
    value.replyCode = code;
    value.payload = std::move(payload);
    return value;
}

std::string discovery()
{
    return
        "{\"discovery_schema\":1,"
        "\"plugin_name\":\"suitebridge\","
        "\"plugin_version\":\"0.10.0\","
        "\"capability_schema\":1,"
        "\"snapshot_schema\":2,"
        "\"local_contract_schema\":2,"
        "\"capabilities\":["
        "{\"id\":\"snapshots\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"},"
        "{\"id\":\"mutations\",\"state\":\"disabled\"}]}";
}

std::string snapshot()
{
    return
        "{\"contract_schema\":2,"
        "\"capability_schema\":1,"
        "\"snapshot_schema\":2,"
        "\"active\":true,"
        "\"total\":4,"
        "\"channel_switch\":4,"
        "\"recording\":0,"
        "\"replaying\":0,"
        "\"timer_change\":0,"
        "\"counter_epoch\":\"11111111111111111111111111111111\","
        "\"counter_overflow\":false}";
}

SuiteBridgeEmbeddedAgentConfig fastConfig()
{
    SuiteBridgeEmbeddedAgentConfig value;
    value.backendId = "default";
    value.enabled = true;
    value.observation.pollInterval = 20ms;
    value.observation.staleAfter = 60ms;
    value.observation.offlineAfter = 200ms;
    value.observation.reconnectInitial = 20ms;
    value.observation.reconnectMaximum = 40ms;
    return value;
}

void testDisabledRuntimeStartsNoWorker()
{
    SuiteBridgeEmbeddedAgentConfig config;
    config.backendId = "default";
    config.enabled = false;

    SuiteBridgeEmbeddedAgentRuntime runtime(config);
    runtime.start();

    const SuiteBridgeEmbeddedAgentHealth health = runtime.health();
    assert(health.backendId == "default");
    assert(!health.configured);
    assert(!health.running);
    assert(health.observation.state ==
           SuiteBridgeObservationState::NotConfigured);
    assert(!health.observation.mutationsEnabled);

    runtime.stop();
}

void testInjectedTransportPublishesBackendScopedHealth()
{
    auto transport = std::make_unique<ThreadSafeFakeTransport>(
        std::vector<SuiteBridgeCommandReply>{
            reply(900, discovery()),
            reply(900, snapshot())
        });
    ThreadSafeFakeTransport* transportView = transport.get();

    SuiteBridgeEmbeddedAgentRuntime runtime(
        fastConfig(),
        std::move(transport));

    runtime.start();
    runtime.start();

    assert(transportView->waitForCalls(2, 1s));

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        if (runtime.health().observation.state ==
            SuiteBridgeObservationState::SnapshotCurrent)
        {
            break;
        }

        std::this_thread::sleep_for(5ms);
    }

    const SuiteBridgeEmbeddedAgentHealth current = runtime.health();
    assert(current.backendId == "default");
    assert(current.configured);
    assert(current.running);
    assert(current.observation.state ==
           SuiteBridgeObservationState::SnapshotCurrent);
    assert(current.observation.hasBaseline);
    assert(current.observation.baseline.total == 4);
    assert(!current.observation.mutationsEnabled);

    const auto commands = transportView->commands();
    assert(commands.size() >= 2);
    assert(commands[0] == SuiteBridgeLocalCommand::DiscoverSchema1);
    assert(commands[1] == SuiteBridgeLocalCommand::Snapshot);

    runtime.stop();
    runtime.stop();

    const SuiteBridgeEmbeddedAgentHealth stopped = runtime.health();
    assert(!stopped.running);
    assert(stopped.observation.state ==
           SuiteBridgeObservationState::Offline);
}

void testBackendIdentityIsBounded()
{
    SuiteBridgeEmbeddedAgentConfig config;
    config.backendId = std::string(256, 'a');
    config.enabled = false;

    SuiteBridgeEmbeddedAgentRuntime runtime(config);
    assert(runtime.health().backendId.size() == 128);
}

}

int main()
{
    testDisabledRuntimeStartsNoWorker();
    testInjectedTransportPublishesBackendScopedHealth();
    testBackendIdentityIsBounded();

    std::cout
        << "test_suite_bridge_embedded_agent_runtime passed"
        << std::endl;
    return 0;
}
''',
)

write(
    "core/daemon/include/RuntimeConfig.h",
    r'''#pragma once

#include <map>
#include <string>

struct RuntimeSuiteBridgeConfig
{
    bool enabled = false;
    std::string backendId = "default";
    std::string host = "127.0.0.1";
    int port = 6419;
    int connectTimeoutMs = 1000;
    int ioTimeoutMs = 1000;
    int operationTimeoutMs = 3000;
    int pollIntervalMs = 5000;
    int staleAfterMs = 15000;
    int offlineAfterMs = 60000;
    int reconnectInitialMs = 1000;
    int reconnectMaximumMs = 30000;
};

class RuntimeConfig
{
public:
    RuntimeConfig();

    const std::string& databasePath() const;
    const std::string& vdrMode() const;
    const std::string& vdrHost() const;
    int vdrPort() const;
    const std::string& httpListenHost() const;
    int httpListenPort() const;
    const std::map<std::string, std::string>& recordingArtworkRoots() const;
    const RuntimeSuiteBridgeConfig& suiteBridge() const;

private:
    std::string databasePath_;
    std::string vdrMode_;
    std::string vdrHost_;
    int vdrPort_;
    std::string httpListenHost_;
    int httpListenPort_;
    std::map<std::string, std::string> recordingArtworkRoots_;
    RuntimeSuiteBridgeConfig suiteBridge_;
};
''',
)

write(
    "core/daemon/src/RuntimeConfig.cpp",
    r'''#include "RuntimeConfig.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>

namespace
{

std::string environmentOrDefault(
    const char* name,
    const std::string& fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr)
    {
        return fallback;
    }

    std::string text(value);

    if (text.empty())
    {
        return fallback;
    }

    return text;
}

std::string environmentOrEmpty(
    const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr
        ? std::string()
        : std::string(value);
}

std::string lowercase(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool environmentBoolean(
    const char* name,
    const bool fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || *value == '\0')
    {
        return fallback;
    }

    const std::string normalized = lowercase(value);

    if (normalized == "1" ||
        normalized == "true" ||
        normalized == "yes" ||
        normalized == "on")
    {
        return true;
    }

    if (normalized == "0" ||
        normalized == "false" ||
        normalized == "no" ||
        normalized == "off")
    {
        return false;
    }

    return fallback;
}

int environmentInteger(
    const char* name,
    const int fallback,
    const int minimum,
    const int maximum)
{
    const char* value = std::getenv(name);

    if (value == nullptr || *value == '\0')
    {
        return fallback;
    }

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);

    if (errno != 0 ||
        end == value ||
        end == nullptr ||
        *end != '\0' ||
        parsed < minimum ||
        parsed > maximum)
    {
        return fallback;
    }

    return static_cast<int>(parsed);
}

bool isValidBackendId(
    const std::string& backendId)
{
    if (backendId.empty() || backendId.size() > 128)
    {
        return false;
    }

    for (const unsigned char character : backendId)
    {
        if (!std::isalnum(character) &&
            character != '-' &&
            character != '_' &&
            character != '.')
        {
            return false;
        }
    }

    return true;
}

std::map<std::string, std::string> parseArtworkRoots(
    const std::string& value)
{
    std::map<std::string, std::string> roots;

    if (value.empty())
    {
        return roots;
    }

    std::size_t start = 0;

    while (start <= value.size())
    {
        const std::size_t end = value.find(';', start);
        const std::string entry = value.substr(
            start,
            end == std::string::npos
                ? std::string::npos
                : end - start);
        const std::size_t separator = entry.find('=');

        if (entry.empty() ||
            separator == std::string::npos ||
            separator == 0 ||
            separator + 1 >= entry.size())
        {
            return {};
        }

        const std::string backendId =
            entry.substr(0, separator);
        const std::string root =
            entry.substr(separator + 1);

        if (!isValidBackendId(backendId) ||
            !std::filesystem::path(root).is_absolute() ||
            !roots.emplace(backendId, root).second)
        {
            return {};
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return roots;
}

RuntimeSuiteBridgeConfig parseSuiteBridgeConfig()
{
    RuntimeSuiteBridgeConfig value;

    value.enabled = environmentBoolean(
        "VDR_SUITE_SUITE_BRIDGE_ENABLED",
        value.enabled);

    const std::string backendId = environmentOrDefault(
        "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
        value.backendId);
    value.backendId = isValidBackendId(backendId)
        ? backendId
        : "default";

    const std::string host = environmentOrDefault(
        "VDR_SUITE_SUITE_BRIDGE_HOST",
        value.host);
    value.host = host.size() <= 255
        ? host
        : "127.0.0.1";

    value.port = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_PORT",
        value.port,
        1,
        65535);
    value.connectTimeoutMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS",
        value.connectTimeoutMs,
        1,
        300000);
    value.ioTimeoutMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS",
        value.ioTimeoutMs,
        1,
        300000);
    value.operationTimeoutMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS",
        value.operationTimeoutMs,
        1,
        900000);
    value.pollIntervalMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS",
        value.pollIntervalMs,
        1,
        3600000);
    value.staleAfterMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS",
        value.staleAfterMs,
        1,
        3600000);
    value.offlineAfterMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS",
        value.offlineAfterMs,
        1,
        3600000);
    value.reconnectInitialMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS",
        value.reconnectInitialMs,
        1,
        3600000);
    value.reconnectMaximumMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS",
        value.reconnectMaximumMs,
        1,
        3600000);

    value.staleAfterMs = std::max(
        value.staleAfterMs,
        value.pollIntervalMs);
    value.offlineAfterMs = std::max(
        value.offlineAfterMs,
        value.staleAfterMs);
    value.reconnectMaximumMs = std::max(
        value.reconnectMaximumMs,
        value.reconnectInitialMs);

    return value;
}

}

RuntimeConfig::RuntimeConfig()
    : databasePath_(environmentOrDefault(
          "VDR_SUITE_DATABASE_PATH",
          "/tmp/vdr-suite-test.db")),
      vdrMode_("restfulapi"),
      vdrHost_("127.0.0.1"),
      vdrPort_(8002),
      httpListenHost_("0.0.0.0"),
      httpListenPort_(18080),
      recordingArtworkRoots_(parseArtworkRoots(
          environmentOrEmpty(
              "VDR_SUITE_RECORDING_ARTWORK_ROOTS"))),
      suiteBridge_(parseSuiteBridgeConfig())
{
}

const std::string& RuntimeConfig::databasePath() const
{
    return databasePath_;
}

const std::string& RuntimeConfig::vdrMode() const
{
    return vdrMode_;
}

const std::string& RuntimeConfig::vdrHost() const
{
    return vdrHost_;
}

int RuntimeConfig::vdrPort() const
{
    return vdrPort_;
}

const std::string& RuntimeConfig::httpListenHost() const
{
    return httpListenHost_;
}

int RuntimeConfig::httpListenPort() const
{
    return httpListenPort_;
}

const std::map<std::string, std::string>& RuntimeConfig::recordingArtworkRoots() const
{
    return recordingArtworkRoots_;
}

const RuntimeSuiteBridgeConfig& RuntimeConfig::suiteBridge() const
{
    return suiteBridge_;
}
''',
)

write(
    "core/daemon/tests/test_runtime_config.cpp",
    r'''#include "RuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{

const std::vector<const char*> SuiteBridgeVariables = {
    "VDR_SUITE_SUITE_BRIDGE_ENABLED",
    "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
    "VDR_SUITE_SUITE_BRIDGE_HOST",
    "VDR_SUITE_SUITE_BRIDGE_PORT",
    "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS",
    "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS",
    "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS",
    "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS",
    "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS",
    "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS",
    "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS",
    "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS"
};

void clearSuiteBridgeEnvironment()
{
    for (const char* name : SuiteBridgeVariables)
    {
        unsetenv(name);
    }
}

}

int main()
{
    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");
    clearSuiteBridgeEnvironment();

    RuntimeConfig defaultConfig;
    assert(defaultConfig.databasePath() == "/tmp/vdr-suite-test.db");
    assert(defaultConfig.recordingArtworkRoots().empty());
    assert(!defaultConfig.suiteBridge().enabled);
    assert(defaultConfig.suiteBridge().backendId == "default");
    assert(defaultConfig.suiteBridge().host == "127.0.0.1");
    assert(defaultConfig.suiteBridge().port == 6419);
    assert(defaultConfig.suiteBridge().connectTimeoutMs == 1000);
    assert(defaultConfig.suiteBridge().ioTimeoutMs == 1000);
    assert(defaultConfig.suiteBridge().operationTimeoutMs == 3000);
    assert(defaultConfig.suiteBridge().pollIntervalMs == 5000);
    assert(defaultConfig.suiteBridge().staleAfterMs == 15000);
    assert(defaultConfig.suiteBridge().offlineAfterMs == 60000);
    assert(defaultConfig.suiteBridge().reconnectInitialMs == 1000);
    assert(defaultConfig.suiteBridge().reconnectMaximumMs == 30000);

    setenv(
        "VDR_SUITE_DATABASE_PATH",
        "/var/lib/vdr-suite/vdr-suite.db",
        1);
    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=/srv/tvscraper;wohnhaus2=/mnt/secondary artwork",
        1);
    setenv("VDR_SUITE_SUITE_BRIDGE_ENABLED", "YES", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_BACKEND_ID", "wohnhaus2", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_HOST", "127.0.0.2", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_PORT", "6420", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS", "1100", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS", "1200", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS", "3300", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS", "6000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS", "18000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS", "72000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS", "1500", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS", "45000", 1);

    RuntimeConfig overriddenConfig;
    assert(overriddenConfig.databasePath() ==
           "/var/lib/vdr-suite/vdr-suite.db");
    assert(overriddenConfig.recordingArtworkRoots().size() == 2);
    assert(overriddenConfig.recordingArtworkRoots().at("default") ==
           "/srv/tvscraper");
    assert(overriddenConfig.recordingArtworkRoots().at("wohnhaus2") ==
           "/mnt/secondary artwork");
    assert(overriddenConfig.suiteBridge().enabled);
    assert(overriddenConfig.suiteBridge().backendId == "wohnhaus2");
    assert(overriddenConfig.suiteBridge().host == "127.0.0.2");
    assert(overriddenConfig.suiteBridge().port == 6420);
    assert(overriddenConfig.suiteBridge().connectTimeoutMs == 1100);
    assert(overriddenConfig.suiteBridge().ioTimeoutMs == 1200);
    assert(overriddenConfig.suiteBridge().operationTimeoutMs == 3300);
    assert(overriddenConfig.suiteBridge().pollIntervalMs == 6000);
    assert(overriddenConfig.suiteBridge().staleAfterMs == 18000);
    assert(overriddenConfig.suiteBridge().offlineAfterMs == 72000);
    assert(overriddenConfig.suiteBridge().reconnectInitialMs == 1500);
    assert(overriddenConfig.suiteBridge().reconnectMaximumMs == 45000);

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=/srv/one;default=/srv/two",
        1);
    RuntimeConfig duplicateConfig;
    assert(duplicateConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=relative/path",
        1);
    RuntimeConfig relativeConfig;
    assert(relativeConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "invalid backend=/srv/tvscraper",
        1);
    RuntimeConfig invalidBackendArtworkConfig;
    assert(invalidBackendArtworkConfig.recordingArtworkRoots().empty());

    setenv("VDR_SUITE_SUITE_BRIDGE_ENABLED", "invalid", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_BACKEND_ID", "invalid backend", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_HOST", std::string(300, 'x').c_str(), 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_PORT", "70000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS", "-1", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS", "not-a-number", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS", "0", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS", "20000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS", "1000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS", "1000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS", "40000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS", "1000", 1);

    RuntimeConfig invalidSuiteBridgeConfig;
    assert(!invalidSuiteBridgeConfig.suiteBridge().enabled);
    assert(invalidSuiteBridgeConfig.suiteBridge().backendId == "default");
    assert(invalidSuiteBridgeConfig.suiteBridge().host == "127.0.0.1");
    assert(invalidSuiteBridgeConfig.suiteBridge().port == 6419);
    assert(invalidSuiteBridgeConfig.suiteBridge().connectTimeoutMs == 1000);
    assert(invalidSuiteBridgeConfig.suiteBridge().ioTimeoutMs == 1000);
    assert(invalidSuiteBridgeConfig.suiteBridge().operationTimeoutMs == 3000);
    assert(invalidSuiteBridgeConfig.suiteBridge().pollIntervalMs == 20000);
    assert(invalidSuiteBridgeConfig.suiteBridge().staleAfterMs == 20000);
    assert(invalidSuiteBridgeConfig.suiteBridge().offlineAfterMs == 20000);
    assert(invalidSuiteBridgeConfig.suiteBridge().reconnectInitialMs == 40000);
    assert(invalidSuiteBridgeConfig.suiteBridge().reconnectMaximumMs == 40000);

    setenv("VDR_SUITE_DATABASE_PATH", "", 1);
    setenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS", "", 1);
    clearSuiteBridgeEnvironment();

    RuntimeConfig emptyOverrideConfig;
    assert(emptyOverrideConfig.databasePath() ==
           "/tmp/vdr-suite-test.db");
    assert(emptyOverrideConfig.recordingArtworkRoots().empty());
    assert(!emptyOverrideConfig.suiteBridge().enabled);

    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");
    clearSuiteBridgeEnvironment();

    return 0;
}
''',
)

replace_once(
    "core/daemon/include/BackendRuntimeContext.h",
    '#include "SearchTimerPreviewEpgCacheRefreshService.h"\n#include "VdrService.h"\n',
    '#include "SearchTimerPreviewEpgCacheRefreshService.h"\n#include "SuiteBridgeEmbeddedAgentRuntime.h"\n#include "VdrService.h"\n',
)
replace_once(
    "core/daemon/include/BackendRuntimeContext.h",
    '    std::unique_ptr<RestfulApiEventStreamClient> eventStreamClient;\n',
    '    std::unique_ptr<RestfulApiEventStreamClient> eventStreamClient;\n    std::unique_ptr<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime;\n',
)

replace_once(
    "core/daemon/src/DaemonRuntime.cpp",
    '#include <thread>\n#include <vector>\n',
    '#include <thread>\n#include <utility>\n#include <vector>\n',
)

replace_once(
    "core/daemon/src/DaemonRuntime.cpp",
    '''    context->eventStreamClient = std::make_unique<RestfulApiEventStreamClient>(
        context->backendId,
        backendConfig.host,
        backendConfig.port + 1,
        [this](const std::string&) {
            externalVdrChangeHint_.store(true);
            epgCacheDirtyHint_.store(true);
            recordingCacheDirtyHint_.store(true);
        });

    return context;
''',
    '''    context->eventStreamClient = std::make_unique<RestfulApiEventStreamClient>(
        context->backendId,
        backendConfig.host,
        backendConfig.port + 1,
        [this](const std::string&) {
            externalVdrChangeHint_.store(true);
            epgCacheDirtyHint_.store(true);
            recordingCacheDirtyHint_.store(true);
        });

    const RuntimeSuiteBridgeConfig& suiteBridgeConfig =
        config_.suiteBridge();

    if (suiteBridgeConfig.enabled &&
        context->backendId == suiteBridgeConfig.backendId) {
        vdrsuite::agent::SuiteBridgeEmbeddedAgentConfig embeddedConfig;
        embeddedConfig.backendId = context->backendId;
        embeddedConfig.enabled = true;
        embeddedConfig.transport.host = suiteBridgeConfig.host;
        embeddedConfig.transport.port = suiteBridgeConfig.port;
        embeddedConfig.transport.connectTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.connectTimeoutMs);
        embeddedConfig.transport.ioTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.ioTimeoutMs);
        embeddedConfig.transport.operationTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.operationTimeoutMs);
        embeddedConfig.observation.pollInterval =
            std::chrono::milliseconds(suiteBridgeConfig.pollIntervalMs);
        embeddedConfig.observation.staleAfter =
            std::chrono::milliseconds(suiteBridgeConfig.staleAfterMs);
        embeddedConfig.observation.offlineAfter =
            std::chrono::milliseconds(suiteBridgeConfig.offlineAfterMs);
        embeddedConfig.observation.reconnectInitial =
            std::chrono::milliseconds(suiteBridgeConfig.reconnectInitialMs);
        embeddedConfig.observation.reconnectMaximum =
            std::chrono::milliseconds(suiteBridgeConfig.reconnectMaximumMs);

        context->suiteBridgeAgentRuntime =
            std::make_unique<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime>(
                std::move(embeddedConfig));
    }

    return context;
''',
)

replace_once(
    "core/daemon/src/DaemonRuntime.cpp",
    '''        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->start();
        }

        backendRuntimeContexts_.push_back(
            std::move(backendRuntimeContext));
''',
    '''        backendRuntimeContexts_.push_back(
            std::move(backendRuntimeContext));
''',
)

replace_once(
    "core/daemon/src/DaemonRuntime.cpp",
    '''    startEpgCacheWarmupWorker();
    startRecordingCacheWarmupWorker();
''',
    '''    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (!backendRuntimeContext) {
            continue;
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            backendRuntimeContext->suiteBridgeAgentRuntime->start();
            std::cout
                << "Suite Bridge embedded Agent runtime started: backend="
                << backendRuntimeContext->backendId
                << std::endl;
        }

        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->start();
        }
    }

    startEpgCacheWarmupWorker();
    startRecordingCacheWarmupWorker();
''',
)

replace_once(
    "core/daemon/src/DaemonRuntime.cpp",
    '''    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->stop();
        }
    }
''',
    '''    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (!backendRuntimeContext) {
            continue;
        }

        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->stop();
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            backendRuntimeContext->suiteBridgeAgentRuntime->stop();
        }
    }
''',
)

write(
    "mk/agent-sources.mk",
    r'''AGENT_HANDSHAKE_SRC := \
	core/agent/src/SuiteBridgeHandshake.cpp \
	core/agent/src/SuiteBridgeLocalContractParser.cpp \
	core/agent/src/SuiteBridgeHandshakeService.cpp

AGENT_SVDRP_TRANSPORT_SRC := \
	core/agent/src/SuiteBridgeSvdrpTransport.cpp

AGENT_OBSERVATION_SRC := \
	core/agent/src/SuiteBridgeObservation.cpp \
	core/agent/src/SuiteBridgeObservationService.cpp \
	core/agent/src/SuiteBridgeObservationWorker.cpp

AGENT_EMBEDDED_RUNTIME_SRC := \
	core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp

AGENT_SRC := \
	$(AGENT_HANDSHAKE_SRC) \
	$(AGENT_SVDRP_TRANSPORT_SRC) \
	$(AGENT_OBSERVATION_SRC) \
	$(AGENT_EMBEDDED_RUNTIME_SRC)
''',
)

replace_once(
    "mk/common.mk",
    '        -Icore/daemon/include \\\n        -Icore/vdr/include \\\n',
    '        -Icore/daemon/include \\\n        -Icore/agent/include \\\n        -Icore/vdr/include \\\n',
)

write(
    "mk/agent-tests.mk",
    r'''.PHONY: test-suite-bridge-agent-boundary test-suite-bridge-handshake test-suite-bridge-handshake-missing-plugin test-suite-bridge-svdrp-transport-boundary test-suite-bridge-svdrp-transport test-suite-bridge-svdrp-transport-live test-suite-bridge-observation-boundary test-suite-bridge-observation-service test-suite-bridge-observation-worker test-suite-bridge-embedded-runtime-boundary test-suite-bridge-embedded-runtime test-suite-bridge-daemon-runtime-wiring test-real-suite-bridge-observation-live

test-suite-bridge-agent-boundary:
	python3 tools/check_suite_bridge_agent_boundary.py

test-suite-bridge-handshake: test-suite-bridge-handshake-missing-plugin
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/tests/test_suite_bridge_handshake.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake
	$(BUILD_DIR)/test_suite_bridge_handshake

test-suite-bridge-handshake-missing-plugin:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/tests/test_suite_bridge_handshake_missing_plugin.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake_missing_plugin
	$(BUILD_DIR)/test_suite_bridge_handshake_missing_plugin

test-suite-bridge-svdrp-transport-boundary:
	python3 tools/check_suite_bridge_svdrp_transport_boundary.py

test-suite-bridge-svdrp-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SVDRP_TRANSPORT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport

test-suite-bridge-observation-boundary:
	python3 tools/check_suite_bridge_observation_boundary.py

test-suite-bridge-observation-service:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/src/SuiteBridgeObservation.cpp \
		core/agent/src/SuiteBridgeObservationService.cpp \
		core/agent/tests/test_suite_bridge_observation_service.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_service
	$(BUILD_DIR)/test_suite_bridge_observation_service

test-suite-bridge-observation-worker:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		$(AGENT_OBSERVATION_SRC) \
		core/agent/tests/test_suite_bridge_observation_worker.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_worker
	$(BUILD_DIR)/test_suite_bridge_observation_worker

test-suite-bridge-embedded-runtime-boundary:
	python3 tools/check_suite_bridge_embedded_runtime_boundary.py

test-suite-bridge-embedded-runtime:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_embedded_agent_runtime.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime
	$(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime

test-suite-bridge-daemon-runtime-wiring:
	python3 tools/check_suite_bridge_daemon_runtime_wiring.py

test-suite-bridge-svdrp-transport-live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport_live
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport_live

test-real-suite-bridge-observation-live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_observation_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_live
	$(BUILD_DIR)/test_suite_bridge_observation_live
''',
)

replace_once(
    "mk/runtime-api-tests.mk",
    '''daemon:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(DAEMON_SRC) \
		apps/daemon/main.cpp \
''',
    '''daemon:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(AGENT_SRC) \
		$(DAEMON_SRC) \
		apps/daemon/main.cpp \
''',
)
replace_once(
    "mk/runtime-api-tests.mk",
    '''test-backend-runtime-context:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
''',
    '''test-backend-runtime-context:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(AGENT_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
''',
)

replace_once(
    "mk/test-groups.mk",
    '''	test-suite-bridge-observation-service \
	test-suite-bridge-observation-worker \
	test-fast \
''',
    '''	test-suite-bridge-observation-service \
	test-suite-bridge-observation-worker \
	test-suite-bridge-embedded-runtime-boundary \
	test-suite-bridge-embedded-runtime \
	test-suite-bridge-daemon-runtime-wiring \
	test-fast \
''',
)
replace_once(
    "mk/test-groups.mk",
    '''	test-suite-bridge-observation-service \
	test-suite-bridge-observation-worker \
	test-backend-node \
''',
    '''	test-suite-bridge-observation-service \
	test-suite-bridge-observation-worker \
	test-suite-bridge-embedded-runtime-boundary \
	test-suite-bridge-embedded-runtime \
	test-suite-bridge-daemon-runtime-wiring \
	test-backend-node \
''',
)

write(
    "tools/check_suite_bridge_embedded_runtime_boundary.py",
    r'''#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/agent/include/SuiteBridgeEmbeddedAgentRuntime.h"
SOURCE = ROOT / "core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp"
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
AGENT_SOURCES = ROOT / "mk/agent-sources.mk"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    context = CONTEXT.read_text(encoding="utf-8")
    agent_sources = AGENT_SOURCES.read_text(encoding="utf-8")
    combined = header + source

    require(
        "SuiteBridgeEmbeddedAgentConfig" in header,
        "embedded Agent runtime must expose typed configuration",
    )
    require(
        "SuiteBridgeEmbeddedAgentHealth" in header,
        "embedded Agent runtime must expose bounded health",
    )
    require(
        "std::make_unique<SuiteBridgeSvdrpTransport>" in source,
        "production runtime must own the accepted typed SVDRP transport",
    )
    require(
        "std::make_unique<SuiteBridgeObservationWorker>" in source,
        "runtime must compose the accepted SB.10c worker",
    )
    require(
        "std::unique_ptr<ISuiteBridgeLocalTransport>" in header,
        "testability must remain behind the typed local transport boundary",
    )
    require(
        "SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime" in context,
        "BackendRuntimeContext must own the embedded Agent runtime",
    )
    require(
        "core/agent/src/SuiteBridgeEmbeddedAgentRuntime.cpp" in agent_sources,
        "embedded runtime source must be owned by AGENT_SRC",
    )

    forbidden = (
        "ApiRouter",
        "Database",
        "sqlite3",
        "RestfulApiVdrAdapter",
        "PLUG suitebridge",
        "system(",
        "popen(",
        "fork(",
        "mutationsEnabled = true",
    )
    for token in forbidden:
        require(
            token not in combined,
            f"embedded Agent runtime must not contain forbidden coupling: {token}",
        )

    print("check_suite_bridge_embedded_runtime_boundary passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
''',
)

write(
    "tools/check_suite_bridge_daemon_runtime_wiring.py",
    r'''#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONFIG_HEADER = ROOT / "core/daemon/include/RuntimeConfig.h"
CONFIG_SOURCE = ROOT / "core/daemon/src/RuntimeConfig.cpp"
CONTEXT = ROOT / "core/daemon/include/BackendRuntimeContext.h"
RUNTIME = ROOT / "core/daemon/src/DaemonRuntime.cpp"
RUNTIME_TESTS = ROOT / "mk/runtime-api-tests.mk"
ARCHITECTURE = ROOT / "docs/architecture/suite-bridge-embedded-agent-runtime.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    config_header = CONFIG_HEADER.read_text(encoding="utf-8")
    config_source = CONFIG_SOURCE.read_text(encoding="utf-8")
    context = CONTEXT.read_text(encoding="utf-8")
    runtime = RUNTIME.read_text(encoding="utf-8")
    runtime_tests = RUNTIME_TESTS.read_text(encoding="utf-8")

    require(ARCHITECTURE.exists(), "SB.10d architecture contract must exist")
    require(
        "RuntimeSuiteBridgeConfig" in config_header,
        "RuntimeConfig must expose Suite Bridge configuration",
    )

    required_environment = (
        "VDR_SUITE_SUITE_BRIDGE_ENABLED",
        "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
        "VDR_SUITE_SUITE_BRIDGE_HOST",
        "VDR_SUITE_SUITE_BRIDGE_PORT",
        "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS",
        "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS",
        "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS",
        "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS",
        "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS",
        "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS",
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS",
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS",
    )
    for name in required_environment:
        require(name in config_source, f"missing runtime configuration: {name}")

    require(
        "SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime" in context,
        "backend context must own backend-scoped Suite Bridge health runtime",
    )
    require(
        "context->backendId == suiteBridgeConfig.backendId" in runtime,
        "Suite Bridge runtime must attach only to the configured backend",
    )
    require(
        "std::make_unique<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime>" in runtime,
        "DaemonRuntime must construct the embedded Agent runtime",
    )
    require(
        "suiteBridgeAgentRuntime->start()" in runtime,
        "DaemonRuntime must start the embedded Agent runtime",
    )
    require(
        "suiteBridgeAgentRuntime->stop()" in runtime,
        "DaemonRuntime must stop the embedded Agent runtime",
    )

    start_index = runtime.index("suiteBridgeAgentRuntime->start()")
    event_start_index = runtime.index("eventStreamClient->start()", start_index)
    require(
        start_index < event_start_index,
        "Suite Bridge worker must start before the RESTfulAPI event-stream client",
    )

    event_stop_index = runtime.index("eventStreamClient->stop()")
    suite_stop_index = runtime.index("suiteBridgeAgentRuntime->stop()", event_stop_index)
    require(
        event_stop_index < suite_stop_index,
        "shutdown must reverse active local reader start order",
    )

    require(
        "$(AGENT_SRC)" in runtime_tests,
        "daemon and backend-context targets must link the embedded Agent sources",
    )
    require(
        "RestfulApiVdrAdapter" in runtime,
        "SB.10d must preserve the existing RESTfulAPI domain adapter",
    )
    require(
        "ApiRouter(" not in config_source,
        "Suite Bridge configuration must not create a public route",
    )

    print("check_suite_bridge_daemon_runtime_wiring passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
''',
)

replace_once(
    "docs/architecture/index.md",
    '- [Suite Bridge Read-Only Observation Lifecycle](suite-bridge-observation-lifecycle.md)\n',
    '- [Suite Bridge Read-Only Observation Lifecycle](suite-bridge-observation-lifecycle.md)\n- [Suite Bridge Embedded Agent Runtime](suite-bridge-embedded-agent-runtime.md)\n',
)

replace_once(
    "vdr-plugin-suite-bridge/docs/ROADMAP.md",
    '- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n\n---\n',
    '- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n- [SB.10d Embedded Agent Runtime](../../docs/architecture/suite-bridge-embedded-agent-runtime.md)\n\n---\n',
)
replace_once(
    "vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md",
    '- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n- [VDR-Suite Documentation Index]',
    '- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\n- [SB.10d Embedded Agent Runtime](../../docs/architecture/suite-bridge-embedded-agent-runtime.md)\n- [VDR-Suite Documentation Index]',
)

print("SB.10d embedded Agent runtime implementation prepared")
