#ifndef _SRC_BASE_ENCODING_HEX_H_
#define _SRC_BASE_ENCODING_HEX_H_

#include <optional>
#include <string>
#include <string_view>

namespace tinyRPC {

std::string EncodeHex(std::string_view from, bool uppercase = false);
std::optional<std::string> DecodeHex(std::string_view from);

void EncodeHex(std::string_view from, std::string* to, bool uppercase = false);
bool DecodeHex(std::string_view from, std::string* to);

}  // namespace tinyRPC

#endif  
