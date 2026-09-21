#include "BackendAgentOsdObservation.h"
#include "OsdJsonCursor.h"
#include <sstream>
#include <limits>

namespace
{
bool identifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    for (unsigned char c : value)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
    return true;
}
std::string quote(const std::string& text)
{
    static const char* hex = "0123456789abcdef";
    std::string result = "\"";
    for (unsigned char c : text)
    {
        if (c == '"' || c == '\\') { result += '\\'; result += c; }
        else if (c < 32) { result += "\\u00"; result += hex[c >> 4]; result += hex[c & 15]; }
        else result += c;
    }
    return result + '"';
}
}

bool validBackendAgentOsdObservation(const BackendAgentOsdObservation& v)
{
    using State = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;
    if (v.protocolVersion != "vdr-suite-agent/1" || !identifier(v.backendId) ||
        !identifier(v.agentInstanceId) || v.backendGeneration == 0 || v.producerSequence == 0 ||
        v.backendGeneration > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        v.producerSequence > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        static_cast<unsigned>(v.snapshot.state) > static_cast<unsigned>(State::InvalidPayload)) return false;
    const auto& b = v.snapshot.buffer;
    if (!b.hasFrame) return v.snapshot.state != State::Current && !b.resyncRequired;
    if (v.snapshot.state != State::Current && v.snapshot.state != State::ResyncRequired) return false;
    const auto& f = b.observed.frame;
    if (f.surface.backendId != v.backendId || f.surface.backendGeneration != v.backendGeneration ||
        f.surface.surfaceId != "primary-native-osd" || !f.fullFrame || !osdFrameWithinLimits(f) ||
        static_cast<unsigned>(f.state) > 3 || static_cast<unsigned>(f.kind) > 2 ||
        f.selectedIndex < -1 || (f.selectedIndex >= 0 &&
        static_cast<std::size_t>(f.selectedIndex) >= f.items.size())) return false;
    if ((!f.surface.osdEpoch.empty() || f.state != OsdSurfaceState::Inactive) &&
        (f.surface.osdEpoch.size() != 32 || f.surface.osdEpoch.find_first_not_of("0123456789abcdef") != std::string::npos))
        return false;
    if ((f.state == OsdSurfaceState::Inactive) != (f.kind == OsdFrameKind::None)) return false;
    if (v.snapshot.state == State::Current &&
        (b.resyncRequired || !f.complete || !b.observed.sourceConsistent ||
         f.state == OsdSurfaceState::Degraded || f.state == OsdSurfaceState::Suppressed)) return false;
    return v.snapshot.state != State::ResyncRequired || b.resyncRequired;
}

std::string serializeBackendAgentOsdObservation(const BackendAgentOsdObservation& v)
{
    if (!validBackendAgentOsdObservation(v)) return {};
    const auto state = static_cast<unsigned>(v.snapshot.state);
    const auto& f = v.snapshot.buffer.observed.frame;
    const auto surfaceState = static_cast<unsigned>(f.state);
    const auto kind = static_cast<unsigned>(f.kind);
    const auto selected = f.selectedIndex;
    std::ostringstream out;
    out << "{\"osdObservationSchema\":1,\"protocolVersion\":\"vdr-suite-agent/1\"";
    out << ",\"backendId\":" << quote(v.backendId);
    out << ",\"agentInstanceId\":" << quote(v.agentInstanceId);
    out << ",\"backendGeneration\":" << v.backendGeneration;
    out << ",\"producerSequence\":" << v.producerSequence;
    out << ",\"sourceState\":" << state;
    out << ",\"hasFrame\":" << (v.snapshot.buffer.hasFrame ? "true" : "false");
    out << ",\"contentFingerprint\":" << v.snapshot.buffer.contentFingerprint;
    out << ",\"resyncRequired\":" << (v.snapshot.buffer.resyncRequired ? "true" : "false");
    if (v.snapshot.buffer.hasFrame)
    {
    out << ",\"surfaceId\":" << quote(f.surface.surfaceId);
    out << ",\"osdEpoch\":" << quote(f.surface.osdEpoch);
    out << ",\"surfaceState\":" << surfaceState;
    out << ",\"kind\":" << kind;
    out << ",\"frameSequence\":" << f.frameSequence;
    out << ",\"observedAt\":" << f.observedAt;
    out << ",\"fullFrame\":" << (f.fullFrame ? "true" : "false");
    out << ",\"complete\":" << (f.complete ? "true" : "false");
    out << ",\"sourceConsistent\":" << (v.snapshot.buffer.observed.sourceConsistent ? "true" : "false");
    out << ",\"droppedUpdates\":" << v.snapshot.buffer.observed.droppedUpdates;
    out << ",\"title\":" << quote(f.title);
    out << ",\"statusMessage\":" << quote(f.statusMessage);
    out << ",\"red\":" << quote(f.red);
    out << ",\"green\":" << quote(f.green);
    out << ",\"yellow\":" << quote(f.yellow);
    out << ",\"blue\":" << quote(f.blue);
    out << ",\"selectedIndex\":" << selected;
    out << ",\"text\":" << quote(f.text);
    out << ",\"channel\":" << quote(f.channel);
    out << ",\"presentTime\":" << f.programme.presentTime;
    out << ",\"presentTitle\":" << quote(f.programme.presentTitle);
    out << ",\"presentSubtitle\":" << quote(f.programme.presentSubtitle);
    out << ",\"followingTime\":" << f.programme.followingTime;
    out << ",\"followingTitle\":" << quote(f.programme.followingTitle);
    out << ",\"followingSubtitle\":" << quote(f.programme.followingSubtitle);

        out << ",\"items\":[";
        bool first = true;
        for (const auto& item : f.items)
        {
            if (!first) out << ',';
            first = false;
            out << "{\"text\":" << quote(item.text) << ",\"selectable\":"
                << (item.selectable ? "true" : "false") << '}';
        }
        out << ']';
    }
    out << '}';
    auto body = out.str();
    return body.size() <= BackendAgentOsdObservation::MaximumBodyBytes ? body : std::string{};
}

bool parseBackendAgentOsdObservation(const std::string& body, BackendAgentOsdObservation& output)
{
    output = {};
    if (body.size() > BackendAgentOsdObservation::MaximumBodyBytes) return false;
    BackendAgentOsdObservation v;
    auto& f = v.snapshot.buffer.observed.frame;
    std::uint64_t schema = 0, state = 0, surfaceState = 0, kind = 0;
    std::int64_t selected = -1;
    vdrsuite::agent::osd_json::Cursor c(body);
    if (!c.consume('{') || !c.key("osdObservationSchema") || !c.unsignedInteger(schema) || schema != 1 ||
        !c.consume(',') || !c.key("protocolVersion") || !c.string(v.protocolVersion, 64)) return false;
    if (!c.consume(',') || !c.key("backendId") || !c.string(v.backendId, 128)) return false;
    if (!c.consume(',') || !c.key("agentInstanceId") || !c.string(v.agentInstanceId, 128)) return false;
    if (!c.consume(',') || !c.key("backendGeneration") || !c.unsignedInteger(v.backendGeneration)) return false;
    if (!c.consume(',') || !c.key("producerSequence") || !c.unsignedInteger(v.producerSequence)) return false;
    if (!c.consume(',') || !c.key("sourceState") || !c.unsignedInteger(state)) return false;
    if (!c.consume(',') || !c.key("hasFrame") || !c.boolean(v.snapshot.buffer.hasFrame)) return false;
    if (!c.consume(',') || !c.key("contentFingerprint") || !c.unsignedInteger(v.snapshot.buffer.contentFingerprint)) return false;
    if (!c.consume(',') || !c.key("resyncRequired") || !c.boolean(v.snapshot.buffer.resyncRequired)) return false;
    if (v.snapshot.buffer.hasFrame)
    {
    if (!c.consume(',') || !c.key("surfaceId") || !c.string(f.surface.surfaceId, 128)) return false;
    if (!c.consume(',') || !c.key("osdEpoch") || !c.string(f.surface.osdEpoch, 32)) return false;
    if (!c.consume(',') || !c.key("surfaceState") || !c.unsignedInteger(surfaceState)) return false;
    if (!c.consume(',') || !c.key("kind") || !c.unsignedInteger(kind)) return false;
    if (!c.consume(',') || !c.key("frameSequence") || !c.unsignedInteger(f.frameSequence)) return false;
    if (!c.consume(',') || !c.key("observedAt") || !c.unsignedInteger(f.observedAt)) return false;
    if (!c.consume(',') || !c.key("fullFrame") || !c.boolean(f.fullFrame)) return false;
    if (!c.consume(',') || !c.key("complete") || !c.boolean(f.complete)) return false;
    if (!c.consume(',') || !c.key("sourceConsistent") || !c.boolean(v.snapshot.buffer.observed.sourceConsistent)) return false;
    if (!c.consume(',') || !c.key("droppedUpdates") || !c.unsignedInteger(v.snapshot.buffer.observed.droppedUpdates)) return false;
    if (!c.consume(',') || !c.key("title") || !c.string(f.title, 256)) return false;
    if (!c.consume(',') || !c.key("statusMessage") || !c.string(f.statusMessage, 512)) return false;
    if (!c.consume(',') || !c.key("red") || !c.string(f.red, 128)) return false;
    if (!c.consume(',') || !c.key("green") || !c.string(f.green, 128)) return false;
    if (!c.consume(',') || !c.key("yellow") || !c.string(f.yellow, 128)) return false;
    if (!c.consume(',') || !c.key("blue") || !c.string(f.blue, 128)) return false;
    if (!c.consume(',') || !c.key("selectedIndex") || !c.signedInteger(selected)) return false;
    if (!c.consume(',') || !c.key("text") || !c.string(f.text, 4096)) return false;
    if (!c.consume(',') || !c.key("channel") || !c.string(f.channel, 512)) return false;
    if (!c.consume(',') || !c.key("presentTime") || !c.signedInteger(f.programme.presentTime)) return false;
    if (!c.consume(',') || !c.key("presentTitle") || !c.string(f.programme.presentTitle, 512)) return false;
    if (!c.consume(',') || !c.key("presentSubtitle") || !c.string(f.programme.presentSubtitle, 512)) return false;
    if (!c.consume(',') || !c.key("followingTime") || !c.signedInteger(f.programme.followingTime)) return false;
    if (!c.consume(',') || !c.key("followingTitle") || !c.string(f.programme.followingTitle, 512)) return false;
    if (!c.consume(',') || !c.key("followingSubtitle") || !c.string(f.programme.followingSubtitle, 512)) return false;

        if (!c.consume(',') || !c.key("items") || !c.consume('[')) return false;
        if (!c.consumeIf(']'))
        {
            do
            {
                if (f.items.size() >= OsdFrameLimits::MaximumItems) return false;
                OsdTextItem item;
                if (!c.consume('{') || !c.key("text") || !c.string(item.text, OsdFrameLimits::ItemTextBytes) ||
                    !c.consume(',') || !c.key("selectable") || !c.boolean(item.selectable) || !c.consume('}')) return false;
                f.items.push_back(std::move(item));
                if (c.consumeIf(']')) break;
                if (!c.consume(',')) return false;
            } while (true);
        }
    }
    if (!c.consume('}') || !c.finished() || state > 7 || surfaceState > 3 || kind > 2 ||
        selected < -1 || selected >= static_cast<std::int64_t>(OsdFrameLimits::MaximumItems)) return false;
    f.selectedIndex = static_cast<int>(selected);
    f.state = static_cast<OsdSurfaceState>(surfaceState);
    f.kind = static_cast<OsdFrameKind>(kind);
    f.surface.backendId = v.backendId;
    f.surface.backendGeneration = v.backendGeneration;
    v.snapshot.state = static_cast<vdrsuite::agent::SuiteBridgeOsdFrameSourceState>(state);
    if (!validBackendAgentOsdObservation(v)) return false;
    output = std::move(v);
    return true;
}
