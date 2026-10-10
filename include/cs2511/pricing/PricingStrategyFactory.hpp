#pragma once

#include "cs2511/pricing/IPricingStrategyFactory.hpp"

namespace cs2511 {

class PricingStrategyFactory final : public IPricingStrategyFactory {
 public:
  [[nodiscard]] Result<std::unique_ptr<IPricingStrategy>> create(
      PricingPlan plan) const override;
};

}  // namespace cs2511
