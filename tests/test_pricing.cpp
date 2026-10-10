#include "cs2511/pricing/BulkPricingStrategy.hpp"
#include "cs2511/pricing/PremiumPricingStrategy.hpp"
#include "cs2511/pricing/PricingStrategyFactory.hpp"
#include "cs2511/pricing/StandardPricingStrategy.hpp"

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<cs2511::OrderLine> linesOf(
    std::initializer_list<std::pair<int, int>> quantityAndPrice) {
  std::vector<cs2511::OrderLine> lines;
  int index = 1;
  for (const std::pair<int, int>& entry : quantityAndPrice) {
    lines.push_back(cs2511::OrderLine{"SKU-" + std::to_string(index), entry.first,
                                       cs2511::Money{entry.second}});
    ++index;
  }
  return lines;
}

}  // namespace

TEST_CASE("standard pricing is the sum of line extensions") {
  const cs2511::StandardPricingStrategy strategy;
  const cs2511::Money total = strategy.quote(linesOf({{2, 500}, {1, 250}}));
  REQUIRE(total.cents() == 1250);
  REQUIRE(strategy.name() == "standard");
}

TEST_CASE("bulk pricing discounts orders that meet the quantity threshold") {
  const cs2511::BulkPricingStrategy strategy;
  REQUIRE(strategy.quote(linesOf({{10, 100}})).cents() == 900);
  REQUIRE(strategy.quote(linesOf({{6, 100}, {4, 50}})).cents() == 720);
  REQUIRE(strategy.quote(linesOf({{11, 100}})).cents() == 990);
}

TEST_CASE("bulk pricing leaves smaller orders unchanged") {
  const cs2511::BulkPricingStrategy strategy;
  REQUIRE(strategy.quote(linesOf({{9, 100}})).cents() == 900);
}

TEST_CASE("premium pricing always applies five percent off") {
  const cs2511::PremiumPricingStrategy strategy;
  REQUIRE(strategy.quote(linesOf({{2, 1000}})).cents() == 1900);
  REQUIRE(strategy.quote(linesOf({{1, 100}})).cents() == 95);
  REQUIRE(strategy.name() == "premium");
}

TEST_CASE("discount rounding truncates toward zero") {
  const cs2511::PremiumPricingStrategy strategy;
  REQUIRE(strategy.quote(linesOf({{1, 199}})).cents() == 189);
}

TEST_CASE("an empty line list quotes zero") {
  const cs2511::StandardPricingStrategy strategy;
  REQUIRE(strategy.quote({}).cents() == 0);
}

TEST_CASE("factory selects a strategy by plan") {
  const cs2511::PricingStrategyFactory factory;
  const std::vector<cs2511::OrderLine> lines = linesOf({{1, 1000}});

  const auto standard = factory.create(cs2511::PricingPlan::Standard);
  const auto bulk = factory.create(cs2511::PricingPlan::Bulk);
  const auto premium = factory.create(cs2511::PricingPlan::Premium);
  REQUIRE(standard.ok());
  REQUIRE(bulk.ok());
  REQUIRE(premium.ok());
  REQUIRE(standard.value()->name() == "standard");
  REQUIRE(bulk.value()->name() == "bulk");
  REQUIRE(premium.value()->name() == "premium");
  REQUIRE(standard.value()->quote(lines).cents() == 1000);
  REQUIRE(premium.value()->quote(lines).cents() == 950);
}

TEST_CASE("an unrecognized plan fails closed") {
  const cs2511::PricingStrategyFactory factory;
  const auto strategy = factory.create(static_cast<cs2511::PricingPlan>(99));
  REQUIRE_FALSE(strategy.ok());
  REQUIRE(strategy.error().code == cs2511::ErrorCode::UnknownPricingPlan);
  REQUIRE(strategy.error().message == "pricing plan is not recognized");
}

TEST_CASE("pricing strategies honor the same contract") {
  const std::vector<cs2511::OrderLine> lines = linesOf({{1, 1000}});
  const std::vector<std::unique_ptr<cs2511::IPricingStrategy>> strategies = [] {
    std::vector<std::unique_ptr<cs2511::IPricingStrategy>> created;
    created.push_back(std::make_unique<cs2511::StandardPricingStrategy>());
    created.push_back(std::make_unique<cs2511::BulkPricingStrategy>());
    created.push_back(std::make_unique<cs2511::PremiumPricingStrategy>());
    return created;
  }();

  for (const std::unique_ptr<cs2511::IPricingStrategy>& strategy : strategies) {
    REQUIRE_FALSE(strategy->name().empty());
    REQUIRE(strategy->quote(lines).cents() >= 0);
    REQUIRE(strategy->quote(lines).cents() <= 1000);
  }
}
