#include "MediaSessionWorkspace.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path.string(), std::ios::binary);
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

void writeFile(const std::filesystem::path& path)
{
    std::ofstream stream(path.string(), std::ios::binary);
    stream << "media";
}

} // namespace

int main()
{
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() /
        "vdr-suite-phase65-workspace-test";
    const std::filesystem::path sourceRoot = temp / "native recording.rec";
    const std::filesystem::path workspaceRoot = temp / "private-media";

    std::error_code ignored;
    std::filesystem::remove_all(temp, ignored);
    std::filesystem::create_directories(sourceRoot);
    writeFile(sourceRoot / "00001.ts");
    writeFile(sourceRoot / "00002.ts");

    {
        MediaSessionWorkspace workspace(workspaceRoot.string());
        const auto result = workspace.prepare(
            "media-session-0123456789abcdef",
            {
                (sourceRoot / "00001.ts").string(),
                (sourceRoot / "00002.ts").string()
            });

        assert(result.ready);
        assert(result.reasonCode.empty());
        assert(!workspace.directory().empty());
        assert(std::filesystem::is_directory(workspace.directory()));
        assert(std::filesystem::is_symlink(
            std::filesystem::path(workspace.directory()) / "source-000001.ts"));
        assert(std::filesystem::is_symlink(
            std::filesystem::path(workspace.directory()) / "source-000002.ts"));

        const std::string concat = readFile(workspace.concatPath());
        assert(concat.find("ffconcat version 1.0") != std::string::npos);
        assert(concat.find("file 'source-000001.ts'") != std::string::npos);
        assert(concat.find("file 'source-000002.ts'") != std::string::npos);
        assert(concat.find(sourceRoot.string()) == std::string::npos);
    }

    assert(!std::filesystem::exists(
        workspaceRoot / "media-session-0123456789abcdef"));

    {
        MediaSessionWorkspace workspace(workspaceRoot.string());
        const auto unsafe = workspace.prepare(
            "../escape",
            {(sourceRoot / "00001.ts").string()});
        assert(!unsafe.ready);
        assert(unsafe.reasonCode == "invalid_workspace_request");
    }

    std::filesystem::remove_all(temp, ignored);
    return 0;
}
