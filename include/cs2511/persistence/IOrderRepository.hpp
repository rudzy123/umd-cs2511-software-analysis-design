#pragma once

#include "cs2511/domain/Order.hpp"
#include "cs2511/domain/Result.hpp"

#include <optional>
#include <string>

namespace cs2511 {

class IOrderRepository {
 public:
  virtual ~IOrderRepository() = default;

  virtual Result<Unit> save(const StoredOrder& order) = 0;

  // Absence is an empty optional. A failed write is a Result error.
  [[nodiscard]] virtual std::optional<StoredOrder> findById(const std::string& id) const = 0;
};

}  // namespace cs2511
