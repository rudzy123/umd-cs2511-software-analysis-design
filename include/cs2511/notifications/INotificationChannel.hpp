#pragma once

#include "cs2511/domain/Result.hpp"
#include "cs2511/notifications/NotificationText.hpp"

#include <string_view>

namespace cs2511 {

class INotificationChannel {
 public:
  virtual ~INotificationChannel() = default;

  virtual Result<Unit> send(const Notification& notification) = 0;
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

}  // namespace cs2511
