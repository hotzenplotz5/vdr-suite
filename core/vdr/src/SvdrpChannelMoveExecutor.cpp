
#include "SvdrpChannelMoveExecutor.h"

#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace
{
std::string logicalCommand(
    int sourceNumber,
    int targetNumber)
{
    std::ostringstream stream;
    stream << "MOVC " << sourceNumber << " " << targetNumber;
    return stream.str();
}

std::string shellCommand(
    const std::string& command,
    int timeoutSeconds,
    int sourceNumber,
    int targetNumber)
{
    std::ostringstream stream;
    stream
        << "timeout "
        << timeoutSeconds
        << "s "
        << command
        << " MOVC "
        << sourceNumber
        << " "
        << targetNumber
        << " 2>&1";
    return stream.str();
}

bool looksSuccessful(
    const std::string& output)
{
    return output.find("250") != std::string::npos &&
           output.find("501") == std::string::npos &&
           output.find("550") == std::string::npos &&
           output.find("554") == std::string::npos;
}
}

SvdrpChannelMoveExecutor::SvdrpChannelMoveExecutor(
    std::string command,
    int timeoutSeconds)
    : command_(std::move(command)),
      timeoutSeconds_(timeoutSeconds)
{
}

VdrChannelMoveResult SvdrpChannelMoveExecutor::moveChannel(
    const VdrChannelMoveRequest& request)
{
    VdrChannelMoveResult result;
    result.backendId = request.backendId.empty() ? "default" : request.backendId;
    result.sourceNumber = request.sourceNumber;
    result.targetNumber = request.targetNumber;
    result.command = logicalCommand(
        request.sourceNumber,
        request.targetNumber);

    const std::string command =
        shellCommand(
            command_,
            timeoutSeconds_,
            request.sourceNumber,
            request.targetNumber);

    std::array<char, 256> buffer{};
    std::string output;

    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        result.success = false;
        result.message = "failed to start svdrpsend";
        result.errors.push_back(result.message);
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }

    const int closeStatus =
        pclose(pipe);

    result.rawOutput = output;

    if (closeStatus != 0 || !looksSuccessful(output))
    {
        result.success = false;
        result.message = "SVDRP MOVC failed";
        result.errors.push_back(result.message);

        if (closeStatus != 0)
        {
            result.errors.push_back("svdrpsend exited with non-zero status");
        }

        return result;
    }

    result.success = true;
    result.message = "channel moved via SVDRP MOVC";
    return result;
}
