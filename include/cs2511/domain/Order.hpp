#pragma once

#include "cs2511/domain/Money.hpp"

#include <string>
#include <vector>

namespace cs2511 {

enum class PricingPlan { Standard, Bulk, Premium };

enum class ChannelKind { Email, Sms };

struct OrderLine {
  std::string sku;
  int quantity{0};
  Money unitPrice{};
};

// Input to the place-order use case. This is a request, not a stored row.
struct OrderRequest {
  std::string orderId;
  std::string customerId;
  std::string email;
  std::string phone;
  std::vector<OrderLine> lines;
  PricingPlan plan{PricingPlan::Standard};
  ChannelKind channel{ChannelKind::Email};
};

// What the repository stores after pricing. Contact fields stay on the request
// because the repository's job is the priced order, not message delivery.
struct StoredOrder {
  std::string id;
  std::string customerId;
  Money total{};
  PricingPlan plan{PricingPlan::Standard};
  ChannelKind channel{ChannelKind::Email};
};

struct OrderReceipt {
  std::string orderId;
  Money total{};
  std::string strategyName;
  std::string channelName;
};

}  // namespace cs2511
