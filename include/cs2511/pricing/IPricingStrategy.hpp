#pragma once

#include "cs2511/domain/Money.hpp"
#include "cs2511/domain/Order.hpp"

#include <string_view>
#include <vector>

namespace cs2511 {

// Strategy: one pricing policy behind a stable operation.
class IPricingStrategy {
 public:
  virtual ~IPricingStrategy() = default;

  [[nodiscard]] virtual Money quote(const std::vector<OrderLine>& lines) const = 0;
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

}  // namespace cs2511
