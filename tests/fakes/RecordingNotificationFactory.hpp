#pragma once

#include "cs2511/notifications/INotificationChannelFactory.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cs2511::test {

class RecordingChannel final : public INotificationChannel {
 public:
  RecordingChannel(std::string name, std::vector<Notification>& sent, bool& failNext)
      : name_(std::move(name)), sent_(sent), failNext_(failNext) {}

  Result<Unit> send(const Notification& notification) override {
    if (failNext_) {
      failNext_ = false;
      return Result<Unit>::failure(
          Error{ErrorCode::NotificationFailed, "failed to send notification"});
    }
    sent_.push_back(notification);
    return Result<Unit>::success(Unit{});
  }

  [[nodiscard]] std::string_view name() const noexcept override { return name_; }

 private:
  std::string name_;
  std::vector<Notification>& sent_;
  bool& failNext_;
};

class RecordingNotificationFactory final : public INotificationChannelFactory {
 public:
  [[nodiscard]] Result<std::unique_ptr<INotificationChannel>> create(
      ChannelKind kind) const override {
    std::string channelName;
    switch (kind) {
      case ChannelKind::Email:
        channelName = "email";
        break;
      case ChannelKind::Sms:
        channelName = "sms";
        break;
    }
    if (channelName.empty()) {
      return Result<std::unique_ptr<INotificationChannel>>::failure(
          Error{ErrorCode::UnknownChannel, "notification channel is not recognized"});
    }
    return Result<std::unique_ptr<INotificationChannel>>::success(
        std::make_unique<RecordingChannel>(std::move(channelName), sent_, failNext_));
  }

  mutable bool failNext_{false};
  mutable std::vector<Notification> sent_;
};

}  // namespace cs2511::test
