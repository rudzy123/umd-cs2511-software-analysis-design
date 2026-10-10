#include "fakes/FakeOrderRepository.hpp"
#include "fakes/RecordingNotificationFactory.hpp"
#include "fakes/RecordingObserver.hpp"
#include "support/Samples.hpp"

#include "OrderProcessor.hpp"
#include "cs2511/application/OrderService.hpp"
#include "cs2511/events/DomainEventPublisher.hpp"
#include "cs2511/notifications/NotificationText.hpp"
#include "cs2511/pricing/PricingStrategyFactory.hpp"
#include "cs2511/validation/OrderValidator.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace {

std::string planName(cs2511::PricingPlan plan) {
  switch (plan) {
    case cs2511::PricingPlan::Standard:
      return "standard";
    case cs2511::PricingPlan::Bulk:
      return "bulk";
    case cs2511::PricingPlan::Premium:
      return "premium";
  }
  return "unrecognized";
}

std::string channelName(cs2511::ChannelKind channel) {
  switch (channel) {
    case cs2511::ChannelKind::Email:
      return "email";
    case cs2511::ChannelKind::Sms:
      return "sms";
  }
  return "unrecognized";
}

int runLegacy(cs2511::before::OrderProcessor& processor, const cs2511::OrderRequest& request) {
  std::vector<std::tuple<std::string, int, int>> items;
  for (const cs2511::OrderLine& line : request.lines) {
    items.emplace_back(line.sku, line.quantity, line.unitPrice.cents());
  }
  return processor.process(request.orderId, request.customerId, request.email, request.phone,
                           items, planName(request.plan), channelName(request.channel));
}

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

void requireSameSuccess(const cs2511::OrderRequest& request) {
  cs2511::before::OrderProcessor legacy;
  ServiceHarness harness;
  const int code = runLegacy(legacy, request);
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);

  REQUIRE(code == 0);
  REQUIRE(legacy.lastError().empty());
  REQUIRE(result.ok());
  REQUIRE(result.value().total.cents() == legacy.lastTotalCents());
  REQUIRE(result.value().strategyName == planName(request.plan));
  REQUIRE(result.value().channelName == channelName(request.channel));
  REQUIRE(legacy.savedIds().size() == 1);
  REQUIRE(legacy.savedIds().front() == result.value().orderId);
  REQUIRE(legacy.savedTotals().front() == result.value().total.cents());
  REQUIRE(harness.repository.saved.size() == 1);
  REQUIRE(harness.repository.saved.front().total.cents() == legacy.lastTotalCents());

  REQUIRE(legacy.events().size() == 1);
  REQUIRE(harness.observer->events.size() == 1);
  REQUIRE(legacy.events().front() ==
          cs2511::orderAcceptedEventText(request.orderId, request.customerId,
                                          result.value().total.cents()));
  REQUIRE(harness.observer->events.front().orderId == request.orderId);
  REQUIRE(harness.observer->events.front().total.cents() == legacy.lastTotalCents());

  REQUIRE(legacy.notifications().size() == 1);
  REQUIRE(harness.channels.sent_.size() == 1);
  REQUIRE(legacy.notifications().front() ==
          cs2511::notificationEnvelope(channelName(request.channel),
                                       harness.channels.sent_.front().recipient,
                                       harness.channels.sent_.front().body));
}

void requireSameRejection(const cs2511::OrderRequest& request,
                          int legacyCode,
                          cs2511::ErrorCode code,
                          const std::string& message) {
  cs2511::before::OrderProcessor legacy;
  ServiceHarness harness;
  const int actualLegacy = runLegacy(legacy, request);
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);

  REQUIRE(actualLegacy == legacyCode);
  REQUIRE(legacy.lastError() == message);
  REQUIRE_FALSE(result.ok());
  REQUIRE(result.error().code == code);
  REQUIRE(result.error().message == message);
  REQUIRE(legacy.savedIds().empty());
  REQUIRE(legacy.events().empty());
  REQUIRE(legacy.notifications().empty());
  REQUIRE(harness.repository.saved.empty());
  REQUIRE(harness.observer->events.empty());
  REQUIRE(harness.channels.sent_.empty());
}

}  // namespace

TEST_CASE("legacy and refactored processors agree on priced orders") {
  cs2511::OrderRequest standardOrder = cs2511::test::validEmailOrder();
  requireSameSuccess(standardOrder);

  cs2511::OrderRequest bulkOrder = cs2511::test::bulkEmailOrder();
  requireSameSuccess(bulkOrder);

  cs2511::OrderRequest premiumOrder = cs2511::test::bulkEmailOrder();
  premiumOrder.plan = cs2511::PricingPlan::Premium;
  requireSameSuccess(premiumOrder);

  cs2511::OrderRequest belowBulk = cs2511::test::validEmailOrder();
  belowBulk.plan = cs2511::PricingPlan::Bulk;
  belowBulk.lines = {cs2511::OrderLine{"SKU-1", 9, cs2511::Money{100}}};
  requireSameSuccess(belowBulk);

  cs2511::OrderRequest truncated = cs2511::test::validEmailOrder();
  truncated.plan = cs2511::PricingPlan::Premium;
  truncated.lines = {cs2511::OrderLine{"SKU-1", 1, cs2511::Money{199}}};
  requireSameSuccess(truncated);

  cs2511::OrderRequest smsOrder = cs2511::test::validEmailOrder();
  smsOrder.channel = cs2511::ChannelKind::Sms;
  requireSameSuccess(smsOrder);
}

TEST_CASE("legacy and refactored processors agree on rejected orders") {
  cs2511::OrderRequest missingId = cs2511::test::validEmailOrder();
  missingId.orderId.clear();
  requireSameRejection(missingId, 1, cs2511::ErrorCode::InvalidOrder, "order id is required");

  cs2511::OrderRequest missingCustomer = cs2511::test::validEmailOrder();
  missingCustomer.customerId.clear();
  requireSameRejection(missingCustomer, 1, cs2511::ErrorCode::InvalidOrder,
                       "customer id is required");

  cs2511::OrderRequest missingLines = cs2511::test::validEmailOrder();
  missingLines.lines.clear();
  requireSameRejection(missingLines, 1, cs2511::ErrorCode::InvalidOrder,
                       "order must contain at least one line");

  cs2511::OrderRequest missingSku = cs2511::test::validEmailOrder();
  missingSku.lines.front().sku.clear();
  requireSameRejection(missingSku, 1, cs2511::ErrorCode::InvalidOrder, "sku is required");

  cs2511::OrderRequest badQuantity = cs2511::test::validEmailOrder();
  badQuantity.lines.front().quantity = 0;
  requireSameRejection(badQuantity, 1, cs2511::ErrorCode::InvalidOrder,
                       "quantity must be positive");

  cs2511::OrderRequest badPrice = cs2511::test::validEmailOrder();
  badPrice.lines.front().unitPrice = cs2511::Money{-5};
  requireSameRejection(badPrice, 1, cs2511::ErrorCode::InvalidOrder,
                       "unit price cannot be negative");

  cs2511::OrderRequest badEmail = cs2511::test::validEmailOrder();
  badEmail.email = "ada.example.com";
  requireSameRejection(badEmail, 1, cs2511::ErrorCode::InvalidOrder,
                       "email channel requires an email address");

  cs2511::OrderRequest badPhone = cs2511::test::validEmailOrder();
  badPhone.channel = cs2511::ChannelKind::Sms;
  badPhone.phone = "123";
  requireSameRejection(badPhone, 1, cs2511::ErrorCode::InvalidOrder,
                       "sms channel requires a phone number");
}

TEST_CASE("legacy and refactored processors agree on duplicate ids") {
  const cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  cs2511::before::OrderProcessor legacy;
  ServiceHarness harness;
  REQUIRE(runLegacy(legacy, request) == 0);
  REQUIRE(harness.service.process(request).ok());

  REQUIRE(runLegacy(legacy, request) == 3);
  const cs2511::Result<cs2511::OrderReceipt> second = harness.service.process(request);
  REQUIRE(legacy.lastError() == "order id already exists");
  REQUIRE_FALSE(second.ok());
  REQUIRE(second.error().message == legacy.lastError());
  REQUIRE(legacy.savedIds().size() == 1);
  REQUIRE(legacy.events().size() == 1);
  REQUIRE(legacy.notifications().size() == 1);
  REQUIRE(harness.repository.saved.size() == 1);
  REQUIRE(harness.observer->events.size() == 1);
  REQUIRE(harness.channels.sent_.size() == 1);
}

TEST_CASE("legacy and refactored processors agree when save fails") {
  const cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  cs2511::before::OrderProcessor legacy;
  legacy.setFailNextSave(true);
  ServiceHarness harness;
  harness.repository.failNextSave = true;

  REQUIRE(runLegacy(legacy, request) == 3);
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);
  REQUIRE(legacy.lastError() == "failed to save order");
  REQUIRE_FALSE(result.ok());
  REQUIRE(result.error().code == cs2511::ErrorCode::PersistenceFailed);
  REQUIRE(result.error().message == legacy.lastError());
  REQUIRE(legacy.savedIds().empty());
  REQUIRE(legacy.events().empty());
  REQUIRE(legacy.notifications().empty());
  REQUIRE(harness.repository.saved.empty());
  REQUIRE(harness.observer->events.empty());
  REQUIRE(harness.channels.sent_.empty());
}

TEST_CASE("legacy and refactored processors agree when notification fails") {
  const cs2511::OrderRequest request = cs2511::test::bulkEmailOrder();
  cs2511::before::OrderProcessor legacy;
  legacy.setFailNextNotification(true);
  ServiceHarness harness;
  harness.channels.failNext_ = true;

  REQUIRE(runLegacy(legacy, request) == 4);
  const cs2511::Result<cs2511::OrderReceipt> result = harness.service.process(request);
  REQUIRE(legacy.lastError() == "failed to send notification");
  REQUIRE(legacy.lastTotalCents() == 720);
  REQUIRE_FALSE(result.ok());
  REQUIRE(result.error().code == cs2511::ErrorCode::NotificationFailed);
  REQUIRE(result.error().message == legacy.lastError());
  REQUIRE(legacy.savedIds().size() == 1);
  REQUIRE(legacy.savedTotals().front() == 720);
  REQUIRE(legacy.events().size() == 1);
  REQUIRE(legacy.notifications().empty());
  REQUIRE(harness.repository.saved.size() == 1);
  REQUIRE(harness.repository.saved.front().total.cents() == 720);
  REQUIRE(harness.observer->events.size() == 1);
  REQUIRE(harness.channels.sent_.empty());
}

TEST_CASE("unrecognized plans and channels fail closed on both sides") {
  cs2511::before::OrderProcessor legacy;
  const int planCode =
      legacy.process("1001", "cust-9", "ada@example.com", "5550100", {{"SKU-1", 1, 100}},
                     "nope", "email");
  REQUIRE(planCode == 2);
  REQUIRE(legacy.lastError() == "pricing plan is not recognized");
  REQUIRE(legacy.savedIds().empty());

  ServiceHarness harness;
  cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  request.plan = static_cast<cs2511::PricingPlan>(99);
  const cs2511::Result<cs2511::OrderReceipt> planResult = harness.service.process(request);
  REQUIRE_FALSE(planResult.ok());
  REQUIRE(planResult.error().code == cs2511::ErrorCode::UnknownPricingPlan);
  REQUIRE(planResult.error().message == legacy.lastError());
  REQUIRE(harness.repository.saved.empty());
  REQUIRE(harness.observer->events.empty());

  cs2511::before::OrderProcessor channelLegacy;
  const int channelCode = channelLegacy.process("1001", "cust-9", "ada@example.com", "5550100",
                                                 {{"SKU-1", 1, 100}}, "standard", "pager");
  REQUIRE(channelCode == 1);
  REQUIRE(channelLegacy.lastError() == "notification channel is not recognized");

  request = cs2511::test::validEmailOrder();
  request.channel = static_cast<cs2511::ChannelKind>(42);
  const cs2511::Result<cs2511::OrderReceipt> channelResult = harness.service.process(request);
  REQUIRE_FALSE(channelResult.ok());
  REQUIRE(channelResult.error().code == cs2511::ErrorCode::InvalidOrder);
  REQUIRE(channelResult.error().message == channelLegacy.lastError());
}
