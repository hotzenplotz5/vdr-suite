#include "BasicHttpClient.h"
#include "RecordingActionRequestPreviewService.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <cstdlib>
#include <iostream>
#include <string>

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
