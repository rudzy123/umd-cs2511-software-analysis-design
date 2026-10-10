#include "cs2511/application/OrderService.hpp"
#include "cs2511/events/DomainEventPublisher.hpp"
#include "cs2511/events/IOrderAcceptedObserver.hpp"
#include "cs2511/notifications/NotificationChannelFactory.hpp"
#include "cs2511/notifications/NotificationLog.hpp"
#include "cs2511/notifications/NotificationText.hpp"
#include "cs2511/persistence/InMemoryOrderRepository.hpp"
#include "cs2511/pricing/PricingStrategyFactory.hpp"
#include "cs2511/validation/OrderValidator.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace {

class PrintingObserver final : public cs2511::IOrderAcceptedObserver {
 public:
  void onOrderAccepted(const cs2511::OrderAccepted& event) override { accepted = event; }

  std::optional<cs2511::OrderAccepted> accepted;
};

}  // namespace

int main() {
  using namespace cs2511;

  OrderRequest request;
  request.orderId = "1001";
  request.customerId = "cust-9";
  request.email = "ada@example.com";
  request.phone = "5550100";
  request.lines.push_back(OrderLine{"SKU-1", 6, Money{100}});
  request.lines.push_back(OrderLine{"SKU-2", 4, Money{50}});
  request.plan = PricingPlan::Bulk;
  request.channel = ChannelKind::Email;

  const OrderValidator validator;
  const PricingStrategyFactory pricing;
  InMemoryOrderRepository repository;
  DomainEventPublisher publisher;
  NotificationLog log;
  const NotificationChannelFactory channels{log};
  const auto observer = std::make_shared<PrintingObserver>();
  publisher.subscribe(observer);

  OrderService service{validator, pricing, repository, publisher, channels};
  const Result<OrderReceipt> result = service.process(request);
  if (!result.ok()) {
    std::cerr << result.error().message << '\n';
    return 1;
  }

  const OrderReceipt& receipt = result.value();
  std::cout << "totalCents=" << receipt.total.cents() << '\n';
  std::cout << "strategy=" << receipt.strategyName << '\n';
  std::cout << "channel=" << receipt.channelName << '\n';
  if (observer->accepted.has_value()) {
    const OrderAccepted& event = *observer->accepted;
    std::cout << orderAcceptedEventText(event.orderId, event.customerId, event.total.cents())
              << '\n';
  }
  for (const DeliveredMessage& message : log.messages()) {
    std::cout << notificationEnvelope(message.channel, message.recipient, message.body) << '\n';
  }
  return 0;
}
