#pragma once

#include "cs2511/domain/OrderAccepted.hpp"

namespace cs2511 {

class IOrderAcceptedObserver {
 public:
  virtual ~IOrderAcceptedObserver() = default;
  virtual void onOrderAccepted(const OrderAccepted& event) = 0;
};

}  // namespace cs2511
