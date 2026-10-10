#include "cs2511/notifications/EmailNotificationChannel.hpp"

namespace cs2511 {

Result<Unit> EmailNotificationChannel::send(const Notification& notification) {
  log_.append(DeliveredMessage{"email", notification.recipient, notification.body});
  return Result<Unit>::success(Unit{});
}

}  // namespace cs2511
