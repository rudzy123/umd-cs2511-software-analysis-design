#include "support/Samples.hpp"

#include "cs2511/validation/OrderValidator.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("a complete email order is valid") {
  const cs2511::OrderValidator validator;
  const cs2511::Result<cs2511::Unit> result = validator.validate(cs2511::test::validEmailOrder());
  REQUIRE(result.ok());
}

TEST_CASE("a complete sms order is valid") {
  const cs2511::OrderValidator validator;
  cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  request.channel = cs2511::ChannelKind::Sms;
  const cs2511::Result<cs2511::Unit> result = validator.validate(request);
  REQUIRE(result.ok());
}

TEST_CASE("zero prices are allowed") {
  const cs2511::OrderValidator validator;
  cs2511::OrderRequest request = cs2511::test::validEmailOrder();
  request.lines = {cs2511::OrderLine{"SKU-FREE", 1, cs2511::Money{0}}};
  REQUIRE(validator.validate(request).ok());
}

TEST_CASE("validation reports the first broken rule") {
  const cs2511::OrderValidator validator;

  SECTION("missing order id") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.orderId.clear();
    request.customerId.clear();
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE_FALSE(result.ok());
    REQUIRE(result.error().code == cs2511::ErrorCode::InvalidOrder);
    REQUIRE(result.error().message == "order id is required");
  }

  SECTION("missing customer id") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.customerId.clear();
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE(result.error().message == "customer id is required");
  }

  SECTION("missing lines") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.lines.clear();
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE(result.error().message == "order must contain at least one line");
  }

  SECTION("missing sku") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.lines.front().sku.clear();
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE(result.error().message == "sku is required");
  }

  SECTION("non positive quantity") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.lines.front().quantity = 0;
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE(result.error().message == "quantity must be positive");
  }

  SECTION("negative unit price") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.lines.front().unitPrice = cs2511::Money{-1};
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE(result.error().message == "unit price cannot be negative");
  }
}

TEST_CASE("contact fields are required for the selected channel") {
  const cs2511::OrderValidator validator;

  SECTION("email must contain one at-sign with text on both sides") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.email = "ada.example.com";
    REQUIRE(validator.validate(request).error().message ==
            "email channel requires an email address");
    request.email = "@example.com";
    REQUIRE(validator.validate(request).error().message ==
            "email channel requires an email address");
    request.email = "ada@";
    REQUIRE(validator.validate(request).error().message ==
            "email channel requires an email address");
    request.email = "ada@@example.com";
    REQUIRE(validator.validate(request).error().message ==
            "email channel requires an email address");
    request.email = "ada @example.com";
    REQUIRE(validator.validate(request).error().message ==
            "email channel requires an email address");
    request.email = "a@b";
    REQUIRE(validator.validate(request).ok());
  }

  SECTION("sms requires at least seven digits") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.channel = cs2511::ChannelKind::Sms;
    request.phone = "123456";
    REQUIRE(validator.validate(request).error().message ==
            "sms channel requires a phone number");
    request.phone = "123456a";
    REQUIRE(validator.validate(request).error().message ==
            "sms channel requires a phone number");
    request.phone = "1234567";
    REQUIRE(validator.validate(request).ok());
  }

  SECTION("an unknown channel value is rejected") {
    cs2511::OrderRequest request = cs2511::test::validEmailOrder();
    request.channel = static_cast<cs2511::ChannelKind>(42);
    const cs2511::Result<cs2511::Unit> result = validator.validate(request);
    REQUIRE(result.error().code == cs2511::ErrorCode::InvalidOrder);
    REQUIRE(result.error().message == "notification channel is not recognized");
  }
}
