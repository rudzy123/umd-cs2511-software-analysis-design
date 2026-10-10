#include "cs2511/pricing/BulkPricingStrategy.hpp"

#include "cs2511/pricing/LineMath.hpp"

namespace cs2511 {

Money BulkPricingStrategy::quote(const std::vector<OrderLine>& lines) const {
  const Money subtotal = lineSubtotal(lines);
  if (totalQuantity(lines) >= kMinimumQuantity) {
    return subtotal.scaledByPercent(100 - kDiscountPercent);
  }
  return subtotal;
}

}  // namespace cs2511
