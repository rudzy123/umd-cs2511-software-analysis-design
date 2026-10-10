#include "cs2511/events/DomainEventPublisher.hpp"

namespace cs2511 {

void DomainEventPublisher::subscribe(std::shared_ptr<IOrderAcceptedObserver> observer) {
  if (!observer) {
    return;
  }
  observers_.push_back(std::move(observer));
}

void DomainEventPublisher::unsubscribe(const IOrderAcceptedObserver* observer) {
  std::erase_if(observers_, [observer](const std::shared_ptr<IOrderAcceptedObserver>& candidate) {
    return candidate.get() == observer;
  });
}

void DomainEventPublisher::publish(const OrderAccepted& event) {
  const std::vector<std::shared_ptr<IOrderAcceptedObserver>> snapshot = observers_;
  for (const std::shared_ptr<IOrderAcceptedObserver>& observer : snapshot) {
    observer->onOrderAccepted(event);
  }
}

}  // namespace cs2511
