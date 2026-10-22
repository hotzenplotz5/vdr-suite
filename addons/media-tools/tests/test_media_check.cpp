#include "MediaCheck.h"
#include "MediaCheckFfmpeg.h"

#include <array>
#include <algorithm>
#include <cerrno>
#include <cassert>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <thread>
#include <sys/wait.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using vdrsuite::mediatools::checkRecordingTransportStream;

namespace {

using Packet = std::array<unsigned char, 188>;

Packet packet(unsigned cc, unsigned pid = 0x100)
{
    Packet p{};
    p.fill(0xff);
    p[0] = 0x47;
    p[1] = static_cast<unsigned char>((pid >> 8) & 0x1f);
    p[2] = static_cast<unsigned char>(pid & 0xff);
    p[3] = static_cast<unsigned char>(0x10 | (cc & 15));
    return p;
}

struct Fixture {
    fs::path home, video, rec;
    Fixture()
    {
        std::string name = (fs::temp_directory_path() /
                            "vdr-suite-media-check-XXXXXX").string();
        std::vector<char> data(name.begin(), name.end());
        data.push_back('\0');
        const char* made = ::mkdtemp(data.data());
        if (!made) throw std::runtime_error("cannot make test fixture");
        home = made;
        video = home / "video";
        rec = video / "Test" / "2026-10-22.07.00.1-0.rec";
        fs::create_directories(rec);
        std::ofstream(rec / "index", std::ios::binary).put('i');
        write("00001.ts", {packet(0), packet(1)});
        write("00002.ts", {packet(2), packet(3)});
    }
    ~Fixture()
    {
        std::error_code error;
        fs::remove_all(home, error);
    }
    void write(const char* name, const std::vector<Packet>& packets,
               const std::string& tail = "")
    {
        std::ofstream out(rec / name, std::ios::binary | std::ios::trunc);
        for (const auto& p : packets)
            out.write(reinterpret_cast<const char*>(p.data()), p.size());
        out.write(tail.data(), static_cast<std::streamsize>(tail.size()));
        assert(out.good());
    }
    fs::path relative() const
    {
        return fs::path("Test") / rec.filename();
    }
    std::string bytes(const char* name) const
    {
        std::ifstream in(rec / name, std::ios::binary);
        return {std::istreambuf_iterator<char>(in),
                std::istreambuf_iterator<char>()};
    }
    std::set<std::string> entries() const
    {
        std::set<std::string> result;
        for (const auto& item : fs::directory_iterator(rec))
            result.insert(item.path().filename().string());
        return result;
    }
};

template<class F>
void throws(F&& callback)
{
    bool thrown = false;
    try { callback(); }
    catch (const std::exception&) { thrown = true; }
    assert(thrown);
}

// Deterministic executable fixture: launched by FfmpegMediaCheckDecoder
// through fork/exec, with the exact FFmpeg argument contract and stdin data.
// No external binary installation and no file writes in the test.
int fakeFfmpeg(int argc, char* argv[])
{
    bool inputPipe = false, nullOutput = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "-i" && i + 1 < argc && std::string(argv[i+1]) == "pipe:0")
            inputPipe = true;
        if (arg == "-f" && i + 1 < argc && std::string(argv[i+1]) == "null")
            nullOutput = true;
        if (arg.find(".rec") != std::string::npos ||
            arg.find(".ts") != std::string::npos)
            return 55; // FFmpeg must never receive recording paths.
    }
    if (!inputPipe || !nullOutput) return 56;

    std::string bytes;
    std::array<char, 1024> chunk{};
    while (true) {
        const ssize_t n = ::read(STDIN_FILENO, chunk.data(), chunk.size());
        if (n == 0) break;
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return 57;
        bytes.append(chunk.data(), static_cast<std::size_t>(n));
        if (bytes.size() > 4096) return 58;
    }
    if (bytes.find("M2_FORCE_STALL") != std::string::npos) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 0;
    }
    if (bytes.find("M2_FORCE_FAIL") != std::string::npos) {
        constexpr char message[] = "synthetic FFmpeg decoder error\n";
        (void)::write(STDERR_FILENO, message, sizeof(message) - 1);
        return 7;
    }
    if (bytes.size() != 4 * 188) return 59;
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc > 1 && std::string(argv[1]) == "-v") {
        // The test binary acts as the exact FFprobe child, receiving only
        // an inherited descriptor on stdin, never a real recording path.
        std::string input;
        char bytes[1024];
        while (true) {
            const ssize_t got = ::read(STDIN_FILENO, bytes, sizeof bytes);
            if (got == 0) break;
            if (got < 0 && errno == EINTR) continue;
            if (got < 0) return 80;
            input.append(bytes, static_cast<std::size_t>(got));
            if (input.size() > 4096) return 81;
        }
        if (input.size() < 188 || input[0] != 'G') return 82;
        std::cout << "{\"streams\":[{\"index\":0,\"codec_type\":\"video\","
                     "\"codec_name\":\"mpeg2video\"}],\"format\":"
                     "{\"format_name\":\"mpegts\"}}";
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "-hide_banner")
        return fakeFfmpeg(argc, argv);

    int cases = 0;
    {
        Fixture f;
        const auto contents = f.bytes("00001.ts");
        const auto inventory = f.entries();
        auto result = checkRecordingTransportStream(f.video, f.relative());
        assert(result.status() == "ok");
        assert(result.segments == 2 && result.packets == 4 &&
               result.bytes == 188 * 4 && result.indexPresent);
        const auto json = result.json();
        assert(json.find("\"status\":\"ok\"") != std::string::npos);
        assert(json.find("\"readOnly\":true") != std::string::npos);
        assert(json.find("\"analysis\":\"ts_structure_only\"") != std::string::npos);
        assert(result.segmentReports.size() == 2);
        assert(result.segmentReports[0].name == "00001.ts" &&
               result.segmentReports[0].status() == "ok" &&
               result.segmentReports[0].packets == 2);
        assert(result.segmentReports[1].name == "00002.ts" &&
               result.segmentReports[1].status() == "ok" &&
               result.segmentReports[1].packets == 2);
        assert(json.find("segmentReports") != std::string::npos);
        assert(f.bytes("00001.ts") == contents && f.entries() == inventory);
        assert(result.sidecarErrors == 0 && result.sidecarWarnings == 0 &&
               result.sidecarReports.size() == 1 &&
               result.sidecarReports[0].name == "index" &&
               result.sidecarReports[0].status == "ok");
        ++cases;
    }
    {
        Fixture f;
        std::ofstream(f.rec / "info") << "T Synthetic recording\\n";
        std::ofstream(f.rec / "de.srt") << "1\\n00:00:01,000 --> 00:00:02,000\\nHallo\\n";
        const auto original = f.entries();
        const auto report = checkRecordingTransportStream(f.video, f.relative());
        assert(report.status() == "ok" &&
               report.sidecarErrors == 0 && report.sidecarWarnings == 0 &&
               report.sidecarReports.size() == 3);
        assert(report.json().find("\"sidecarReports\"") != std::string::npos);
        assert(f.entries() == original);
        ++cases;
    }
    {
        Fixture f;
        fs::create_symlink(f.rec / "index", f.rec / "info");
        const auto report = checkRecordingTransportStream(f.video, f.relative());
        assert(report.status() == "errors" && report.sidecarErrors == 1);
        assert(report.json().find("\"sidecarErrors\":1") != std::string::npos);
        ++cases;
    }
    {
        Fixture f;
        std::ofstream(f.rec / "de.srt");
        const auto report = checkRecordingTransportStream(f.video, f.relative());
        assert(report.status() == "warnings" && report.sidecarWarnings == 1);
        ++cases;
    }
    {
        Fixture f;
        fs::remove(f.rec / "index");
        auto r = checkRecordingTransportStream(f.video, f.relative());
        assert(r.status() == "warnings" && !r.indexPresent);
        ++cases;
    }
    {
        Fixture f;
        f.write("00002.ts", {packet(7)});
        auto r = checkRecordingTransportStream(f.video, f.relative());
        assert(r.status() == "warnings" && r.continuityWarnings == 1);
        assert(r.segmentReports.size() == 2 &&
               r.segmentReports[0].status() == "ok" &&
               r.segmentReports[1].status() == "warnings" &&
               r.segmentReports[1].continuityWarnings == 1);
        ++cases;
    }
    {
        Fixture f;
        Packet discontinuous = packet(7);
        discontinuous[3] = 0x30 | 7; // adaptation + payload
        discontinuous[4] = 1;
        discontinuous[5] = 0x80; // ISO 13818-1 discontinuity indicator
        f.write("00002.ts", {discontinuous, packet(8)});
        const auto r = checkRecordingTransportStream(f.video, f.relative());
        assert(r.status() == "warnings" &&
               r.discontinuities == 1 && r.continuityWarnings == 0);
        ++cases;
    }
    {
        Fixture f;
        Packet badSync = packet(2);
        badSync[0] = 0x00;
        Packet badTransport = packet(3);
        badTransport[1] |= 0x80;
        Packet badAdaptation = packet(4);
        badAdaptation[3] = 0x00;
        f.write("00002.ts", {badSync, badTransport, badAdaptation}, "x");
        auto r = checkRecordingTransportStream(f.video, f.relative());
        assert(r.status() == "errors" && r.syncErrors == 1 &&
               r.transportErrors == 1 && r.adaptationErrors == 1 &&
               r.trailingBytes == 1 && r.packets == 5);
        assert(r.segmentReports.size() == 2 &&
               r.segmentReports[0].status() == "ok" &&
               r.segmentReports[1].status() == "errors" &&
               r.segmentReports[1].syncErrors == 1 &&
               r.segmentReports[1].transportErrors == 1 &&
               r.segmentReports[1].adaptationErrors == 1 &&
               r.segmentReports[1].trailingBytes == 1);
        const auto before = f.bytes("00002.ts");
        assert(f.bytes("00002.ts") == before);
        ++cases;
    }
    {
        Fixture f;
        Packet malformed = packet(2);
        malformed[3] = 0x20 | 2; // adaptation only, no payload
        malformed[4] = 1; // leaves 182 bytes unaccounted for
        f.write("00002.ts", {malformed});
        const auto result = checkRecordingTransportStream(f.video, f.relative());
        assert(result.status() == "errors" && result.adaptationErrors == 1);
        assert(result.segmentReports[1].adaptationErrors == 1);
        ++cases;
    }
    {
        Fixture f;
        Packet valid = packet(2);
        valid[3] = 0x20 | 2;
        valid[4] = 183; // adaptation field occupies the entire packet
        valid[5] = 0; // adaptation flags: no discontinuity indicated
        f.write("00002.ts", {valid});
        const auto result = checkRecordingTransportStream(f.video, f.relative());
        assert(result.status() == "ok" && result.adaptationErrors == 0);
        assert(result.continuityWarnings == 0);
        ++cases;
    }
    {
        Fixture f;
        fs::rename(f.rec / "00002.ts", f.rec / "00003.ts");
        throws([&] { checkRecordingTransportStream(f.video, f.relative()); });
        ++cases;
    }
    {
        Fixture f;
        fs::remove(f.rec / "00002.ts");
        fs::create_symlink(f.rec / "00001.ts", f.rec / "00002.ts");
        throws([&] { checkRecordingTransportStream(f.video, f.relative()); });
        ++cases;
    }
    {
        Fixture f;
        const auto spoofed = f.video / "Alias";
        fs::create_directory_symlink(f.video / "Test", spoofed);
        throws([&] { checkRecordingTransportStream(
            f.video, fs::path("Alias") / f.rec.filename()); });
        ++cases;
    }
    {
        Fixture f;
        throws([&] { checkRecordingTransportStream(
            f.video, fs::path("Test/../Test") / f.rec.filename()); });
        throws([&] { checkRecordingTransportStream(
            f.video, fs::path("../elsewhere.rec")); });
        throws([&] { checkRecordingTransportStream(
            f.video, fs::path("/outside.rec")); });
        throws([&] { checkRecordingTransportStream(
            f.video, fs::path("Test") / "missing.rec"); });
        ++cases;
    }
    {
        Fixture f;
        fs::path linked = f.home / "linked";
        fs::create_directory_symlink(f.video, linked);
        throws([&] { checkRecordingTransportStream(linked, f.relative()); });
        ++cases;
    }
    {
        Fixture f;
        Packet nullPacket = packet(13, 0x1fff);
        f.write("00002.ts", {nullPacket});
        auto r = checkRecordingTransportStream(f.video, f.relative());
        assert(r.status() == "ok" && r.continuityWarnings == 0);
        ++cases;
    }
    {
        Fixture f;
        const auto original = f.bytes("00001.ts");
        const auto entries = f.entries();
        vdrsuite::mediatools::FfprobeMediaCheckProber prober(
            std::chrono::seconds(5), "/proc/self/exe");
        const auto report = vdrsuite::mediatools::checkRecordingWithProber(
            f.video, f.relative(), prober);
        assert(report.status() == "ok" && report.ffprobe &&
               report.ffprobe->status == "ok" &&
               report.ffprobe->details.find("mpeg2video") != std::string::npos);
        assert(report.json().find("\"ffprobe\"") != std::string::npos);
        assert(f.bytes("00001.ts") == original && f.entries() == entries);
        ++cases;
    }
    {
        Fixture f;
        vdrsuite::mediatools::FfprobeMediaCheckProber missing(
            std::chrono::seconds(2), "/usr/bin/vdr-suite-no-ffprobe");
        const auto result = vdrsuite::mediatools::checkRecordingWithProber(
            f.video, f.relative(), missing);
        assert(result.status() == "incomplete" && result.ffprobe &&
               result.ffprobe->status == "incomplete");
        assert(result.json().find("\"status\":\"incomplete\"") !=
               std::string::npos);
        ++cases;
    }
    {
        Fixture f;
        const auto original = f.bytes("00001.ts");
        const auto entries = f.entries();
        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder(
            std::chrono::seconds(5), "/proc/self/exe");
        const auto result = vdrsuite::mediatools::checkRecordingWithDecoder(
            f.video, f.relative(), decoder);
        assert(result.status() == "ok" && result.decode.has_value() &&
               result.decode->status == "ok" &&
               result.decode->exitCode == 0 &&
               result.decode->streamedBytes == 4 * 188);
        assert(result.json().find("\"analysis\":\"ts_structure_and_ffmpeg_decode\"") !=
               std::string::npos);
        assert(f.bytes("00001.ts") == original && f.entries() == entries);
        ++cases;
    }
    {
        Fixture f;
        Packet broken = packet(2);
        constexpr char marker[] = "M2_FORCE_FAIL";
        std::copy(marker, marker + sizeof(marker) - 1, broken.begin() + 20);
        f.write("00002.ts", {broken});
        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder(
            std::chrono::seconds(5), "/proc/self/exe");
        const auto result = vdrsuite::mediatools::checkRecordingWithDecoder(
            f.video, f.relative(), decoder);
        assert(result.status() == "errors" && result.decode &&
               result.decode->exitCode == 7 &&
               result.decode->diagnostic.find("synthetic FFmpeg decoder error") !=
                   std::string::npos);
        ++cases;
    }
    {
        Fixture f;
        Packet stalled = packet(2);
        constexpr char marker[] = "M2_FORCE_STALL";
        std::copy(marker, marker + sizeof(marker) - 1, stalled.begin() + 20);
        f.write("00002.ts", {stalled});
        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder(
            std::chrono::seconds(1), "/proc/self/exe");
        const auto result = vdrsuite::mediatools::checkRecordingWithDecoder(
            f.video, f.relative(), decoder);
        assert(result.status() == "incomplete" && result.decode &&
               result.decode->timedOut);
        ++cases;
    }
    {
        Fixture f;
        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder(
            std::chrono::seconds(2), "/usr/bin/vdr-suite-ffmpeg-no-such-binary");
        const auto result = vdrsuite::mediatools::checkRecordingWithDecoder(
            f.video, f.relative(), decoder);
        assert(result.status() == "incomplete" && result.decode &&
               result.decode->status == "incomplete" &&
               result.decode->exitCode == 127);
        ++cases;
    }
    if (::access("/usr/bin/ffmpeg", X_OK) == 0) {
        // Real FFmpeg integration on disposable, generated sample media.
        // This never reads or writes a genuine VDR recording.
        Fixture f;
        fs::remove(f.rec / "00002.ts");
        const std::string generated = (f.rec / "00001.ts").string();
        const pid_t child = ::fork();
        assert(child >= 0);
        if (child == 0) {
            ::execl("/usr/bin/ffmpeg", "ffmpeg", "-hide_banner", "-nostdin",
                    "-loglevel", "error", "-f", "lavfi", "-i",
                    "testsrc2=size=64x64:rate=8", "-frames:v", "8",
                    "-c:v", "mpeg2video", "-f", "mpegts", "-y",
                    generated.c_str(), static_cast<char*>(nullptr));
            ::_exit(127);
        }
        int status = 0;
        assert(::waitpid(child, &status, 0) == child &&
               WIFEXITED(status) && WEXITSTATUS(status) == 0);
        const auto original = f.bytes("00001.ts");
        vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder(
            std::chrono::seconds(20));
        auto result = vdrsuite::mediatools::checkRecordingWithDecoder(
            f.video, f.relative(), decoder);
        assert(result.decode && result.decode->status == "ok" &&
               result.decode->exitCode == 0 &&
               result.decode->streamedBytes == original.size());
        assert(f.bytes("00001.ts") == original);
        ++cases;
        std::cout << "MEDIA_CHECK_REAL_FFMPEG_DECODE=PASS\n";
    } else {
        std::cout << "MEDIA_CHECK_REAL_FFMPEG_DECODE=SKIP_NO_FFMPEG\n";
    }
    std::cout << "MEDIA_CHECK_READ_ONLY_CPP_TEST=PASS cases=" << cases << "\n";
    return 0;
}
