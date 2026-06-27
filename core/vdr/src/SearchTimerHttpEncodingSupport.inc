std::string urlEncode(const std::string& value)
{
    std::ostringstream out;
    const char* digits = "0123456789ABCDEF";

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            out << character;
        }
        else
        {
            out << '%' << digits[(character >> 4) & 0x0F];
            out << digits[character & 0x0F];
        }
    }

    return out.str();
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream out;

    for (const char character : value)
    {
        switch (character)
        {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << character; break;
        }
    }

    return out.str();
}
