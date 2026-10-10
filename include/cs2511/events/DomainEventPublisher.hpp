#pragma once

#include "cs2511/events/IDomainEventPublisher.hpp"
#include "cs2511/events/IOrderAcceptedObserver.hpp"

#include <memory>
#include <vector>

namespace cs2511 {

// Observer: the publisher tells subscribers about OrderAccepted and does not
// know what they do with it. The publisher shares ownership for as long as
// the subscription lasts.
class DomainEventPublisher final : public IDomainEventPublisher {
 public:
  DomainEventPublisher() = default;
  DomainEventPublisher(const DomainEventPublisher&) = delete;
  DomainEventPublisher& operator=(const DomainEventPublisher&) = delete;

  void subscribe(std::shared_ptr<IOrderAcceptedObserver> observer);
  void unsubscribe(const IOrderAcceptedObserver* observer);
  void publish(const OrderAccepted& event) override;

 private:
  std::vector<std::shared_ptr<IOrderAcceptedObserver>> observers_;
};

}  // namespace cs2511
