#include "MediaSessionWorkspace.h"
#include "RecordingTrickplayPreviewRenderer.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main()
{
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() /
        "vdr-suite-trickplay-preview-renderer-unit";
    std::error_code ec;
    std::filesystem::remove_all(temp, ec);
    std::filesystem::create_directories(temp / "input");
    {
        std::ofstream source((temp / "input/00001.ts").string());
        source << "test-only-ts";
    }

    const auto plan = RecordingTrickplayPreviewPlanner().build(
        36, 60, 1, {60.0});
    assert(plan.valid);
    int calls = 0;
    RecordingTrickplayPreviewRenderer renderer(
        [&calls](const std::vector<std::string>& args, const std::string& wd,
                 std::chrono::milliseconds timeout, std::size_t outputLimit,
                 MediaProcessLimits limits) {
            ++calls;
            assert(!wd.empty());
            assert(args.size() > 10);
            assert(timeout == std::chrono::seconds(3));
            assert(outputLimit == 256u * 1024u);
            assert(limits.addressSpaceBytes == 768u * 1024u * 1024u);
            assert(limits.cpuSeconds == 2);
            MediaProcessCaptureResult result;
            result.success = true;
            result.completed = true;
            result.output = std::string("\xff\xd8",2) +
                "test-closed-image" + std::string("\xff\xd9",2);
            return result;
        });

    {
        MediaSessionWorkspace empty((temp / "private").string());
        const auto result = renderer.render(plan, empty);
        assert(!result.ready && calls == 0);
        assert(result.reasonCode == "trickplay_workspace_invalid");
    }
    {
        MediaSessionWorkspace workspace((temp / "private").string());
        const auto prepared = workspace.prepare(
            "trickplay-test-recording", {(temp / "input/00001.ts").string()},
            {60.0});
        assert(prepared.ready);
        const auto result = renderer.render(plan, workspace);
        assert(result.ready && result.sampleSeconds == 30);
        assert(result.jpeg.size() == 21);
        assert(calls == 1);

        auto forged = plan;
        forged.argv.push_back("/etc/shadow");
        const auto rejected = renderer.render(forged, workspace);
        assert(!rejected.ready);
        assert(rejected.reasonCode == "trickplay_plan_invalid");
        assert(calls == 1);

        auto unavailable = plan;
        unavailable.valid = false;
        assert(!renderer.render(unavailable, workspace).ready);
        assert(calls == 1);
    }
    assert(!std::filesystem::exists(
        temp / "private" / "trickplay-test-recording"));
    std::filesystem::remove_all(temp, ec);
    return 0;
}
