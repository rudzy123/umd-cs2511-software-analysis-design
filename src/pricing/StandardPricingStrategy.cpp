#include "cs2511/pricing/StandardPricingStrategy.hpp"

#include "cs2511/pricing/LineMath.hpp"

namespace cs2511 {

Money StandardPricingStrategy::quote(const std::vector<OrderLine>& lines) const {
  return lineSubtotal(lines);
}

}  // namespace cs2511
