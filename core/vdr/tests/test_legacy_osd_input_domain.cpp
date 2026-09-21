#include "LegacyOsdInputDomain.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
LegacyOsdInputCommand commandFor(const std::string& actionName)
{
    LegacyOsdInputAction action;
    assert(legacyOsdInputActionFromName(actionName, action));
    LegacyOsdInputCommand command;
    command.inputCommandId = "input_001";
    command.legacyOsdSessionId = "los_001";
    command.sessionRevision = 7;
    command.viewerBindingId = "ovb_001";
    command.controllerLeaseId = "ocl_001";
    command.controllerLeaseEpoch = 4;
    command.leaseRevision = 3;
    command.actorId = "user:controller";
    command.clientInstanceId = "device:browser";
    command.backendId = "default";
    command.backendGeneration = 9;
    command.osdSurfaceId = "primary-native-osd";
    command.osdEpoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    command.action = action;
    command.inputMode = "press";
    command.repeatCount = 1;
    command.deadline = 1003;
    command.correlationId = "corr:input";
    return command;
}
}

int main()
{
    const std::vector<std::string> allowed = {
        "up", "down", "left", "right", "ok", "back",
        "red", "green", "yellow", "blue"
    };
    for (const std::string& action : allowed)
    {
        LegacyOsdInputCommand original = commandFor(action);
        assert(legacyOsdInputCommandValid(original));
        const std::string encoded =
            legacyOsdInputCommandSerialize(original);
        assert(!encoded.empty());

        LegacyOsdInputCommand parsed;
        assert(legacyOsdInputCommandParse(encoded, parsed));
        assert(parsed.inputCommandId == original.inputCommandId);
        assert(parsed.controllerLeaseId == original.controllerLeaseId);
        assert(parsed.controllerLeaseEpoch ==
            original.controllerLeaseEpoch);
        assert(parsed.leaseRevision == original.leaseRevision);
        assert(parsed.backendGeneration ==
            original.backendGeneration);
        assert(parsed.osdSurfaceId == original.osdSurfaceId);
        assert(parsed.osdEpoch == original.osdEpoch);
        assert(legacyOsdInputActionName(parsed.action) == action);
        assert(parsed.inputMode == "press");
        assert(parsed.repeatCount == 1);
        assert(parsed.deadline == original.deadline);
    }

    LegacyOsdInputAction rejectedAction;
    for (const std::string& rejected :
         {"menu", "play", "pause", "stop", "volume_up",
          "number_1", "42", "raw_key"})
        assert(!legacyOsdInputActionFromName(rejected, rejectedAction));

    auto invalid = commandFor("up");
    invalid.inputMode = "hold";
    assert(!legacyOsdInputCommandValid(invalid));

    invalid = commandFor("up");
    invalid.repeatCount = 2;
    assert(!legacyOsdInputCommandValid(invalid));

    invalid = commandFor("up");
    invalid.inputCommandId = std::string(97, 'x');
    assert(!legacyOsdInputCommandValid(invalid));

    LegacyOsdInputCommand parsed;
    assert(!legacyOsdInputCommandParse(
        "osdi1|input_001|los_001|7|ovb_001|ocl_001|4|3|"
        "user:controller|device:browser|default|9|primary-native-osd|"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa|menu|press|1|1003",
        parsed));

    return 0;
}
