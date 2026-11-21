#include "SuiteBridgeHandshakeService.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace vdrsuite::agent;

namespace
{

class MissingPluginTransport final : public ISuiteBridgeLocalTransport
{
public:
    SuiteBridgeCommandReply execute(
        const SuiteBridgeLocalCommand command) override
    {
        commands.push_back(command);

        SuiteBridgeCommandReply reply;
        reply.transportStatus = SuiteBridgeTransportStatus::Success;
        reply.replyCode = 550;
        reply.payload =
            "Plugin \"suitebridge\" not found (use PLUG for a list of plugins)";
        return reply;
    }

    std::vector<SuiteBridgeLocalCommand> commands;
};

}

int main()
{
    MissingPluginTransport transport;
    SuiteBridgeHandshakeService service(transport);

    const SuiteBridgeHandshakeResult result = service.perform();

    assert(result.status == SuiteBridgeHandshakeStatus::LegacyOrUnknown);
    assert(result.diagnostic == "suite bridge plugin unavailable");
    assert(!result.ready());
    assert(!result.mutationsEnabled);
    assert(transport.commands.size() == 1);
    assert(transport.commands.front() ==
           SuiteBridgeLocalCommand::DiscoverSchema1);

    std::cout
        << "test_suite_bridge_handshake_missing_plugin passed"
        << std::endl;
    return 0;
}
