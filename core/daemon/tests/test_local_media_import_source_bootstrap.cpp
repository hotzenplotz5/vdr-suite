#include "LocalMediaImportSourceAdmin.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// Read-only contract fixture. No /etc, Agent, VDR or recording mutations.
int main() {
    using namespace vdrsuite::mediatools::local_source_admin;
    assert(revision("") == "cbf29ce484222325");
    assert(rootSyntax("/data/tv"));
    assert(!rootSyntax("/data/tv/../other"));
    assert(!rootSyntax("/root/secrets"));

    Snapshot first;
    first.backend = "default";
    first.videoRoot = "/data/tv";
    first.revision = revision("");
    auto pending = render(first, true);
    assert(pending.statusCode == 200);
    assert(pending.body.find("\\"sources\\":[]") != std::string::npos);
    assert(pending.body.find("\\"videoRoot\\":\\"/data/tv\\"") != std::string::npos);
    assert(pending.body.find("\\"initialSetup\\":true") != std::string::npos);

    Snapshot existing = first;
    existing.sources.push_back({"/data/import", "Import"});
    const auto saved = render(existing);
    assert(saved.body.find("\\"initialSetup\\":false") != std::string::npos);
    assert(saved.body.find("\\"path\\":\\"/data/import\\"") != std::string::npos);

    std::string backend, expected;
    std::vector<Source> sources;
    const std::string body =
        R"({"backendId":"default","revision":"cbf29ce484222325","entries":"%2Ftmp%09Test"})";
    assert(parseBody(body, backend, expected, sources));
    assert(backend == "default");
    assert(expected == revision(""));
    assert(sources.size() == 1 && sources[0].name == "Test");
    assert(!parseBody(
        R"({"backendId":"default","revision":"cbf29ce484222325","entries":"%2Froot%09No"})",
        backend, expected, sources));

    std::cout << "MEDIA_IMPORT_SOURCE_BOOTSTRAP=PASS\n";
}
