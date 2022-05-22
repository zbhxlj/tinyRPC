#include "Endpoint.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Logging.h"

using namespace std::literals;

namespace tinyRPC {

namespace {

std::string SockAddrToString(const sockaddr* addr) {
  static constexpr auto kPortChars = 6;  // ":12345"
  auto af = addr->sa_family;
  switch (af) {
    case AF_INET: {
      std::string result;
      auto p = reinterpret_cast<const sockaddr_in*>(addr);
      result.resize(INET_ADDRSTRLEN + kPortChars + 1 /* '\x00' */);
      CHECK(inet_ntop(af, &p->sin_addr, result.data(), result.size()));
      auto ptr = result.data() + result.find('\x00');
      *ptr++ = ':';
      snprintf(ptr, kPortChars, "%d", ntohs(p->sin_port));
      result.resize(result.find('\x00'));
      return result;
    }
    case AF_INET6: {
      std::string result;
      auto p = reinterpret_cast<const sockaddr_in6*>(addr);
      result.resize(INET6_ADDRSTRLEN + 2 /* "[]" */ + kPortChars +
                    1 /* '\x00' */);
      result[0] = '[';
      CHECK(
          inet_ntop(af, &p->sin6_addr, result.data() + 1, result.size()));
      auto ptr = result.data() + result.find('\x00');
      *ptr++ = ']';
      *ptr++ = ':';
      snprintf(ptr, kPortChars, "%d", ntohs(p->sin6_port));
      result.resize(result.find('\x00'));
      return result;
    }
    case AF_UNIX: {
      auto p = reinterpret_cast<const sockaddr_un*>(addr);
      if (p->sun_path[0] == '\0' && p->sun_path[1] != '\0') {
        return "@"s + &p->sun_path[1];
      } else {
        return p->sun_path;
      }
    }
    default: {
      LOG(FATAL) << "Unimplemented AF\n";
      return std::string("TODO : Endpoint::ToString() for AF %h", af);
    }
  }
}

}  // namespace

#define INTERNAL_PTR() (*reinterpret_cast<sockaddr_storage**>(&storage_))
#define INTERNAL_CPTR() (*reinterpret_cast<sockaddr_storage* const*>(&storage_))

sockaddr* EndpointRetriever::RetrieveAddr() {
  return reinterpret_cast<sockaddr*>(&storage_);
}

socklen_t* EndpointRetriever::RetrieveLength() { return &length_; }

Endpoint EndpointRetriever::Build() {
  return Endpoint(RetrieveAddr(), length_);
}

Endpoint::Endpoint(const sockaddr* addr, socklen_t len) : length_(len) {
  if (length_ <= kOptimizedSize) {
    memcpy(&storage_, addr, length_);
  } else {
    INTERNAL_PTR() = new sockaddr_storage;
    memcpy(INTERNAL_PTR(), addr, length_);
  }
}

void Endpoint::SlowDestroy() { delete INTERNAL_PTR(); }

void Endpoint::SlowCopyUninitialized(const Endpoint& ep) {
  length_ = ep.Length();
  INTERNAL_PTR() = new sockaddr_storage;
  memcpy(INTERNAL_PTR(), ep.Get(), length_);
}

void Endpoint::SlowCopy(const Endpoint& ep) {
  if (!IsTriviallyCopyable()) {
    SlowDestroy();
  }
  if (ep.IsTriviallyCopyable()) {
    length_ = ep.length_;
    memcpy(&storage_, &ep.storage_, length_);
  } else {
    length_ = ep.length_;
    INTERNAL_PTR() = new sockaddr_storage;
    memcpy(INTERNAL_PTR(), ep.Get(), length_);
  }
}

const sockaddr* Endpoint::SlowGet() const {
  return reinterpret_cast<const sockaddr*>(INTERNAL_CPTR());
}

std::string Endpoint::ToString() const {
  if (Empty()) {
    // We don't want to `CHECK(0)` here. Checking if the endpoint is initialized
    // before calling `ToString()` each time is way too complicated.
    return "(null)";
  }
  return SockAddrToString(Get());
}

bool operator==(const Endpoint& left, const Endpoint& right) {
  return memcmp(left.Get(), right.Get(), left.Length()) == 0;
}

Endpoint EndpointFromIpv4(const std::string& ip, std::uint16_t port) {
  EndpointRetriever er;
  auto addr = er.RetrieveAddr();
  auto p = reinterpret_cast<sockaddr_in*>(addr);
  memset(p, 0, sizeof(sockaddr_in));
  PCHECK(inet_pton(AF_INET, ip.c_str(), &p->sin_addr))
               << "Cannot parse " << ip << "\n";
  p->sin_port = htons(port);
  p->sin_family = AF_INET;
  *er.RetrieveLength() = sizeof(sockaddr_in);
  return er.Build();
}

Endpoint EndpointFromIpv6(const std::string& ip, std::uint16_t port) {
  EndpointRetriever er;
  auto addr = er.RetrieveAddr();
  auto p = reinterpret_cast<sockaddr_in6*>(addr);
  memset(p, 0, sizeof(sockaddr_in6));
  PCHECK(inet_pton(AF_INET6, ip.c_str(), &p->sin6_addr))
               << "Cannot parse " << ip << "\n";
  p->sin6_port = htons(port);
  p->sin6_family = AF_INET6;
  *er.RetrieveLength() = sizeof(sockaddr_in6);
  return er.Build();
}

std::string EndpointGetIp(const Endpoint& endpoint) {
  std::string result;
  if (endpoint.Family() == AF_INET) {
    result.resize(INET_ADDRSTRLEN + 1 /* '\x00' */);
    CHECK(inet_ntop(endpoint.Family(),
                          &endpoint.UnsafeGet<sockaddr_in>()->sin_addr,
                          result.data(), result.size()));
  } else if (endpoint.Family() == AF_INET6) {
    result.resize(INET6_ADDRSTRLEN + 1 /* '\x00' */);
    CHECK(inet_ntop(endpoint.Family(),
                          &endpoint.UnsafeGet<sockaddr_in6>()->sin6_addr,
                          result.data(), result.size()));
  } else {
    FLARE_CHECK(
        0, "Unexpected: Address family #{} is not a valid IP address family.",
        endpoint.Family());
  }
  return result.substr(0, result.find('\x00'));
}

std::uint16_t EndpointGetPort(const Endpoint& endpoint) {
  CHECK(
      endpoint.Family() == AF_INET || endpoint.Family() == AF_INET6)
      << "Unexpected: Address family " << endpoint.Family() << " is not a valid IP address family.\n";
  if (endpoint.Family() == AF_INET) {
    return ntohs(
        reinterpret_cast<const sockaddr_in*>(endpoint.Get())->sin_port);
  } else if (endpoint.Family() == AF_INET6) {
    return ntohs(
        reinterpret_cast<const sockaddr_in6*>(endpoint.Get())->sin6_port);
  }
  FLARE_UNREACHABLE();
}


bool IsPrivateIpv4AddressRfc(const Endpoint& addr) {
  constexpr std::pair<std::uint32_t, std::uint32_t> kRanges[] = {
      {0xFF000000, 0x0A000000},  // 10.0.0.0/8
      {0xFFF00000, 0xAC100000},  // 172.16.0.0/12
      {0xFFFF0000, 0xC0A80000},  // 192.168.0.0/16
  };

  if (addr.Family() != AF_INET) {
    return false;
  }

  auto ip = ntohl(addr.UnsafeGet<sockaddr_in>()->sin_addr.s_addr);
  for (auto&& [mask, expected] : kRanges) {
    if ((ip & mask) == expected) {
      return true;
    }
  }
  return false;
}

bool IsPrivateIpv4AddressCorp(const Endpoint& addr) {
  constexpr std::pair<std::uint32_t, std::uint32_t> kRanges[] = {
      {0xFF000000, 0x0A000000},  // 10.0.0.0/8
      {0xFFC00000, 0x64400000},  // 100.64.0.0/10
      {0xFFF00000, 0xAC100000},  // 172.16.0.0/12
      {0xFFFF0000, 0xC0A80000},  // 192.168.0.0/16

      {0xFF000000, 0x09000000},  // 9.0.0.0/8
      {0xFF000000, 0x0B000000},  // 11.0.0.0/8
      {0xFF000000, 0x1E000000},  // 30.0.0.0/8
  };
  if (addr.Family() != AF_INET) {
    return false;
  }

  auto ip = ntohl(addr.UnsafeGet<sockaddr_in>()->sin_addr.s_addr);
  for (auto&& [mask, expected] : kRanges) {
    if ((ip & mask) == expected) {
      return true;
    }
  }
  return false;
}

bool IsGuaIpv6Address(const Endpoint& addr) {
  if (addr.Family() != AF_INET6) {
    return false;
  }
  auto v6 = addr.UnsafeGet<sockaddr_in6>()->sin6_addr;
  // 2000::/3
  return v6.s6_addr[0] >= 0x20 && v6.s6_addr[0] <= 0x3f;
}

std::ostream& operator<<(std::ostream& os, const Endpoint& endpoint) {
  return os << endpoint.ToString();
}

std::optional<Endpoint> TryParseTraits<Endpoint>::TryParse(std::string_view s,
                                                           from_ipv4_t) {
  auto pos = s.find(':');
  if (pos == std::string_view::npos) {
    return {};
  }
  auto ip = std::string(s.substr(0, pos));
  auto port = tinyRPC::TryParse<std::uint16_t>(s.substr(pos + 1));
  if (!port) {
    return {};
  }
  EndpointRetriever er;
  auto addr = er.RetrieveAddr();
  auto p = reinterpret_cast<sockaddr_in*>(addr);
  memset(p, 0, sizeof(sockaddr_in));
  if (inet_pton(AF_INET, ip.c_str(), &p->sin_addr) != 1) {
    return {};
  }
  p->sin_port = htons(*port);
  p->sin_family = AF_INET;
  *er.RetrieveLength() = sizeof(sockaddr_in);
  return er.Build();
}

std::optional<Endpoint> TryParseTraits<Endpoint>::TryParse(std::string_view s,
                                                           from_ipv6_t) {
  auto pos = s.find_last_of(':');
  if (pos == std::string_view::npos) {
    return {};
  }
  if (pos < 2) {
    return {};
  }
  auto ip = std::string(s.substr(1, pos - 2));
  auto port = tinyRPC::TryParse<std::uint16_t>(s.substr(pos + 1));
  if (!port) {
    return {};
  }
  EndpointRetriever er;
  auto addr = er.RetrieveAddr();
  auto p = reinterpret_cast<sockaddr_in6*>(addr);
  memset(p, 0, sizeof(sockaddr_in6));
  if (inet_pton(AF_INET6, ip.c_str(), &p->sin6_addr) != 1) {
    return {};
  }
  p->sin6_port = htons(*port);
  p->sin6_family = AF_INET6;
  *er.RetrieveLength() = sizeof(sockaddr_in6);
  return er.Build();
}

std::optional<Endpoint> TryParseTraits<Endpoint>::TryParse(std::string_view s) {
  if (auto opt = TryParse(s, from_ipv4)) {
    return opt;
  } else {
    return TryParse(s, from_ipv6);
  }
}

Endpoint EndpointFromString(const std::string& ip_port) {
  auto opt = TryParse<Endpoint>(ip_port);
  FLARE_CHECK(!!opt, "Cannot parse endpoint [{}].", ip_port);
  return *opt;
}

}  
