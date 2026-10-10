#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace cs2511 {

enum class ErrorCode {
  InvalidOrder,
  UnknownPricingPlan,
  PersistenceFailed,
  NotificationFailed,
  UnknownChannel
};

struct Error {
  ErrorCode code{};
  std::string message;
};

// Empty success payload for operations that have no domain value to return.
struct Unit {
  friend bool operator==(Unit, Unit) noexcept { return true; }
};

// Explicit success or failure. Domain code returns this instead of hiding a
// failure in a bare integer or a boolean.
template <typename T>
class Result {
 public:
  static Result success(T value) {
    Result result;
    result.value_ = std::move(value);
    return result;
  }

  static Result failure(Error error) {
    Result result;
    result.error_ = std::move(error);
    return result;
  }

  [[nodiscard]] bool ok() const noexcept { return value_.has_value(); }

  [[nodiscard]] const T& value() const {
    if (!value_) {
      throw std::logic_error("Result has no value");
    }
    return *value_;
  }

  [[nodiscard]] T& value() {
    if (!value_) {
      throw std::logic_error("Result has no value");
    }
    return *value_;
  }

  [[nodiscard]] const Error& error() const {
    if (!error_) {
      throw std::logic_error("Result has no error");
    }
    return *error_;
  }

 private:
  Result() = default;

  std::optional<T> value_;
  std::optional<Error> error_;
};

}  // namespace cs2511
