#ifndef _SRC_BASE_STATUS_H_
#define _SRC_BASE_STATUS_H_

#include <memory>
#include <string>
#include <type_traits>

namespace tinyRPC {

namespace detail {}  // namespace detail

// This class describes status code, as its name implies.
//
// `0` is treated as success, other values are failures.
class Status {
 public:
  Status() noexcept = default;
  explicit Status(int status, std::string desc);

  // If an enum type is inherited from `int` (which is the default), we allow
  // constructing `Status` from that type without further casting.
  //
  // Special note: `Status` treats 0 as success. If enumerator with value 0 in
  // `T` is not a successful status, you need to take special care when using
  // it.
  template <class T>
  explicit Status(T status, std::string desc)
      : Status(static_cast<int>(status), desc) {}

  // Test if this object represents a successful status.
  bool ok() const noexcept { return !state_; }

  // Get status value.
  int code() const noexcept { return !state_ ? 0 : state_->status; }

  // Get description of the status.
  std::string message() const noexcept;

  // Returns a human readable string describing the status.
  std::string ToString() const;

 private:
  struct State : public std::enable_shared_from_this<State> {
    int status;
    std::string desc;
  };
  // For successful state, we use `nullptr` here. (For performance reasons.).
  std::shared_ptr<State> state_;
};

}  // namespace tinyRPC

#endif  
