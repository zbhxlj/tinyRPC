#include "Hex.h"

#include <string>
#include <string_view>

#include "HexChars.h"
#include "Logging.h"

namespace tinyRPC {

std::string EncodeHex(std::string_view from, bool uppercase) {
  std::string result;
  EncodeHex(from, &result, uppercase);
  return result;
}

std::optional<std::string> DecodeHex(std::string_view from) {
  std::string result;
  DecodeHex(from, &result);
  return result;
}

void EncodeHex(std::string_view from, std::string* to, bool uppercase) {
  to->reserve(from.size() * 2);
  for (auto&& e : from) {
    auto index = static_cast<std::uint8_t>(e);
    if (uppercase) {
      to->append({detail::kHexCharsUppercase[index].a,
                  detail::kHexCharsUppercase[index].b});
    } else {
      to->append({detail::kHexCharsLowercase[index].a,
                  detail::kHexCharsLowercase[index].b});
    }
  }
}

bool DecodeHex(std::string_view from, std::string* to) {
  if (from.size() % 2 != 0) {
    return false;
  }
  to->reserve(from.size() / 2);
  for (int i = 0; i != from.size(); i += 2) {
    auto v = detail::AsciiCodeFromHexCharPair(from[i], from[i + 1]);
    if (v == -1) {
      return false;
    }
    FLARE_CHECK(v >= 0 && v <= 255);
    to->push_back(v);
  }
  return true;
}

}  // namespace tinyRPC
