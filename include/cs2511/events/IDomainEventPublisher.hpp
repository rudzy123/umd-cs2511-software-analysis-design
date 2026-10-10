#pragma once

#include "cs2511/domain/OrderAccepted.hpp"

namespace cs2511 {

// Publish-only port. Subscribing is a composition-root concern, so the
// application service does not see subscribe or unsubscribe.
class IDomainEventPublisher {
 public:
  virtual ~IDomainEventPublisher() = default;
  virtual void publish(const OrderAccepted& event) = 0;
};

}  // namespace cs2511
