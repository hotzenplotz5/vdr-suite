#ifndef VDR_SUITE_BRIDGE_OSD_SNAPSHOT_H
#define VDR_SUITE_BRIDGE_OSD_SNAPSHOT_H

#include <array>
#include <cstddef>
#include <cstdint>

enum class SuiteBridgeOsdFrameKind : std::uint8_t {
  Inactive,
  Menu,
  ChannelInfo,
};

struct SuiteBridgeOsdItemSnapshot final {
  static constexpr std::size_t TextBytes = 512;

  std::array<char, TextBytes + 1> text{{0}};
  bool selectable = true;
};

struct SuiteBridgeOsdSnapshot final {
  static constexpr unsigned int SchemaVersion() noexcept
  {
    return 1;
  }

  static constexpr std::size_t EpochHexLength = 32;
  static constexpr std::size_t MaximumItems = 128;
  static constexpr std::size_t TitleBytes = 256;
  static constexpr std::size_t StatusBytes = 512;
  static constexpr std::size_t ButtonBytes = 128;
  static constexpr std::size_t DetailTextBytes = 4096;
  static constexpr std::size_t ChannelBytes = 512;
  static constexpr std::size_t ProgrammeTextBytes = 512;

  bool consistent = true;
  bool active = false;
  bool complete = true;
  SuiteBridgeOsdFrameKind kind = SuiteBridgeOsdFrameKind::Inactive;
  unsigned long long frameSequence = 0;
  unsigned long long observedAtMilliseconds = 0;
  unsigned long long droppedUpdates = 0;
  std::array<char, EpochHexLength + 1> osdEpoch{{0}};

  std::array<char, TitleBytes + 1> title{{0}};
  std::array<char, StatusBytes + 1> statusMessage{{0}};
  std::array<char, ButtonBytes + 1> red{{0}};
  std::array<char, ButtonBytes + 1> green{{0}};
  std::array<char, ButtonBytes + 1> yellow{{0}};
  std::array<char, ButtonBytes + 1> blue{{0}};
  std::array<SuiteBridgeOsdItemSnapshot, MaximumItems> items{};
  std::size_t itemCount = 0;
  int selectedIndex = -1;
  std::array<char, DetailTextBytes + 1> detailText{{0}};

  std::array<char, ChannelBytes + 1> channel{{0}};
  std::int64_t presentTime = 0;
  std::array<char, ProgrammeTextBytes + 1> presentTitle{{0}};
  std::array<char, ProgrammeTextBytes + 1> presentSubtitle{{0}};
  std::int64_t followingTime = 0;
  std::array<char, ProgrammeTextBytes + 1> followingTitle{{0}};
  std::array<char, ProgrammeTextBytes + 1> followingSubtitle{{0}};
};

#endif
