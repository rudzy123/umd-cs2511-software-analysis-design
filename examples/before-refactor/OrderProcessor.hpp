#pragma once

// EDUCATIONAL ANTI-PATTERN.
// Safe to compile and run. Unsafe to imitate in production code.
//
// OrderProcessor validates, prices, stores, publishes, and notifies inside one
// type. Plans and channels are strings. Failures are bare integers whose
// meaning lives in this comment:
//   0 success
//   1 validation failure
//   2 unrecognized pricing plan
//   3 persistence failure
//   4 notification failure
//
// The refactored design lives under include/ and src/. See
// docs/refactoring-notes.md.

#include <string>
#include <tuple>
#include <vector>

namespace cs2511::before {

class OrderProcessor {
 public:
  int process(const std::string& orderId,
              const std::string& customerId,
              const std::string& email,
              const std::string& phone,
              const std::vector<std::tuple<std::string, int, int>>& items,
              const std::string& plan,
              const std::string& channel);

  void setFailNextSave(bool fail) { failNextSave_ = fail; }
  void setFailNextNotification(bool fail) { failNextNotification_ = fail; }

  [[nodiscard]] int lastCode() const noexcept { return lastCode_; }
  [[nodiscard]] int lastTotalCents() const noexcept { return lastTotalCents_; }
  [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }
  [[nodiscard]] const std::vector<std::string>& savedIds() const noexcept { return savedIds_; }
  [[nodiscard]] const std::vector<int>& savedTotals() const noexcept { return savedTotals_; }
  [[nodiscard]] const std::vector<std::string>& events() const noexcept { return events_; }
  [[nodiscard]] const std::vector<std::string>& notifications() const noexcept {
    return notifications_;
  }

 private:
  int lastCode_{0};
  int lastTotalCents_{0};
  std::string lastError_;
  bool failNextSave_{false};
  bool failNextNotification_{false};
  std::vector<std::string> savedIds_;
  std::vector<int> savedTotals_;
  std::vector<std::string> events_;
  std::vector<std::string> notifications_;
};

}  // namespace cs2511::before
