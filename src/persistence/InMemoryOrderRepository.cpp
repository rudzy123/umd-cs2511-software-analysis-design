#include "cs2511/persistence/InMemoryOrderRepository.hpp"

namespace cs2511 {

Result<Unit> InMemoryOrderRepository::save(const StoredOrder& order) {
  if (findById(order.id).has_value()) {
    return Result<Unit>::failure(
        Error{ErrorCode::PersistenceFailed, "order id already exists"});
  }
  orders_.push_back(order);
  return Result<Unit>::success(Unit{});
}

std::optional<StoredOrder> InMemoryOrderRepository::findById(const std::string& id) const {
  for (const StoredOrder& order : orders_) {
    if (order.id == id) {
      return order;
    }
  }
  return std::nullopt;
}

}  // namespace cs2511
