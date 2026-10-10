#pragma once

#include "cs2511/domain/Order.hpp"
#include "cs2511/domain/Result.hpp"
#include "cs2511/pricing/IPricingStrategy.hpp"

#include <memory>

namespace cs2511 {

class IPricingStrategyFactory {
 public:
  virtual ~IPricingStrategyFactory() = default;

  [[nodiscard]] virtual Result<std::unique_ptr<IPricingStrategy>> create(
      PricingPlan plan) const = 0;
};

}  // namespace cs2511
