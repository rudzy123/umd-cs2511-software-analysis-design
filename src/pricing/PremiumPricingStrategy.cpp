#include "cs2511/pricing/PremiumPricingStrategy.hpp"

#include "cs2511/pricing/LineMath.hpp"

namespace cs2511 {

Money PremiumPricingStrategy::quote(const std::vector<OrderLine>& lines) const {
  return lineSubtotal(lines).scaledByPercent(100 - kDiscountPercent);
}

}  // namespace cs2511
