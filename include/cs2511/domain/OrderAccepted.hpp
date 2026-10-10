#pragma once

#include "cs2511/domain/Money.hpp"

#include <string>

namespace cs2511 {

struct OrderAccepted {
  std::string orderId;
  std::string customerId;
  Money total{};
};

}  // namespace cs2511
