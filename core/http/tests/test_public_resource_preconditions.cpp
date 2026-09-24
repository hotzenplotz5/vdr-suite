#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

using namespace vdrsuite::http;

int main()
{
    assert(publicStrongEntityTag("").empty());
    assert(publicStrongEntityTag("1") == "\"vsr-31\"");
    assert(publicStrongEntityTag("rev:7") ==
        "\"vsr-7265763a37\"");

    const std::string current =
        publicStrongEntityTag("revision-7");
    assert(!current.empty());

    std::string decodedRevision;
    assert(publicStrongEntityTagResourceRevision(
        publicStrongEntityTag("7"),
        decodedRevision));
    assert(decodedRevision == "7");
    assert(publicStrongEntityTagResourceRevision(
        publicStrongEntityTag("revision:42"),
        decodedRevision));
    assert(decodedRevision == "revision:42");
    assert(!publicStrongEntityTagResourceRevision(
        "W/" + publicStrongEntityTag("7"),
        decodedRevision));
    assert(!publicStrongEntityTagResourceRevision(
        "*",
        decodedRevision));
    assert(!publicStrongEntityTagResourceRevision(
        "\"vsr-3A\"",
        decodedRevision));
    assert(!publicStrongEntityTagResourceRevision(
        "\"other-31\"",
        decodedRevision));
    assert(!publicStrongEntityTagResourceRevision(
        publicStrongEntityTag("7") + ", " +
            publicStrongEntityTag("8"),
        decodedRevision));

    assert(publicEvaluateIfMatch("", current) ==
        PublicEntityTagConditionResult::missing);
    assert(publicEvaluateIfMatch("*", current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfMatch(current, current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfMatch(
        "\"other\", " + current,
        current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfMatch(
        "W/" + current,
        current) ==
        PublicEntityTagConditionResult::notMatched);
    assert(publicEvaluateIfMatch(
        "\"other\"",
        current) ==
        PublicEntityTagConditionResult::notMatched);
    assert(publicEvaluateIfMatch(
        "*, " + current,
        current) ==
        PublicEntityTagConditionResult::malformed);
    assert(publicEvaluateIfMatch(
        "\"unterminated",
        current) ==
        PublicEntityTagConditionResult::malformed);

    assert(publicEvaluateIfNoneMatch("", current) ==
        PublicEntityTagConditionResult::missing);
    assert(publicEvaluateIfNoneMatch("*", current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfNoneMatch(current, current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfNoneMatch(
        "W/" + current,
        current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfNoneMatch(
        "\"other\", W/" + current,
        current) ==
        PublicEntityTagConditionResult::matched);
    assert(publicEvaluateIfNoneMatch(
        "\"other\"",
        current) ==
        PublicEntityTagConditionResult::notMatched);
    assert(publicEvaluateIfNoneMatch(
        "\"bad tag\"",
        current) ==
        PublicEntityTagConditionResult::malformed);

    bool wildcard = false;
    std::vector<PublicEntityTag> tags;
    assert(publicEntityTagParseList(
        "\"opaque,comma\", \"second\"",
        wildcard,
        tags));
    assert(!wildcard);
    assert(tags.size() == 2U);
    assert(tags[0].opaque == "opaque,comma");
    assert(tags[1].opaque == "second");

    return 0;
}
