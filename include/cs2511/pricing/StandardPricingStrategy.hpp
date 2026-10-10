#pragma once

#include "cs2511/pricing/IPricingStrategy.hpp"

namespace cs2511 {

class StandardPricingStrategy final : public IPricingStrategy {
 public:
  [[nodiscard]] Money quote(const std::vector<OrderLine>& lines) const override;
  [[nodiscard]] std::string_view name() const noexcept override { return "standard"; }
};

}  // namespace cs2511
