#include "cs2511/notifications/SmsNotificationChannel.hpp"

namespace cs2511 {

Result<Unit> SmsNotificationChannel::send(const Notification& notification) {
  log_.append(DeliveredMessage{"sms", notification.recipient, notification.body});
  return Result<Unit>::success(Unit{});
}

}  // namespace cs2511
