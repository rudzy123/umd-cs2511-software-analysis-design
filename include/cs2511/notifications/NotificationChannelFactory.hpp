#pragma once

#include "cs2511/notifications/INotificationChannelFactory.hpp"
#include "cs2511/notifications/NotificationLog.hpp"

namespace cs2511 {

// Factory: builds the channel the request asked for. The service stays free of
// the concrete email and SMS types.
class NotificationChannelFactory final : public INotificationChannelFactory {
 public:
  explicit NotificationChannelFactory(NotificationLog& log) : log_(log) {}

  NotificationChannelFactory(const NotificationChannelFactory&) = delete;
  NotificationChannelFactory& operator=(const NotificationChannelFactory&) = delete;

  [[nodiscard]] Result<std::unique_ptr<INotificationChannel>> create(
      ChannelKind kind) const override;

 private:
  NotificationLog& log_;
};

}  // namespace cs2511
