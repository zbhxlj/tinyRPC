#ifndef _SRC_BASE_ENCODING_DETAIL_HEX_CHARS_H_
#define _SRC_BASE_ENCODING_DETAIL_HEX_CHARS_H_

#include <array>

#include "Logging.h"

namespace tinyRPC::detail {

struct CharPair {
  char a, b;
};

constexpr auto kHexCharsLowercase = [] {
  char chars[] = "0123456789abcdef";
  std::array<CharPair, 256> result{};
  for (int i = 0; i != 256; ++i) {
    result[i].a = chars[(i >> 4) & 0xF];
    result[i].b = chars[i & 0xF];
  }
  return result;
}();

constexpr auto kHexCharsUppercase = [] {
  char chars[] = "0123456789ABCDEF";
  std::array<CharPair, 256> result{};
  for (int i = 0; i != 256; ++i) {
    result[i].a = chars[(i >> 4) & 0xF];
    result[i].b = chars[i & 0xF];
  }
  return result;
}();

constexpr auto kHexCharToDecimal = [] {
  std::array<int, 256> result{};
  for (auto&& e : result) {
    e = -1;
  }
  for (int i = 0; i != 10; ++i) {
    result[i + '0'] = i;
  }
  for (int i = 0; i != 6; ++i) {
    result[i + 'a'] = 10 + i;
    result[i + 'A'] = 10 + i;
  }
  return result;
}();

inline int AsciiCodeFromHexCharPair(std::uint8_t x, std::uint8_t y) {
  auto a = kHexCharToDecimal[x], b = kHexCharToDecimal[y];
  if (a == -1 || b == -1) {
    return -1;
  }
  FLARE_DCHECK(a >= 0 && a < 16 && b >= 0 && b < 16);
  return a * 16 + b;
}

}  // namespace tinyRPC::detail

#endif  
