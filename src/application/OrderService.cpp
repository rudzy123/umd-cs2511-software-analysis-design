#include "cs2511/application/OrderService.hpp"

#include "cs2511/domain/OrderAccepted.hpp"
#include "cs2511/notifications/NotificationText.hpp"
#include "cs2511/pricing/IPricingStrategy.hpp"

#include <memory>
#include <string>
#include <utility>

namespace cs2511 {

OrderService::OrderService(const OrderValidator& validator,
                           const IPricingStrategyFactory& pricingFactory,
                           IOrderRepository& repository,
                           IDomainEventPublisher& publisher,
                           const INotificationChannelFactory& channelFactory)
    : validator_(validator),
      pricingFactory_(pricingFactory),
      repository_(repository),
      publisher_(publisher),
      channelFactory_(channelFactory) {}

Result<OrderReceipt> OrderService::process(const OrderRequest& request) {
  if (const Result<Unit> validation = validator_.validate(request); !validation.ok()) {
    return Result<OrderReceipt>::failure(validation.error());
  }

  Result<std::unique_ptr<IPricingStrategy>> strategyResult = pricingFactory_.create(request.plan);
  if (!strategyResult.ok()) {
    return Result<OrderReceipt>::failure(strategyResult.error());
  }
  const std::unique_ptr<IPricingStrategy> strategy = std::move(strategyResult.value());
  const Money total = strategy->quote(request.lines);

  if (repository_.findById(request.orderId).has_value()) {
    return Result<OrderReceipt>::failure(
        Error{ErrorCode::PersistenceFailed, "order id already exists"});
  }

  StoredOrder stored;
  stored.id = request.orderId;
  stored.customerId = request.customerId;
  stored.total = total;
  stored.plan = request.plan;
  stored.channel = request.channel;
  if (const Result<Unit> saved = repository_.save(stored); !saved.ok()) {
    return Result<OrderReceipt>::failure(saved.error());
  }

  publisher_.publish(OrderAccepted{request.orderId, request.customerId, total});

  Result<std::unique_ptr<INotificationChannel>> channelResult =
      channelFactory_.create(request.channel);
  if (!channelResult.ok()) {
    return Result<OrderReceipt>::failure(channelResult.error());
  }
  const std::unique_ptr<INotificationChannel> channel = std::move(channelResult.value());

  Notification notification;
  notification.recipient =
      request.channel == ChannelKind::Email ? request.email : request.phone;
  notification.body = orderAcceptedText(request.orderId, request.customerId, total.cents());
  if (const Result<Unit> sent = channel->send(notification); !sent.ok()) {
    return Result<OrderReceipt>::failure(sent.error());
  }

  OrderReceipt receipt;
  receipt.orderId = request.orderId;
  receipt.total = total;
  receipt.strategyName = std::string{strategy->name()};
  receipt.channelName = std::string{channel->name()};
  return Result<OrderReceipt>::success(std::move(receipt));
}

}  // namespace cs2511
