#pragma once

#include "cs2511/domain/Order.hpp"

namespace cs2511::test {

inline OrderRequest validEmailOrder() {
  OrderRequest request;
  request.orderId = "1001";
  request.customerId = "cust-9";
  request.email = "ada@example.com";
  request.phone = "5550100";
  request.lines.push_back(OrderLine{"SKU-1", 2, Money{500}});
  request.lines.push_back(OrderLine{"SKU-2", 1, Money{250}});
  request.plan = PricingPlan::Standard;
  request.channel = ChannelKind::Email;
  return request;
}

inline OrderRequest bulkEmailOrder() {
  OrderRequest request = validEmailOrder();
  request.lines.clear();
  request.lines.push_back(OrderLine{"SKU-1", 6, Money{100}});
  request.lines.push_back(OrderLine{"SKU-2", 4, Money{50}});
  request.plan = PricingPlan::Bulk;
  return request;
}

}  // namespace cs2511::test
