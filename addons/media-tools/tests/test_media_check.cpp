#include "MediaCheck.h"

#include <array>
#include <cassert>
#include <cstdlib>
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

} // namespace

int main()
{
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
        assert(f.bytes("00001.ts") == contents && f.entries() == inventory);
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
        const auto before = f.bytes("00002.ts");
        assert(f.bytes("00002.ts") == before);
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
    std::cout << "MEDIA_CHECK_READ_ONLY_CPP_TEST=PASS cases=" << cases << "\n";
    return 0;
}
