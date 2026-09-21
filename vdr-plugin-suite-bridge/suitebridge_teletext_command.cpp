#include "suitebridge_teletext_command.h"

#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <strings.h>

namespace {

constexpr std::size_t kMaximumPayloadBytes = 96U * 1024U;

bool ParseUnsigned(
    const std::string &value,
    unsigned long maximum,
    unsigned long &parsed)
{
  if (value.empty()) return false;
  errno = 0;
  char *end = nullptr;
  const unsigned long candidate = std::strtoul(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0' || candidate > maximum) {
    return false;
  }
  parsed = candidate;
  return true;
}

const char *ResultName(std::uint8_t result)
{
  switch (result) {
    case OSDTELETEXT_RESULT_OK: return "ok";
    case OSDTELETEXT_RESULT_INVALID_REQUEST: return "invalid_request";
    case OSDTELETEXT_RESULT_NO_LIVE_SERVICE: return "no_live_service";
    case OSDTELETEXT_RESULT_CHANNEL_MISMATCH: return "channel_mismatch";
    case OSDTELETEXT_RESULT_NO_TELETEXT: return "no_teletext";
    case OSDTELETEXT_RESULT_RECEIVER_INACTIVE: return "receiver_inactive";
    case OSDTELETEXT_RESULT_PAGE_NOT_FOUND: return "page_not_found";
  }
  return "unknown";
}

const char *SourceName(std::uint8_t source)
{
  switch (source) {
    case OSDTELETEXT_SOURCE_LIVE: return "live";
    case OSDTELETEXT_SOURCE_CACHED: return "cached";
    default: return "unknown";
  }
}

void AppendJsonString(std::ostringstream &out, const std::string &value)
{
  out << '"';
  for (unsigned char ch : value) {
    switch (ch) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (ch < 0x20) {
          out << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<unsigned int>(ch) << std::dec << std::setfill(' ');
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  out << '"';
}

void AppendUtf8(std::string &out, std::uint32_t codepoint)
{
  if (codepoint < 0x20 || codepoint > 0x10FFFF ||
      (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
    out.push_back(' ');
    return;
  }
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

std::string RowText(const OsdTeletextGetPageV1 &page, unsigned int row)
{
  std::string text;
  text.reserve(OSDTELETEXT_PAGE_COLUMNS * 2U);
  for (unsigned int column = 0; column < OSDTELETEXT_PAGE_COLUMNS; ++column) {
    const OsdTeletextCellV1 &cell =
        page.cells[row * OSDTELETEXT_PAGE_COLUMNS + column];
    if (cell.kind != OSDTELETEXT_CELL_TEXT || cell.codepoint == 0) {
      text.push_back(' ');
    } else {
      AppendUtf8(text, cell.codepoint);
    }
  }
  return text;
}

SuiteBridgeCommandResult Rejected(int replyCode, const char *reason)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  std::ostringstream payload;
  payload << "{\"schemaVersion\":1,\"result\":\"rejected\",\"reason\":";
  AppendJsonString(payload, reason ? reason : "unknown");
  payload << '}';
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult CapabilityReply(
    const ISuiteBridgeTeletextProvider *provider,
    const char *option)
{
  if (option == nullptr || std::string(option) != "1") {
    return Rejected(504, "teletext_capability_schema_unsupported");
  }
  if (provider == nullptr) return Rejected(550, "provider_unavailable");

  OsdTeletextCapabilitiesV1 capabilities{};
  std::string error;
  if (!provider->ReadCapabilities(capabilities, error)) {
    return Rejected(550, error.c_str());
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = 250;
  std::ostringstream payload;
  payload << "{\"schemaVersion\":1"
          << ",\"provider\":\"osdteletext\""
          << ",\"providerSchemaVersion\":" << capabilities.schemaVersion
          << ",\"capability\":\"broadcast.teletext.page\""
          << ",\"available\":true"
          << ",\"rows\":" << capabilities.rows
          << ",\"columns\":" << capabilities.columns
          << ",\"maxSnapshots\":" << capabilities.maxSnapshots
          << ",\"subpages\":" << (capabilities.subpages ? "true" : "false")
          << ",\"level1\":" << (capabilities.level1 ? "true" : "false")
          << ",\"x26Partial\":" << (capabilities.x26Partial ? "true" : "false")
          << ",\"conceal\":" << (capabilities.conceal ? "true" : "false")
          << ",\"blink\":" << (capabilities.blink ? "true" : "false")
          << ",\"doubleSize\":" << (capabilities.doubleSize ? "true" : "false")
          << ",\"flof\":" << (capabilities.flof ? "true" : "false")
          << ",\"topNavigation\":" << (capabilities.topNavigation ? "true" : "false")
          << '}';
  result.payload = payload.str();
  return result;
}

} // namespace

SuiteBridgeCommandResult SuiteBridgeTeletextCommandService::Handle(
    const char *command,
    const char *option) const
{
  if (command == nullptr) return {};
  if (strcasecmp(command, "TTXC") == 0) {
    return CapabilityReply(provider_, option);
  }
  if (strcasecmp(command, "TTXP") != 0) return {};

  if (provider_ == nullptr) return Rejected(550, "provider_unavailable");
  if (option == nullptr) return Rejected(504, "teletext_page_request_invalid");

  std::istringstream input(option);
  std::string schema;
  std::string channelId;
  std::string pageToken;
  std::string subpageToken;
  std::string extra;
  if (!(input >> schema >> channelId >> pageToken >> subpageToken) ||
      (input >> extra) || schema != "1") {
    return Rejected(504, "teletext_page_request_invalid");
  }

  unsigned long parsedPage = 0;
  if (!ParseUnsigned(pageToken, 899, parsedPage) || parsedPage < 100) {
    return Rejected(504, "teletext_page_request_invalid");
  }

  std::uint16_t subpage = OSDTELETEXT_SUBPAGE_AUTO;
  if (subpageToken != "auto") {
    unsigned long parsedSubpage = 0;
    if (!ParseUnsigned(subpageToken, OSDTELETEXT_SUBPAGE_AUTO - 1U, parsedSubpage)) {
      return Rejected(504, "teletext_page_request_invalid");
    }
    subpage = static_cast<std::uint16_t>(parsedSubpage);
  }

  OsdTeletextGetPageV1 page{};
  std::string error;
  if (!provider_->ReadPage(
          channelId,
          static_cast<std::uint16_t>(parsedPage),
          subpage,
          page,
          error)) {
    return Rejected(550, error.c_str());
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = 250;

  std::ostringstream payload;
  payload << "{\"schemaVersion\":1,\"result\":";
  AppendJsonString(payload, ResultName(page.result));
  payload << ",\"resultCode\":" << static_cast<unsigned int>(page.result)
          << ",\"channel\":";
  AppendJsonString(payload, channelId);
  payload << ",\"requestedPage\":" << parsedPage
          << ",\"requestedSubpage\":";
  if (subpage == OSDTELETEXT_SUBPAGE_AUTO) {
    payload << "\"auto\"";
  } else {
    payload << subpage;
  }
  payload << ",\"source\":";
  AppendJsonString(payload, SourceName(page.sourceState));
  payload << ",\"receiverActive\":" << (page.receiverActive ? "true" : "false")
          << ",\"teletextAvailable\":" << (page.teletextAvailable ? "true" : "false")
          << ",\"serviceEpoch\":" << page.serviceEpoch
          << ",\"revision\":" << page.pageRevision
          << ",\"observedAt\":" << page.observedAt
          << ",\"page\":" << page.resolvedPageNumber
          << ",\"subpage\":" << page.resolvedSubpageCode
          << ",\"complete\":" << (page.complete ? "true" : "false")
          << ",\"rows\":" << page.rows
          << ",\"columns\":" << page.columns;

  if (page.result == OSDTELETEXT_RESULT_OK) {
    payload << ",\"text\":[";
    for (unsigned int row = 0; row < OSDTELETEXT_PAGE_ROWS; ++row) {
      if (row != 0) payload << ',';
      AppendJsonString(payload, RowText(page, row));
    }
    payload << "] ,\"cells\":[";
    for (unsigned int index = 0; index < OSDTELETEXT_PAGE_CELL_COUNT; ++index) {
      if (index != 0) payload << ',';
      const OsdTeletextCellV1 &cell = page.cells[index];
      payload << '[' << cell.codepoint
              << ',' << static_cast<unsigned int>(cell.rawChar)
              << ',' << static_cast<unsigned int>(cell.charset)
              << ',' << static_cast<unsigned int>(cell.foreground)
              << ',' << static_cast<unsigned int>(cell.background)
              << ',' << static_cast<unsigned int>(cell.kind)
              << ',' << static_cast<unsigned int>(cell.flags)
              << ']';
    }
    payload << ']';
  }
  payload << '}';

  result.payload = payload.str();
  if (result.payload.size() > kMaximumPayloadBytes) {
    return Rejected(552, "teletext_payload_too_large");
  }
  return result;
}
