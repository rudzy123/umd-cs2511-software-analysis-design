#pragma once

#include "cs2511/notifications/INotificationChannel.hpp"
#include "cs2511/notifications/NotificationLog.hpp"

namespace cs2511 {

class EmailNotificationChannel final : public INotificationChannel {
 public:
  explicit EmailNotificationChannel(NotificationLog& log) : log_(log) {}

  Result<Unit> send(const Notification& notification) override;
  [[nodiscard]] std::string_view name() const noexcept override { return "email"; }

 private:
  NotificationLog& log_;
};

}  // namespace cs2511
