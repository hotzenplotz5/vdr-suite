#include "DaemonMediaCheckRuntime.h"
#include "MediaCheckApiRuntime.h"
#include "Database.h"

#include <array>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
namespace {

constexpr const char* kBackend = "local";
constexpr const char* kPublicId = "rec_0123456789abcdef0123456789abcdef";

using Packet = std::array<unsigned char, 188>;

Packet packet(unsigned counter, bool broken)
{
    Packet bytes{};
    bytes.fill(0xff);
    bytes[0] = broken ? 0x00 : 0x47;
    bytes[1] = 0x01;
    bytes[2] = 0x00;
    bytes[3] = static_cast<unsigned char>(0x10 | counter);
    return bytes;
}

struct Fixture
{
    fs::path home;
    fs::path video;
    fs::path recording;
    std::set<std::string> initialFiles;
    std::string firstBytes;
    std::string secondBytes;

    Fixture()
    {
        std::string pattern = (fs::temp_directory_path() /
            "vdr-suite-media-check-api-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        const char* folder = ::mkdtemp(buffer.data());
        assert(folder != nullptr);
        home = folder;
        video = home / "video";
        recording = video / "Sample" / "2026-10-22.11.00.1-0.rec";
        fs::create_directories(recording);
        write("00001.ts", packet(0, false));
        write("00002.ts", packet(1, true));
        std::ofstream(recording / "index", std::ios::binary).put('i');
        firstBytes = read("00001.ts");
        secondBytes = read("00002.ts");
        for (const auto& entry : fs::directory_iterator(recording))
            initialFiles.insert(entry.path().filename().string());
    }

    ~Fixture()
    {
        std::error_code error;
        fs::remove_all(home, error);
    }

    void write(const std::string& name, const Packet& bytes)
    {
        std::ofstream out(recording / name, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        assert(out.good());
    }

    std::string read(const std::string& name) const
    {
        std::ifstream in(recording / name, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in),
                           std::istreambuf_iterator<char>());
    }

    bool unchanged() const
    {
        if (read("00001.ts") != firstBytes ||
            read("00002.ts") != secondBytes) return false;
        std::set<std::string> current;
        for (const auto& entry : fs::directory_iterator(recording))
            current.insert(entry.path().filename().string());
        return current == initialFiles;
    }
};

ApiResponse callPost(const std::string& actor, const std::string& authorized,
                     const std::string& body)
{
    ApiResponse response;
    const bool handled = MediaCheckApiRuntime::instance().tryHandlePost(
        "/api/v1/backends/local/media-checks",
        body, actor, authorized, "application/json", response);
    assert(handled);
    return response;
}

ApiResponse callGet(const std::string& id, const std::string& actor,
                    const std::string& allowed)
{
    ApiResponse response;
    const bool handled = MediaCheckApiRuntime::instance().tryHandleGet(
        "/api/v1/backends/local/media-checks/" + id, actor,
        allowed, {}, response);
    assert(handled);
    return response;
}

std::string checkId(const std::string& body)
{
    const auto begin = body.find("chk_");
    assert(begin != std::string::npos);
    assert(begin + 36 <= body.size());
    return body.substr(begin, 36);
}

} // namespace

int main()
{
    Fixture fixture;
    Database database;
    assert(database.open((fixture.home / "suite.sqlite").string()));
    DaemonMediaCheckRuntime runtime(
        {{kBackend, fixture.video.string()}},
        [&fixture](const std::string& backend, const std::string& id)
            -> std::optional<fs::path> {
            if (backend == kBackend && id == kPublicId)
                return fixture.recording;
            return std::nullopt;
        },
        [](const std::string& backend) {
            return backend == kBackend;
        }, database);
    assert(runtime.ensurePersistence());

    MediaCheckApiRuntime::instance().registerHandlers(
        [&runtime](const std::string& actor, const std::string& backend,
                   const std::string& id, const std::string& mode) {
            return runtime.start(actor, backend, id, mode);
        },
        [&runtime](const std::string& actor, const std::string& backend,
                   const std::string& id) {
            return runtime.status(actor, backend, id);
        },
        [&runtime](const std::string& backend) {
            return runtime.capability(backend);
        });

    auto& api = MediaCheckApiRuntime::instance();
    ApiResponse response;
    assert(api.tryHandleGet(
        "/api/v1/backends/local/capabilities/media-check",
        "actor-a", "local", {}, response));
    assert(response.statusCode == 200 &&
           response.body.find("\"available\":true") != std::string::npos);
    assert(api.tryHandleGet(
        "/api/v1/backends/remote/capabilities/media-check",
        "actor-a", "*", {}, response));
    assert(response.statusCode == 200 &&
           response.body.find("\"available\":false") != std::string::npos);

    const std::string body = std::string("{\"recordingId\":\"") +
        kPublicId + "\",\"mode\":\"structure\"}";
    assert(callPost("", "local", body).statusCode == 401);
    assert(callPost("actor-a", "other", body).statusCode == 403);
    assert(callPost("actor-a", "local",
        "{\"recordingId\":\"../../recording.rec\",\"mode\":\"structure\"}"
        ).statusCode == 422);
    assert(callPost("actor-a", "local",
        "{\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\","
        "\"mode\":\"repair\"}").statusCode == 422);
    assert(callPost("actor-a", "local",
        "{\"recordingId\":\"rec_ffffffffffffffffffffffffffffffff\","
        "\"mode\":\"structure\"}").statusCode == 404);

    const ApiResponse accepted = callPost("actor-a", "local", body);
    assert(accepted.statusCode == 202);
    assert(accepted.body.find("\"state\":\"running\"") != std::string::npos);
    const std::string id = checkId(accepted.body);
    assert(id.size() == 36);
    assert(callGet(id, "", "local").statusCode == 401);
    assert(callGet(id, "actor-b", "local").statusCode == 404);
    assert(callGet(id, "actor-a", "other").statusCode == 404);

    ApiResponse result;
    bool finished = false;
    for (int attempt = 0; attempt < 500; ++attempt) {
        result = callGet(id, "actor-a", "local");
        assert(result.statusCode == 200);
        if (result.body.find("\"state\":\"completed\"") !=
                std::string::npos) {
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(finished);
    assert(result.body.find("\"status\":\"errors\"") != std::string::npos);
    assert(result.body.find("\"name\":\"00002.ts\",\"status\":\"errors\"")
        != std::string::npos);
    assert(result.body.find("\"syncErrors\":1") != std::string::npos);
    assert(result.body.find(kPublicId) != std::string::npos);
    assert(result.body.find(fixture.video.string()) == std::string::npos);
    assert(result.body.find("\"readOnly\":true") != std::string::npos);
    assert(fixture.unchanged());

    // A second runtime sees the completed result through Suite's durable
    // SQLite read model rather than the first runtime's in-memory job.
    DaemonMediaCheckRuntime restarted(
        {{kBackend, fixture.video.string()}},
        [&fixture](const std::string& backend, const std::string& id)
            -> std::optional<fs::path> {
            if (backend == kBackend && id == kPublicId)
                return fixture.recording;
            return std::nullopt;
        },
        [](const std::string& backend) {
            return backend == kBackend;
        }, database);
    assert(restarted.ensurePersistence());
    assert(restarted.status("actor-a", "local", id).body == result.body);
    assert(restarted.status("actor-b", "local", id).statusCode == 404);
    assert(restarted.status("actor-a", "other", id).statusCode == 404);
    assert(fixture.unchanged());

    api.registerHandlers({}, {}, {});
    std::cout << "MEDIA_CHECK_API_V1_CPP_EXECUTION=PASS\n";
    return 0;
}
