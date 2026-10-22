#include "MediaImport.h"

#include <cassert>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>

using namespace vdrsuite::mediatools;
namespace fs = std::filesystem;

struct Fixture {
    fs::path base;
    fs::path input;
    fs::path video;
    fs::path source;

    Fixture() {
        std::string templ = (fs::temp_directory_path() / "suite-cpp-import-XXXXXX").string();
        std::vector<char> buffer(templ.begin(), templ.end());
        buffer.push_back('\0');
        char* created = ::mkdtemp(buffer.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        base = created;
        input = base / "input";
        video = base / "video";
        fs::create_directory(input);
        fs::create_directory(video);
        source = input / "Sample.mp4";
        std::ofstream(source) << "synthetic-source-unchanged";
    }
    ~Fixture() { std::error_code ec; fs::remove_all(base, ec); }
    ImportRequest request() const { return {input, video, source}; }
};

struct FakeProcess : ProcessRunner {
    std::string codec{"h264\n"};
    bool failRemux = false;
    bool failIndex = false;
    fs::path probedSource;
    int probes = 0;
    int remuxes = 0;
    int transcodes = 0;
    int indices = 0;
    ProcessResult run(const std::vector<std::string>& argv, bool output) override {
        assert(!argv.empty());
        if (argv[0] == "/usr/bin/ffprobe") {
            assert(output);
            ++probes;
            probedSource = argv.back();
            assert(probedSource.filename() == "input.mp4");
            assert(fs::exists(probedSource));
            std::ifstream in(probedSource, std::ios::binary);
            const std::string data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
            assert(data == "synthetic-source-unchanged");
            return {0, codec};
        }
        assert(!output);
        if (argv[0] == "/usr/bin/ffmpeg") {
            const auto input = std::find(argv.begin(), argv.end(), "-i");
            assert(input != argv.end() && (input + 1) != argv.end());
            assert(fs::path(*(input + 1)) == probedSource);
            assert(std::find(argv.begin(), argv.end(), "-fs") != argv.end());
            if (std::find(argv.begin(), argv.end(), "libx264") != argv.end()) {
                ++transcodes;
                assert(std::find(argv.begin(), argv.end(), "aac") != argv.end());
                assert(std::find(argv.begin(), argv.end(), "yuv420p") != argv.end());
            } else {
                ++remuxes;
                assert(std::find(argv.begin(), argv.end(), "copy") != argv.end());
            }
            if (failRemux) return {1, ""};
            std::ofstream(argv.back()) << "synthetic ts bytes";
            return {0, ""};
        }
        if (argv[0] == "/usr/bin/vdr") {
            ++indices;
            if (failIndex) return {1, ""};
            const std::string param = argv[1];
            assert(param.rfind("--genindex=", 0) == 0);
            std::ofstream(fs::path(param.substr(11)) / "index") << "synthetic index";
            return {0, ""};
        }
        throw std::runtime_error("unexpected native process");
    }
};

template<class F>
void mustFail(F&& f) {
    bool threw = false;
    try { f(); } catch (const std::exception&) { threw = true; }
    assert(threw);
}

int main() {
    int checks = 0;
    {
        Fixture f;
        const auto plan = inspectImport(f.request());
        assert(plan.title == "Sample" && plan.sourceBytes > 0);
        assert(!fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        auto r = f.request();
        r.sourceFile = f.video / "intruder.mp4";
        std::ofstream(r.sourceFile) << "x";
        mustFail([&] { inspectImport(r); });
        r = f.request();
        r.sourceFile = f.input / ".." / "input" / "Sample.mp4";
        mustFail([&] { inspectImport(r); });
        ++checks;
    }
    {
        Fixture f;
        fs::create_symlink(f.source, f.input / "Alias.mp4");
        auto r = f.request();
        r.sourceFile = f.input / "Alias.mp4";
        mustFail([&] { inspectImport(r); });
        ++checks;
    }
    {
        Fixture f;
        fs::create_directories(f.video / "Sample" / "2026-01-01.00.00.1-0.rec");
        mustFail([&] { inspectImport(f.request()); });
        ++checks;
    }
    {
        Fixture f;
        auto request = f.request();
        fs::rename(f.source, f.input / "Sample.webm");
        request.sourceFile = f.input / "Sample.webm";
        mustFail([&] { inspectImport(request); });
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        auto plan = inspectImport(f.request());
        const auto output = executeRemuxImport(plan, runner, [](const ImportPlan&) {});
        assert(fs::exists(output / "00001.ts") && fs::exists(output / "index"));
        assert(fs::exists(f.source));
        assert(!fs::exists(runner.probedSource));
        assert(runner.probes == 1 && runner.remuxes == 1 && runner.indices == 1);
        assert(fs::path(output).extension() == ".rec");
        mustFail([&] { inspectImport(f.request()); });
        ++checks;
    }
    {
        Fixture f;
        std::ofstream(f.input / "Sample.nfo") <<
            "<movie><title>Example &amp; Friends</title>"
            "<outline>Short text</outline><plot>A longer description.</plot>"
            "<year>2024</year><genre>Drama</genre></movie>";
        std::ofstream(f.input / "Sample.srt") <<
            "1\n00:00:01,000 --> 00:00:02,000\nHello\n";
        FakeProcess runner;
        const auto plan = inspectImport(f.request());
        assert(plan.title == "Example & Friends");
        const auto output = executeRemuxImport(
            plan, runner, [](const ImportPlan&) {});
        std::ifstream info(output / "info");
        const std::string contents((std::istreambuf_iterator<char>(info)),
                                    std::istreambuf_iterator<char>());
        assert(contents.find("T Example & Friends\n") != std::string::npos);
        assert(contents.find("S Short text\n") != std::string::npos);
        assert(contents.find("Year: 2024") != std::string::npos);
        assert(contents.find("Genre: Drama") != std::string::npos);
        assert(fs::exists(output / "subtitles.srt"));
        assert(fs::exists(f.input / "Sample.nfo") &&
               fs::exists(f.input / "Sample.srt") && fs::exists(f.source));
        ++checks;
    }
    {
        Fixture f;
        fs::create_symlink(f.source, f.input / "Sample.nfo");
        mustFail([&] { inspectImport(f.request()); });
        assert(fs::exists(f.source));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.codec = "vp9\n";
        auto plan = inspectImport(f.request());
        const auto output = executeRemuxImport(plan, runner, [](const ImportPlan&) {});
        assert(fs::exists(output / "00001.ts") && fs::exists(output / "index"));
        assert(fs::exists(f.source) && runner.remuxes == 0 &&
               runner.transcodes == 1);
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.failIndex = true;
        auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && !fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {
            throw std::runtime_error("capacity refused"); }); });
        assert(fs::exists(f.source) && runner.probes == 0);
        ++checks;
    }

    {
        Fixture f;
        FakeProcess runner;
        runner.failRemux = true;
        const auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && !fs::exists(f.video / "Sample"));
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        auto plan = inspectImport(f.request());
        std::ofstream(f.source, std::ios::app) << "source changed";
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && runner.probes == 0);
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        const auto plan = inspectImport(f.request());
        const auto original = f.input / "Sample-original.mp4";
        fs::rename(f.source, original);
        // Same extension, pathname and length, but different inode.
        std::ofstream(f.source) << "synthetic-source-unchanged";
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(runner.probes == 0 && runner.remuxes == 0);
        assert(fs::exists(original) && fs::exists(f.source));
        ++checks;
    }
    std::cout << "MEDIA_IMPORT_CPP_TEST=PASS checks=" << checks << "\n";
    return 0;
}
