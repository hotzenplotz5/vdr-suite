#include "RemoteActionDomain.h"

#include <cassert>

int main()
{
    assert(remoteActionTypeFromName("up") == RemoteActionType::Up);
    assert(remoteActionTypeFromName("volumeUp") == RemoteActionType::VolumeUp);
    assert(remoteActionTypeFromName("switchChannel") == RemoteActionType::SwitchChannel);
    assert(remoteActionTypeFromName("Volume+") == RemoteActionType::Invalid);
    assert(remoteActionTypeFromName("seq") == RemoteActionType::Invalid);
    assert(remoteActionTypeName(RemoteActionType::FastForward) == "fastForward");
    assert(isRemoteActionAllowlisted(RemoteActionType::Previous));
    assert(!isRemoteActionAllowlisted(RemoteActionType::Invalid));
    assert(isRemoteActionRequestToken("remote-4711", 128));
    assert(!isRemoteActionRequestToken("", 128));
    assert(!isRemoteActionRequestToken("bad token", 128));
    return 0;
}
