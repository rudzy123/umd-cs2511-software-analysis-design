#include "cs2511/pricing/PricingStrategyFactory.hpp"

#include "cs2511/pricing/BulkPricingStrategy.hpp"
#include "cs2511/pricing/PremiumPricingStrategy.hpp"
#include "cs2511/pricing/StandardPricingStrategy.hpp"

#include <memory>

namespace cs2511 {

Result<std::unique_ptr<IPricingStrategy>> PricingStrategyFactory::create(
    PricingPlan plan) const {
  switch (plan) {
    case PricingPlan::Standard:
      return Result<std::unique_ptr<IPricingStrategy>>::success(
          std::make_unique<StandardPricingStrategy>());
    case PricingPlan::Bulk:
      return Result<std::unique_ptr<IPricingStrategy>>::success(
          std::make_unique<BulkPricingStrategy>());
    case PricingPlan::Premium:
      return Result<std::unique_ptr<IPricingStrategy>>::success(
          std::make_unique<PremiumPricingStrategy>());
  }

  return Result<std::unique_ptr<IPricingStrategy>>::failure(
      Error{ErrorCode::UnknownPricingPlan, "pricing plan is not recognized"});
}

}  // namespace cs2511
