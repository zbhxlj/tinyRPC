#include "Status.h"
#include "internal/Logging.h"

namespace tinyRPC {

Status::Status(int status, std::string desc) {
  if (status == 0) {
    FLARE_LOG_ERROR_IF_ONCE(
        !desc.empty(),
        "Status `SUCCESS` may not carry description, but [{}] is given.", desc);
    // NOTHING else.
  } else {
    state_ = std::make_shared<State>();
    state_->status = status;
    state_->desc = desc;
  }
}

std::string Status::message() const noexcept {
  return !state_ ? std::string()  : state_->desc;
}

std::string Status::ToString() const {
  return fmt::format(
      "[{}] {}", code(),
      !ok() ? message() : "The operation completed successfully.");
}

}  // namespace tinyRPC
