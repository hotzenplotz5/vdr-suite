#include "AddonHandlerRegistry.h"
#include <cassert>

int main()
{
    using namespace vdrsuite::addons;
    const auto* handler = reviewedHandler("rectools", "vdr-suite-addon-media-tools");
    assert(handler != nullptr);
    assert(handler->moduleId == "rectools");
    assert(handler->transport == HandlerTransport::backendAgent);
    assert(handler->binaryPath ==
           "/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import");
    assert(!handler->executable);
    assert(reviewedHandler("rectools", "vdr-suite-addon-music") == nullptr);
    assert(reviewedHandler("image", "vdr-suite-addon-image") == nullptr);
    assert(reviewedHandler("music", "vdr-suite-addon-music") == nullptr);
    assert(reviewedHandler("../../rectools", "vdr-suite-addon-media-tools") == nullptr);
}
