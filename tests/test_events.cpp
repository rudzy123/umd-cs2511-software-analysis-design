#include "fakes/RecordingObserver.hpp"

#include "cs2511/events/DomainEventPublisher.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>

TEST_CASE("publisher notifies every subscriber") {
  cs2511::DomainEventPublisher publisher;
  const auto first = std::make_shared<cs2511::test::RecordingObserver>();
  const auto second = std::make_shared<cs2511::test::RecordingObserver>();
  publisher.subscribe(first);
  publisher.subscribe(second);

  const cs2511::OrderAccepted event{"1001", "cust-9", cs2511::Money{720}};
  publisher.publish(event);

  REQUIRE(first->events.size() == 1);
  REQUIRE(second->events.size() == 1);
  REQUIRE(first->events.front().orderId == "1001");
  REQUIRE(first->events.front().customerId == "cust-9");
  REQUIRE(first->events.front().total.cents() == 720);
  REQUIRE(second->events.front().total == event.total);
}

TEST_CASE("publishing with no subscribers is a no-op") {
  cs2511::DomainEventPublisher publisher;
  publisher.publish(cs2511::OrderAccepted{"1001", "cust-9", cs2511::Money{1}});
}

TEST_CASE("unsubscribe stops further delivery") {
  cs2511::DomainEventPublisher publisher;
  const auto first = std::make_shared<cs2511::test::RecordingObserver>();
  const auto second = std::make_shared<cs2511::test::RecordingObserver>();
  publisher.subscribe(first);
  publisher.subscribe(second);
  publisher.unsubscribe(first.get());

  publisher.publish(cs2511::OrderAccepted{"1002", "cust-9", cs2511::Money{10}});

  REQUIRE(first->events.empty());
  REQUIRE(second->events.size() == 1);
  REQUIRE(second->events.front().orderId == "1002");
}
