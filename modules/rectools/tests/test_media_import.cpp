#include "MediaImport.h"

#include <cassert>
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
    int probes = 0;
    int remuxes = 0;
    int indices = 0;
    ProcessResult run(const std::vector<std::string>& argv, bool output) override {
        assert(!argv.empty());
        if (argv[0] == "/usr/bin/ffprobe") {
            assert(output);
            ++probes;
            return {0, codec};
        }
        assert(!output);
        if (argv[0] == "/usr/bin/ffmpeg") {
            ++remuxes;
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
        assert(runner.probes == 1 && runner.remuxes == 1 && runner.indices == 1);
        assert(fs::path(output).extension() == ".rec");
        mustFail([&] { inspectImport(f.request()); });
        ++checks;
    }
    {
        Fixture f;
        FakeProcess runner;
        runner.codec = "vp9\n";
        auto plan = inspectImport(f.request());
        mustFail([&] { executeRemuxImport(plan, runner, [](const ImportPlan&) {}); });
        assert(fs::exists(f.source) && runner.remuxes == 0);
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
    std::cout << "MEDIA_IMPORT_CPP_TEST=PASS checks=" << checks << "\n";
    return 0;
}
