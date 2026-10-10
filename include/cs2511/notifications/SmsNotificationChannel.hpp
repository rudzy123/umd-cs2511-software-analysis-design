#pragma once

#include "cs2511/notifications/INotificationChannel.hpp"
#include "cs2511/notifications/NotificationLog.hpp"

namespace cs2511 {

class SmsNotificationChannel final : public INotificationChannel {
 public:
  explicit SmsNotificationChannel(NotificationLog& log) : log_(log) {}

  Result<Unit> send(const Notification& notification) override;
  [[nodiscard]] std::string_view name() const noexcept override { return "sms"; }

 private:
  NotificationLog& log_;
};

}  // namespace cs2511
