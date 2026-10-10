#include "fakes/FakeOrderRepository.hpp"
#include "fakes/RecordingNotificationFactory.hpp"
#include "fakes/RecordingObserver.hpp"
#include "support/Samples.hpp"

#include "cs2511/application/OrderService.hpp"
#include "cs2511/events/DomainEventPublisher.hpp"
#include "cs2511/notifications/NotificationChannelFactory.hpp"
#include "cs2511/notifications/NotificationLog.hpp"
#include "cs2511/notifications/NotificationText.hpp"
#include "cs2511/persistence/InMemoryOrderRepository.hpp"
#include "cs2511/pricing/PricingStrategyFactory.hpp"
#include "cs2511/validation/OrderValidator.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>

namespace {

struct ServiceHarness {
  cs2511::OrderValidator validator;
  cs2511::PricingStrategyFactory pricing;
  cs2511::test::FakeOrderRepository repository;
  cs2511::DomainEventPublisher publisher;
  cs2511::test::RecordingNotificationFactory channels;
  std::shared_ptr<cs2511::test::RecordingObserver> observer{
      std::make_shared<cs2511::test::RecordingObserver>()};
  cs2511::OrderService service{validator, pricing, repository, publisher, channels};

  ServiceHarness() { publisher.subscribe(observer); }
};

}  // namespace

TEST_CASE("processing a valid order prices, stores, publishes, and notifies") {
  ServiceHarness harness;
  const cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);

  REQUIRE(result.ok());
  REQUIRE(result.value().orderId == "1001");
  REQUIRE(result.value().total.cents() == 1250);
  REQUIRE(result.value().strategyName == "standard");
  REQUIRE(result.value().channelName == "email");

  REQUIRE(harness.repository.saved.size() == 1);
  REQUIRE(harness.repository.saved.front().customerId == "cust-9");
  REQUIRE(harness.repository.saved.front().total.cents() == 1250);
  REQUIRE(harness.repository.saved.front().plan == cs2511::PricingPlan::Standard);

  REQUIRE(harness.observer->events.size() == 1);
  REQUIRE(harness.observer->events.front().orderId == "1001");
  REQUIRE(harness.observer->events.front().total.cents() == 1250);

  REQUIRE(harness.channels.sent_.size() == 1);
  REQUIRE(harness.channels.sent_.front().recipient == "ada@example.com");
  REQUIRE(harness.channels.sent_.front().body ==
          "Order accepted: id=1001 customer=cust-9 totalCents=1250");
}

TEST_CASE("sms orders notify the phone number") {
  ServiceHarness harness;
  cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  request.channel = cs2511::ChannelKind::Sms;
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);

  REQUIRE(result.ok());
  REQUIRE(result.value().channelName == "sms");
  REQUIRE(harness.channels.sent_.size() == 1);
  REQUIRE(harness.channels.sent_.front().recipient == "5550100");
}

TEST_CASE("invalid orders do not leave side effects") {
  ServiceHarness harness;
  cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  request.lines.clear();
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);

  REQUIRE_FALSE(result.ok());
  REQUIRE(result.error().code == cs2511::ErrorCode::InvalidOrder);
  REQUIRE(harness.repository.saved.empty());
  REQUIRE(harness.observer->events.empty());
  REQUIRE(harness.channels.sent_.empty());
}

TEST_CASE("duplicate ids are rejected after pricing and before publication") {
  ServiceHarness harness;
  const cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  REQUIRE(harness.service.process(request).ok());

  const cs2511::Result<cs2511::OrderReceipt> duplicate = harness.service.process(request);
  REQUIRE_FALSE(duplicate.ok());
  REQUIRE(duplicate.error().code == cs2511::ErrorCode::PersistenceFailed);
  REQUIRE(duplicate.error().message == "order id already exists");
  REQUIRE(harness.repository.saved.size() == 1);
  REQUIRE(harness.observer->events.size() == 1);
  REQUIRE(harness.channels.sent_.size() == 1);
}

TEST_CASE("a repository failure skips publication and notification") {
  ServiceHarness harness;
  harness.repository.failNextSave = true;
  const cs2511::Result<cs2511::OrderReceipt> result =
      harness.service.process(cs2511::test::validEmailOrder());

  REQUIRE_FALSE(result.ok());
  REQUIRE(result.error().code == cs2511::ErrorCode::PersistenceFailed);
  REQUIRE(result.error().message == "failed to save order");
  REQUIRE(harness.repository.saved.empty());
  REQUIRE(harness.observer->events.empty());
  REQUIRE(harness.channels.sent_.empty());
}

TEST_CASE("a notification failure keeps the saved order and the published event") {
  ServiceHarness harness;
  harness.channels.failNext_ = true;
  const cs2511::Result<cs2511::OrderReceipt> result =
      harness.service.process(cs2511::test::bulkEmailOrder());

  REQUIRE_FALSE(result.ok());
  REQUIRE(result.error().code == cs2511::ErrorCode::NotificationFailed);
  REQUIRE(result.error().message == "failed to send notification");
  REQUIRE(harness.repository.saved.size() == 1);
  REQUIRE(harness.repository.saved.front().total.cents() == 720);
  REQUIRE(harness.observer->events.size() == 1);
  REQUIRE(harness.observer->events.front().total.cents() == 720);
  REQUIRE(harness.channels.sent_.empty());
}

TEST_CASE("channel factory creates the requested channel") {
  cs2511::NotificationLog log;
  const cs2511::NotificationChannelFactory factory{log};

  const auto email = factory.create(cs2511::ChannelKind::Email);
  const auto sms = factory.create(cs2511::ChannelKind::Sms);
  REQUIRE(email.ok());
  REQUIRE(sms.ok());
  REQUIRE(email.value()->name() == "email");
  REQUIRE(sms.value()->name() == "sms");

  REQUIRE(email.value()->send(cs2511::Notification{"ada@example.com", "hello"}).ok());
  REQUIRE(log.messages().size() == 1);
  REQUIRE(log.messages().front().channel == "email");
  REQUIRE(log.messages().front().body == "hello");
}

TEST_CASE("in-memory repository stores and finds orders") {
  cs2511::InMemoryOrderRepository repository;
  cs2511::StoredOrder order;
  order.id = "1001";
  order.customerId = "cust-9";
  order.total = cs2511::Money{720};
  order.plan = cs2511::PricingPlan::Bulk;
  order.channel = cs2511::ChannelKind::Email;

  REQUIRE(repository.save(order).ok());
  REQUIRE(repository.findById("1001").has_value());
  REQUIRE(repository.findById("1001")->total.cents() == 720);
  REQUIRE_FALSE(repository.findById("missing").has_value());

  const cs2511::Result<cs2511::Unit> duplicate = repository.save(order);
  REQUIRE_FALSE(duplicate.ok());
  REQUIRE(duplicate.error().message == "order id already exists");
  REQUIRE(repository.orders().size() == 1);
}

TEST_CASE("real adapters complete the use case") {
  const cs2511::OrderValidator validator;
  const cs2511::PricingStrategyFactory pricing;
  cs2511::InMemoryOrderRepository repository;
  cs2511::DomainEventPublisher publisher;
  cs2511::NotificationLog log;
  const cs2511::NotificationChannelFactory channels{log};
  const auto observer = std::make_shared<cs2511::test::RecordingObserver>();
  publisher.subscribe(observer);
  cs2511::OrderService service{validator, pricing, repository, publisher, channels};

  const cs2511::Result<cs2511::OrderReceipt> result =
      service.process(cs2511::test::bulkEmailOrder());
  REQUIRE(result.ok());
  REQUIRE(result.value().total.cents() == 720);
  REQUIRE(result.value().strategyName == "bulk");
  REQUIRE(repository.orders().size() == 1);
  REQUIRE(observer->events.size() == 1);
  REQUIRE(log.messages().size() == 1);
  REQUIRE(cs2511::notificationEnvelope(log.messages().front().channel,
                                       log.messages().front().recipient,
                                       log.messages().front().body) ==
          "[email] to=ada@example.com | Order accepted: id=1001 customer=cust-9 totalCents=720");
}
