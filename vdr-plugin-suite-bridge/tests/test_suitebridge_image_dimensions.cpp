#include "../suitebridge_image_dimensions.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

std::string TemporaryPath(const char *suffix)
{
  return "/tmp/test_suitebridge_image_dimensions_" +
      std::to_string(static_cast<long long>(getpid())) + suffix;
}

void WriteFile(
    const std::string &path,
    const std::vector<unsigned char> &data)
{
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  assert(output);
  output.write(
      reinterpret_cast<const char *>(data.data()),
      static_cast<std::streamsize>(data.size()));
  assert(output.good());
}

void TestJpeg()
{
  const std::string path = TemporaryPath(".jpg");
  WriteFile(path, {
      0xffU, 0xd8U,
      0xffU, 0xe0U, 0x00U, 0x04U, 0x00U, 0x00U,
      0xffU, 0xc0U, 0x00U, 0x11U, 0x08U,
      0x01U, 0x68U, 0x02U, 0x80U,
      0x03U, 0x01U, 0x11U, 0x00U,
      0x02U, 0x11U, 0x00U,
      0x03U, 0x11U, 0x00U});

  SuiteBridgeImageDimensions dimensions;
  assert(SuiteBridgeReadImageDimensions(path, dimensions));
  assert(dimensions.width == 640);
  assert(dimensions.height == 360);
  std::remove(path.c_str());
}

void TestPng()
{
  const std::string path = TemporaryPath(".png");
  WriteFile(path, {
      0x89U, 0x50U, 0x4eU, 0x47U,
      0x0dU, 0x0aU, 0x1aU, 0x0aU,
      0x00U, 0x00U, 0x00U, 0x0dU,
      0x49U, 0x48U, 0x44U, 0x52U,
      0x00U, 0x00U, 0x07U, 0x80U,
      0x00U, 0x00U, 0x04U, 0x38U});

  SuiteBridgeImageDimensions dimensions;
  assert(SuiteBridgeReadImageDimensions(path, dimensions));
  assert(dimensions.width == 1920);
  assert(dimensions.height == 1080);
  std::remove(path.c_str());
}

void TestInvalidMissingAndEmpty()
{
  const std::string invalidPath = TemporaryPath(".invalid");
  WriteFile(invalidPath, {0xffU, 0xd8U, 0xffU});

  SuiteBridgeImageDimensions dimensions;
  dimensions.width = 99;
  dimensions.height = 88;
  assert(!SuiteBridgeReadImageDimensions(invalidPath, dimensions));
  assert(!dimensions.Valid());
  std::remove(invalidPath.c_str());

  dimensions.width = 77;
  dimensions.height = 66;
  assert(!SuiteBridgeReadImageDimensions(
      TemporaryPath(".missing"), dimensions));
  assert(!dimensions.Valid());

  dimensions.width = 55;
  dimensions.height = 44;
  assert(!SuiteBridgeReadImageDimensions("", dimensions));
  assert(!dimensions.Valid());
}

} // namespace

int main()
{
  TestJpeg();
  TestPng();
  TestInvalidMissingAndEmpty();
  return 0;
}
