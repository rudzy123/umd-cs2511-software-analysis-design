#pragma once

#include "cs2511/domain/OrderAccepted.hpp"
#include "cs2511/events/IOrderAcceptedObserver.hpp"

#include <vector>

namespace cs2511::test {

class RecordingObserver final : public IOrderAcceptedObserver {
 public:
  void onOrderAccepted(const OrderAccepted& event) override { events.push_back(event); }

  std::vector<OrderAccepted> events;
};

}  // namespace cs2511::test
