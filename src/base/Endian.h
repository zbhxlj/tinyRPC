#ifndef _SRC_BASE_ENDIAN_H_
#define _SRC_BASE_ENDIAN_H_

#include <cstddef>
#include <cstdint>

#include <bit>  

namespace std{
  enum class endian
  {
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
  };

}

namespace tinyRPC {

namespace endian {

namespace detail {

// For all types not overloaded explicitly, we'll raise an error here.
template <class T>
T SwapEndian(T) = delete;

inline std::byte SwapEndian(std::byte v) { return v; }
inline std::int8_t SwapEndian(std::int8_t v) { return v; }
inline std::uint8_t SwapEndian(std::uint8_t v) { return v; }

#ifdef _MSC_VER

inline std::int16_t SwapEndian(std::int16_t v) { return _byteswap_ushort(v); }
inline std::int32_t SwapEndian(std::int32_t v) { return _byteswap_ulong(v); }
inline std::int64_t SwapEndian(std::int64_t v) { return _byteswap_uint64(v); }
inline std::uint16_t SwapEndian(std::uint16_t v) { return _byteswap_ushort(v); }
inline std::uint32_t SwapEndian(std::uint32_t v) { return _byteswap_ulong(v); }
inline std::uint64_t SwapEndian(std::uint64_t v) { return _byteswap_uint64(v); }

#else

inline std::int16_t SwapEndian(std::int16_t v) { return __builtin_bswap16(v); }

inline std::int32_t SwapEndian(std::int32_t v) { return __builtin_bswap32(v); }

inline std::int64_t SwapEndian(std::int64_t v) { return __builtin_bswap64(v); }

inline std::uint16_t SwapEndian(std::uint16_t v) {
  return __builtin_bswap16(v);
}

inline std::uint32_t SwapEndian(std::uint32_t v) {
  return __builtin_bswap32(v);
}

inline std::uint64_t SwapEndian(std::uint64_t v) {
  return __builtin_bswap64(v);
}

#endif

}  // namespace detail

// Convert `T` between big endian and native endian.
//
// `T` must be specify explicitly so as not to introduce unintended implicit
// type conversion.
//
// If `v` is a big endian value, a native endian one is returned. If `v` is a
// native endian one, a big endian one is returned.
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline T SwapForBigEndian(std::common_type_t<T> v) {
  if constexpr (std::endian::native == std::endian::big) {
    return v;
  } else {
    return detail::SwapEndian(v);
  }
}

// Convert `T` between little endian and native endian.
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline T SwapForLittleEndian(std::common_type_t<T> v) {
  if constexpr (std::endian::native == std::endian::little) {
    return v;
  } else {
    return detail::SwapEndian(v);
  }
}

}  // namespace endian

// Convert native endian to big endian.
//
// `T` must be specify explicitly so as not to introduce unintended implicit
// type conversion.
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline auto FromBigEndian(std::common_type_t<T> v) {
  return endian::SwapForBigEndian<T>(v);
}

// Convert big endian to native endian.
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline auto ToBigEndian(std::common_type_t<T> v) {
  return endian::SwapForBigEndian<T>(v);
}

// Convert native endian to little endian.
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline auto FromLittleEndian(std::common_type_t<T> v) {
  return endian::SwapForLittleEndian<T>(v);
}

// Convert little endian to native endian.
template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline auto ToLittleEndian(std::common_type_t<T> v) {
  return endian::SwapForLittleEndian<T>(v);
}

// Inplace version of conversions above.
//
// For inplace version, we do not require the caller to specify `T` explicitly
// as the output & input type are guaranteed to be the same (i.e., `T`).

template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline void FromBigEndian(T* v) {
  *v = FromBigEndian<T>(*v);
}

template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline void ToBigEndian(T* v) {
  *v = ToBigEndian<T>(*v);
}

template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline void FromLittleEndian(T* v) {
  *v = FromLittleEndian<T>(*v);
}

template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
inline void ToLittleEndian(T* v) {
  *v = ToLittleEndian<T>(*v);
}

// BigToNative / NativeToBig? Why would you want that anyway?

}  // namespace tinyRPC

#endif  
