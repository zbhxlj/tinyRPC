#ifndef _SRC_BASE_TYPE_INDEX_H_
#define _SRC_BASE_TYPE_INDEX_H_

#include <functional>
#include <typeindex>

namespace tinyRPC {

namespace detail {

// For each type, there is only one instance of `TypeIndexEntity`. `TypeIndex`
// keeps a reference to the entity, which is used for comparison and other
// stuff.
struct TypeIndexEntity {
  std::type_index runtime_type_index;
};

template <class T>
inline const TypeIndexEntity kTypeIndexEntity{std::type_index(typeid(T))};

}  // namespace detail

// Due to QoI issue in libstdc++, which uses `strcmp` for comparing
// `std::type_index`, we roll our own.
//
// Note our own does NOT support runtime type, only compile time type is
// recognized.
class TypeIndex {
 public:
  constexpr TypeIndex() noexcept : entity_(nullptr) {}

  // In case you need `std::type_index` of the corresponding type, this method
  // is provided for you convenience.
  //
  // Keep in mind, though, that this method can be slow. In most cases you
  // should only use it for logging purpose.
  std::type_index GetRuntimeTypeIndex() const {
    return entity_->runtime_type_index;
  }

  constexpr bool operator==(TypeIndex t) const noexcept {
    return entity_ == t.entity_;
  }
  constexpr bool operator!=(TypeIndex t) const noexcept {
    return entity_ != t.entity_;
  }
  constexpr bool operator<(TypeIndex t) const noexcept {
    return entity_ < t.entity_;
  }

  // If C++20 is usable, you get all other comparison operators (!=, <=, >, ...)
  // automatically. I'm not gonna implement them as we don't use them for the
  // time being.

 private:
  template <class T>
  friend constexpr TypeIndex GetTypeIndex();
  friend struct std::hash<TypeIndex>;

  constexpr explicit TypeIndex(const detail::TypeIndexEntity* entity) noexcept
      : entity_(entity) {}

 private:
  const detail::TypeIndexEntity* entity_;
};

template <class T>
constexpr TypeIndex GetTypeIndex() {
  return TypeIndex(&detail::kTypeIndexEntity<T>);
}

}  // namespace tinyRPC

namespace std {

template <>
struct hash<tinyRPC::TypeIndex> {
  size_t operator()(const tinyRPC::TypeIndex& type) const noexcept {
    return std::hash<const void*>{}(type.entity_);
  }
};

}  // namespace tinyRPC

#endif  
