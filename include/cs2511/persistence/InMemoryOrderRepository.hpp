#pragma once

#include "cs2511/persistence/IOrderRepository.hpp"

#include <vector>

namespace cs2511 {

class InMemoryOrderRepository final : public IOrderRepository {
 public:
  Result<Unit> save(const StoredOrder& order) override;
  [[nodiscard]] std::optional<StoredOrder> findById(const std::string& id) const override;

  [[nodiscard]] const std::vector<StoredOrder>& orders() const noexcept { return orders_; }

 private:
  std::vector<StoredOrder> orders_;
};

}  // namespace cs2511
