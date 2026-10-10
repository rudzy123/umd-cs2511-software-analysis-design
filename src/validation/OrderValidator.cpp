#include "cs2511/validation/OrderValidator.hpp"

#include <cctype>
#include <string_view>

namespace cs2511 {
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

Result<Unit> invalid(const char* message) {
  return Result<Unit>::failure(Error{ErrorCode::InvalidOrder, message});
}

}  // namespace

Result<Unit> OrderValidator::validate(const OrderRequest& request) const {
  if (request.orderId.empty()) {
    return invalid("order id is required");
  }
  if (request.customerId.empty()) {
    return invalid("customer id is required");
  }
  if (request.lines.empty()) {
    return invalid("order must contain at least one line");
  }

  for (const OrderLine& line : request.lines) {
    if (line.sku.empty()) {
      return invalid("sku is required");
    }
    if (line.quantity <= 0) {
      return invalid("quantity must be positive");
    }
    if (line.unitPrice.cents() < 0) {
      return invalid("unit price cannot be negative");
    }
  }

  if (request.channel == ChannelKind::Email) {
    if (!isValidEmail(request.email)) {
      return invalid("email channel requires an email address");
    }
  } else if (request.channel == ChannelKind::Sms) {
    if (!isValidPhone(request.phone)) {
      return invalid("sms channel requires a phone number");
    }
  } else {
    return invalid("notification channel is not recognized");
  }

  return Result<Unit>::success(Unit{});
}

}  // namespace cs2511
