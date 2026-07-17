#include "recording_name.h"

#include <vdr/plugin.h>
#include <vdr/recording.h>

#include <string>

namespace {

constexpr const char* VERSION = "0.1.0";
constexpr const char* DESCRIPTION = "Native VDR execution bridge for VDR-Suite";

bool splitMoveOption(
    const char* option,
    std::string& source,
    std::string& targetFolder,
    std::string& error)
{
    if (option == nullptr) {
        error = "missing MOVR arguments";
        return false;
    }

    const std::string arguments(option);
    const std::size_t separator = arguments.find('\t');
    if (separator == std::string::npos) {
        error = "MOVR requires source and target folder separated by a tab";
        return false;
    }

    source = arguments.substr(0, separator);
    targetFolder = arguments.substr(separator + 1);

    if (source.empty()) {
        error = "MOVR source is empty";
        return false;
    }

    return true;
}

class cPluginSuiteAgent final : public cPlugin
{
public:
    const char* Version() override
    {
        return VERSION;
    }

    const char* Description() override
    {
        return DESCRIPTION;
    }

    const char** SVDRPHelpPages() override
    {
        static const char* HelpPages[] = {
            "MOVR <source-file-name><TAB><target-folder>\n"
            "    Move a recording through cRecording::ChangeName().\n"
            "    The real cRecording::Name() leaf is preserved.\n"
            "    '/' or an empty target folder selects the recording root.",
            nullptr
        };

        return HelpPages;
    }

    cString SVDRPCommand(
        const char* command,
        const char* option,
        int& replyCode) override
    {
        if (command == nullptr || strcasecmp(command, "MOVR") != 0) {
            return nullptr;
        }

        std::string source;
        std::string targetFolder;
        std::string parseError;
        if (!splitMoveOption(option, source, targetFolder, parseError)) {
            replyCode = 501;
            return cString(parseError.c_str());
        }

        LOCK_RECORDINGS_WRITE;

        cRecording* recording = Recordings->GetByName(source.c_str());
        if (recording == nullptr) {
            replyCode = 550;
            return cString("recording not found");
        }

        const vdrsuite::agent::RecordingMoveNameResult nameResult =
            vdrsuite::agent::buildMovedRecordingName(
                recording->Name(),
                targetFolder);

        if (!nameResult.success) {
            replyCode = 501;
            return cString(nameResult.error.c_str());
        }

        const std::string oldFileName(recording->FileName());
        if (!recording->ChangeName(nameResult.newName.c_str())) {
            replyCode = 550;
            return cString("VDR rejected recording move");
        }

        Recordings->TouchUpdate();
        cRecordingUserCommand::InvokeCommand(
            RUC_MOVEDRECORDING,
            recording->FileName(),
            oldFileName.c_str());

        replyCode = 250;
        return cString::sprintf(
            "moved\t%s\t%s",
            nameResult.newName.c_str(),
            recording->FileName());
    }
};

} // namespace

VDRPLUGINCREATOR(cPluginSuiteAgent);
