#pragma once

namespace cs2511 {

// Currency amount in whole cents. A value object keeps quantities and prices
// from being interchangeable integers.
class Money {
 public:
  Money() = default;
  explicit Money(int cents) noexcept : cents_(cents) {}

  [[nodiscard]] int cents() const noexcept { return cents_; }

  [[nodiscard]] Money operator+(Money other) const noexcept {
    return Money{cents_ + other.cents_};
  }

  // Truncates toward zero. Callers pass a non-negative amount.
  [[nodiscard]] Money scaledByPercent(int percent) const noexcept {
    return Money{cents_ * percent / 100};
  }

  [[nodiscard]] bool operator==(Money other) const noexcept {
    return cents_ == other.cents_;
  }

 private:
  int cents_{0};
};

}  // namespace cs2511
