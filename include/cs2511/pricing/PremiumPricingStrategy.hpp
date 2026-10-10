#pragma once

#include "cs2511/pricing/IPricingStrategy.hpp"

namespace cs2511 {

class PremiumPricingStrategy final : public IPricingStrategy {
 public:
  static constexpr int kDiscountPercent = 5;

  [[nodiscard]] Money quote(const std::vector<OrderLine>& lines) const override;
  [[nodiscard]] std::string_view name() const noexcept override { return "premium"; }
};

}  // namespace cs2511
