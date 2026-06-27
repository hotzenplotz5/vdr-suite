#include "JsonStringDecoder.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    assert(vdrsuite::decodeJsonStringEscapes("plain") == "plain");
    assert(vdrsuite::decodeJsonStringEscapes("Line\\nBreak") == "Line\nBreak");
    assert(vdrsuite::decodeJsonStringEscapes("Quote: \\\"x\\\"") == "Quote: \"x\"");
    assert(vdrsuite::decodeJsonStringEscapes("Slash: \\\/") == "Slash: /");
    assert(vdrsuite::decodeJsonStringEscapes("Backslash: \\\\") == "Backslash: \\");
    assert(vdrsuite::decodeJsonStringEscapes("Der gro\\u00dfe B\\u00f6rsencrash") ==
           "Der große Börsencrash");
    assert(vdrsuite::decodeJsonStringEscapes("f\\u00fcr Kinder") == "für Kinder");
    assert(vdrsuite::decodeJsonStringEscapes("M\\u00fcnchen") == "München");

    std::cout << "test_json_string_decoder passed" << std::endl;
    return 0;
}
