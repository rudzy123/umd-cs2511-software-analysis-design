#pragma once

#include "cs2511/domain/Order.hpp"
#include "cs2511/domain/Result.hpp"
#include "cs2511/events/IDomainEventPublisher.hpp"
#include "cs2511/notifications/INotificationChannelFactory.hpp"
#include "cs2511/persistence/IOrderRepository.hpp"
#include "cs2511/pricing/IPricingStrategyFactory.hpp"
#include "cs2511/validation/OrderValidator.hpp"

namespace cs2511 {

// Application service for the place-order use case. Collaborators are
// constructor-injected. This header includes ports, not concrete adapters.
class OrderService final {
 public:
  OrderService(const OrderValidator& validator,
               const IPricingStrategyFactory& pricingFactory,
               IOrderRepository& repository,
               IDomainEventPublisher& publisher,
               const INotificationChannelFactory& channelFactory);

  OrderService(const OrderService&) = delete;
  OrderService& operator=(const OrderService&) = delete;

  [[nodiscard]] Result<OrderReceipt> process(const OrderRequest& request);

 private:
  const OrderValidator& validator_;
  const IPricingStrategyFactory& pricingFactory_;
  IOrderRepository& repository_;
  IDomainEventPublisher& publisher_;
  const INotificationChannelFactory& channelFactory_;
};

}  // namespace cs2511
