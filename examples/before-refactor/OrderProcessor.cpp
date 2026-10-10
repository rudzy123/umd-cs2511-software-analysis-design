#include "OrderProcessor.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <tuple>

namespace cs2511::before {
namespace {

bool isValidEmail(std::string_view email) {
  if (email.find(' ') != std::string_view::npos) {
    return false;
  }
  const std::size_t at = email.find('@');
  if (at == std::string_view::npos || at == 0 || at + 1 == email.size()) {
    return false;
  }
  return email.find('@', at + 1) == std::string_view::npos;
}

bool isValidPhone(std::string_view phone) {
  if (phone.size() < 7) {
    return false;
  }
  for (char ch : phone) {
    if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
      return false;
    }
  }
  return true;
}

}  // namespace

int OrderProcessor::process(const std::string& orderId,
                            const std::string& customerId,
                            const std::string& email,
                            const std::string& phone,
                            const std::vector<std::tuple<std::string, int, int>>& items,
                            const std::string& plan,
                            const std::string& channel) {
  const auto reject = [this](int code, const char* message) -> int {
    lastCode_ = code;
    lastError_ = message;
    return code;
  };

  // Validate, price, save, publish, and notify — one method, every reason.
  if (orderId.empty()) {
    return reject(1, "order id is required");
  }
  if (customerId.empty()) {
    return reject(1, "customer id is required");
  }
  if (items.empty()) {
    return reject(1, "order must contain at least one line");
  }
  for (const std::tuple<std::string, int, int>& item : items) {
    if (std::get<0>(item).empty()) {
      return reject(1, "sku is required");
    }
    if (std::get<1>(item) <= 0) {
      return reject(1, "quantity must be positive");
    }
    if (std::get<2>(item) < 0) {
      return reject(1, "unit price cannot be negative");
    }
  }
  if (channel == "email") {
    if (!isValidEmail(email)) {
      return reject(1, "email channel requires an email address");
    }
  } else if (channel == "sms") {
    if (!isValidPhone(phone)) {
      return reject(1, "sms channel requires a phone number");
    }
  } else {
    return reject(1, "notification channel is not recognized");
  }

  int totalCents = 0;
  int quantityCount = 0;
  for (const std::tuple<std::string, int, int>& item : items) {
    totalCents += std::get<1>(item) * std::get<2>(item);
    quantityCount += std::get<1>(item);
  }

  if (plan == "bulk") {
    if (quantityCount >= 10) {
      totalCents = totalCents * 90 / 100;
    }
  } else if (plan == "premium") {
    totalCents = totalCents * 95 / 100;
  } else if (plan == "standard") {
    totalCents = totalCents * 100 / 100;
  } else {
    return reject(2, "pricing plan is not recognized");
  }
  lastTotalCents_ = totalCents;

  for (const std::string& savedId : savedIds_) {
    if (savedId == orderId) {
      return reject(3, "order id already exists");
    }
  }
  if (failNextSave_) {
    failNextSave_ = false;
    return reject(3, "failed to save order");
  }

  savedIds_.push_back(orderId);
  savedTotals_.push_back(totalCents);
  events_.push_back("OrderAccepted id=" + orderId + " customer=" + customerId +
                    " totalCents=" + std::to_string(totalCents));

  if (failNextNotification_) {
    failNextNotification_ = false;
    return reject(4, "failed to send notification");
  }

  const std::string recipient = channel == "email" ? email : phone;
  const std::string body = "Order accepted: id=" + orderId + " customer=" + customerId +
                           " totalCents=" + std::to_string(totalCents);
  notifications_.push_back("[" + channel + "] to=" + recipient + " | " + body);
  lastCode_ = 0;
  lastError_.clear();
  return 0;
}

}  // namespace cs2511::before
