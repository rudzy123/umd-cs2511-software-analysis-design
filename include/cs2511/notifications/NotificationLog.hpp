#pragma once

#include "cs2511/notifications/NotificationText.hpp"

#include <utility>
#include <vector>

namespace cs2511 {

class NotificationLog {
 public:
  void append(DeliveredMessage message) { messages_.push_back(std::move(message)); }

  [[nodiscard]] const std::vector<DeliveredMessage>& messages() const noexcept {
    return messages_;
  }

 private:
  std::vector<DeliveredMessage> messages_;
};

}  // namespace cs2511
