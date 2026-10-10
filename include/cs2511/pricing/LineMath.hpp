#pragma once

#include "cs2511/domain/Money.hpp"
#include "cs2511/domain/Order.hpp"

#include <vector>

namespace cs2511 {

[[nodiscard]] inline Money lineSubtotal(const std::vector<OrderLine>& lines) {
  Money total;
  for (const OrderLine& line : lines) {
    total = total + Money{line.quantity * line.unitPrice.cents()};
  }
  return total;
}

[[nodiscard]] inline int totalQuantity(const std::vector<OrderLine>& lines) {
  int quantity = 0;
  for (const OrderLine& line : lines) {
    quantity += line.quantity;
  }
  return quantity;
}

}  // namespace cs2511
