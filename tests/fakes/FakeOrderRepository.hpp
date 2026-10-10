#pragma once

#include "cs2511/persistence/IOrderRepository.hpp"

#include <vector>

namespace cs2511::test {

class FakeOrderRepository final : public IOrderRepository {
 public:
  Result<Unit> save(const StoredOrder& order) override {
    if (failNextSave) {
      failNextSave = false;
      return Result<Unit>::failure(
          Error{ErrorCode::PersistenceFailed, "failed to save order"});
    }
    if (findById(order.id).has_value()) {
      return Result<Unit>::failure(
          Error{ErrorCode::PersistenceFailed, "order id already exists"});
    }
    saved.push_back(order);
    return Result<Unit>::success(Unit{});
  }

  [[nodiscard]] std::optional<StoredOrder> findById(const std::string& id) const override {
    for (const StoredOrder& order : saved) {
      if (order.id == id) {
        return order;
      }
    }
    return std::nullopt;
  }

  bool failNextSave{false};
  std::vector<StoredOrder> saved;
};

}  // namespace cs2511::test
