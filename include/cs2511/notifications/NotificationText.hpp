#pragma once

#include <string>

namespace cs2511 {

struct Notification {
  std::string recipient;
  std::string body;
};

struct DeliveredMessage {
  std::string channel;
  std::string recipient;
  std::string body;
};

inline std::string orderAcceptedText(const std::string& orderId,
                                     const std::string& customerId,
                                     int totalCents) {
  return "Order accepted: id=" + orderId + " customer=" + customerId +
         " totalCents=" + std::to_string(totalCents);
}

inline std::string orderAcceptedEventText(const std::string& orderId,
                                           const std::string& customerId,
                                           int totalCents) {
  return "OrderAccepted id=" + orderId + " customer=" + customerId +
         " totalCents=" + std::to_string(totalCents);
}

inline std::string notificationEnvelope(const std::string& channel,
                                        const std::string& recipient,
                                        const std::string& body) {
  return "[" + channel + "] to=" + recipient + " | " + body;
}

}  // namespace cs2511
