#include "cs2511/notifications/NotificationChannelFactory.hpp"

#include "cs2511/notifications/EmailNotificationChannel.hpp"
#include "cs2511/notifications/SmsNotificationChannel.hpp"

#include <memory>

namespace cs2511 {

Result<std::unique_ptr<INotificationChannel>> NotificationChannelFactory::create(
    ChannelKind kind) const {
  switch (kind) {
    case ChannelKind::Email:
      return Result<std::unique_ptr<INotificationChannel>>::success(
          std::make_unique<EmailNotificationChannel>(log_));
    case ChannelKind::Sms:
      return Result<std::unique_ptr<INotificationChannel>>::success(
          std::make_unique<SmsNotificationChannel>(log_));
  }

  return Result<std::unique_ptr<INotificationChannel>>::failure(
      Error{ErrorCode::UnknownChannel, "notification channel is not recognized"});
}

}  // namespace cs2511
