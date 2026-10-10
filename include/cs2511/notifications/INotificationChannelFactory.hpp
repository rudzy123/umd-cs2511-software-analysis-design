#pragma once

#include "cs2511/domain/Order.hpp"
#include "cs2511/domain/Result.hpp"
#include "cs2511/notifications/INotificationChannel.hpp"

#include <memory>

namespace cs2511 {

class INotificationChannelFactory {
 public:
  virtual ~INotificationChannelFactory() = default;

  [[nodiscard]] virtual Result<std::unique_ptr<INotificationChannel>> create(
      ChannelKind kind) const = 0;
};

}  // namespace cs2511
