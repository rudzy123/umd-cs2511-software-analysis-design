#pragma once

#include "cs2511/pricing/IPricingStrategy.hpp"

namespace cs2511 {

class BulkPricingStrategy final : public IPricingStrategy {
 public:
  static constexpr int kMinimumQuantity = 10;
  static constexpr int kDiscountPercent = 10;

  [[nodiscard]] Money quote(const std::vector<OrderLine>& lines) const override;
  [[nodiscard]] std::string_view name() const noexcept override { return "bulk"; }
};

}  // namespace cs2511
