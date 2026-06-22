#include "BasicHttpClient.h"
#include "RecordingActionRequestPreviewService.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
struct Options
{
    std::string host = "127.0.0.1";
    int port = 8002;
    std::string source;
    std::string target;
    bool execute = false;
    bool help = false;
};

void printUsage()
{
    std::cout
        << "Usage:\n"
        << "  restfulapi_recording_action_real_move_smoke "
        << "--host 127.0.0.1 --port 8002 "
        << "--source '<vdr-recording-source>' "
        << "--target 'Archive/Target' [--execute]\n\n"
        << "Default mode is preview-only. Real execution requires both:\n"
        << "  --execute\n"
        << "  VDR_SUITE_ALLOW_REAL_RECORDING_ACTION=YES\n";
}

Options parseOptions(
    int argc,
    char** argv)
{
    Options options;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];

        if (argument == "--help" || argument == "-h")
        {
            options.help = true;
        }
        else if (argument == "--execute")
        {
            options.execute = true;
        }
        else if (argument == "--host" && index + 1 < argc)
        {
            options.host = argv[++index];
        }
        else if (argument == "--port" && index + 1 < argc)
        {
            options.port = std::stoi(argv[++index]);
        }
        else if (argument == "--source" && index + 1 < argc)
        {
            options.source = argv[++index];
        }
        else if (argument == "--target" && index + 1 < argc)
        {
            options.target = argv[++index];
        }
        else
        {
            std::cerr << "Unknown or incomplete argument: "
                      << argument << "\n";
            options.help = true;
        }
    }

    return options;
}

RestfulApiRecordingActionBackendConfig makeConfig(
    const Options& options)
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "real-vdr";
    config.host = options.host;
    config.port = options.port;
    config.basePath = "";
    config.readOnly = !options.execute;
    config.allowExecution = options.execute;
    return config;
}

RecordingActionRequest makeRequest(
    const Options& options)
{
    RecordingActionRequest request;
    request.backendId = "real-vdr";
    request.recordingId = options.source;
    request.type = RecordingActionType::Move;
    request.dryRun = !options.execute;
    request.parameters["recordingPath"] = options.source;
    request.parameters["targetPath"] = options.target;
    return request;
}

RecordingActionJobPayload makePayload(
    const Options& options)
{
    RecordingActionJobPayload payload;
    payload.backendId = "real-vdr";
    payload.recordingId = options.source;
    payload.type = RecordingActionType::Move;
    payload.dryRun = !options.execute;
    payload.parameters["recordingPath"] = options.source;
    payload.parameters["targetPath"] = options.target;
    return payload;
}


bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

bool hasTestMarker(
    const std::string& path)
{
    return contains(path, "VDR-SUITE-TEST");
}

std::string readRecordings(
    BasicHttpClient& httpClient)
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/recordings.json";
    request.headers["Accept"] = "application/json";

    const HttpResponse response =
        httpClient.execute(request);

    if (response.statusCode < 200 || response.statusCode >= 300)
    {
        return "";
    }

    return response.body;
}

void printCheck(
    bool& allPassed,
    const std::string& name,
    const bool passed,
    const std::string& details = "")
{
    allPassed = allPassed && passed;

    std::cout
        << (passed ? "PASS " : "FAIL ")
        << name;

    if (!details.empty())
    {
        std::cout << " - " << details;
    }

    std::cout << "\n";
}

bool realExecutionAllowedByEnvironment()
{
    const char* value =
        std::getenv("VDR_SUITE_ALLOW_REAL_RECORDING_ACTION");

    return value != nullptr &&
           std::string(value) == "YES";
}
}

int main(
    int argc,
    char** argv)
{
    const Options options =
        parseOptions(argc, argv);

    if (options.help)
    {
        printUsage();
        return 0;
    }

    if (options.source.empty() || options.target.empty())
    {
        printUsage();
        return 2;
    }

    if (!hasTestMarker(options.source) || !hasTestMarker(options.target))
    {
        std::cerr
            << "Refusing move smoke without VDR-SUITE-TEST marker "
            << "in both --source and --target.\n";
        return 8;
    }

    const RestfulApiRecordingActionBackendConfig config =
        makeConfig(options);

    RecordingActionRequestPreviewService previewService;

    const RecordingActionRequestPreviewResult preview =
        previewService.previewRestfulApiRequest(
            makeRequest(options),
            config);

    std::cout
        << "Preview success: " << (preview.success ? "true" : "false") << "\n"
        << "Method: " << preview.method << "\n"
        << "URL: " << preview.url << "\n"
        << "Body: " << preview.body << "\n";

    if (!preview.success)
    {
        std::cerr << "Preview failed. Refusing execution.\n";
        return 3;
    }

    if (!options.execute)
    {
        std::cout << "Preview-only mode. No HTTP mutation sent.\n";
        return 0;
    }

    if (!realExecutionAllowedByEnvironment())
    {
        std::cerr
            << "Refusing real execution. Set "
            << "VDR_SUITE_ALLOW_REAL_RECORDING_ACTION=YES "
            << "in addition to --execute.\n";
        return 4;
    }

    BasicHttpClient httpClient(
        options.host,
        options.port);

    bool checksPassed = true;

    const std::string beforeRecordings =
        readRecordings(httpClient);

    printCheck(
        checksPassed,
        "READBACK before move",
        !beforeRecordings.empty(),
        "recordingsBytes=" + std::to_string(beforeRecordings.size()));

    printCheck(
        checksPassed,
        "SOURCE exists before move",
        contains(beforeRecordings, options.source),
        options.source);

    if (!checksPassed)
    {
        std::cerr << "Pre-move verification failed. Refusing execution.\n";
        return 9;
    }

    RestfulApiRecordingActionExecutor executor(
        "real-vdr",
        "restfulapi",
        config,
        httpClient);

    const RecordingActionExecutionResult result =
        executor.execute(makePayload(options));

    std::cout
        << "Execution success: " << (result.success ? "true" : "false") << "\n"
        << "Message: " << result.message << "\n";

    for (const std::string& error : result.errors)
    {
        std::cerr << "Error: " << error << "\n";
    }

    if (!result.success)
    {
        return 5;
    }

    const std::string afterRecordings =
        readRecordings(httpClient);

    printCheck(
        checksPassed,
        "READBACK after move",
        !afterRecordings.empty(),
        "recordingsBytes=" + std::to_string(afterRecordings.size()));

    printCheck(
        checksPassed,
        "SOURCE gone after move",
        !contains(afterRecordings, options.source),
        options.source);

    printCheck(
        checksPassed,
        "TARGET present after move",
        contains(afterRecordings, options.target),
        options.target);

    if (!checksPassed)
    {
        return 10;
    }

    if (result.message != "RESTfulAPI recording action request executed")
    {
        std::cerr << "Unexpected success message: "
                  << result.message << "\n";
        return 6;
    }

    if (!result.errors.empty())
    {
        std::cerr << "Successful execution returned unexpected errors.\n";
        return 7;
    }

    return 0;
}
