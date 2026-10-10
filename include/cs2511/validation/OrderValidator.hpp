#pragma once

#include "cs2511/domain/Order.hpp"
#include "cs2511/domain/Result.hpp"

namespace cs2511 {

class OrderValidator {
 public:
  [[nodiscard]] Result<Unit> validate(const OrderRequest& request) const;
};

}  // namespace cs2511
