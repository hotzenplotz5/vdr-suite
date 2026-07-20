#ifndef VDR_SUITE_BRIDGE_EPG_ARTWORK_CONTRACT_H
#define VDR_SUITE_BRIDGE_EPG_ARTWORK_CONTRACT_H

#include "suitebridge_artwork_reference.h"

#include <array>
#include <cstddef>
#include <string>

class SuiteBridgeEpgArtworkRequest final {
public:
  SuiteBridgeEpgArtworkRequest(const char *command, const char *option);

  bool Handled() const noexcept;
  bool Valid() const noexcept;
  const std::string &ChannelId() const noexcept;
  unsigned int EventId() const noexcept;

private:
  bool handled_ = false;
  bool valid_ = false;
  std::string channelId_;
  unsigned int eventId_ = 0;
};

class SuiteBridgeEpgArtworkPayload final {
public:
  explicit SuiteBridgeEpgArtworkPayload(
      const SuiteBridgeArtworkReference &artwork) noexcept;

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  static constexpr std::size_t kCapacity = 2048;

  std::array<char, kCapacity> data_{};
  std::size_t size_ = 0;
  bool complete_ = false;
};

#endif
