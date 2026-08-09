#pragma once

#include <stddef.h>
#include <stdint.h>

enum class AlertState : uint8_t {
  UNKNOWN,
  STOP,
  RUN,
  SENSOR_FAULT,
  OFFLINE
};

struct AlertEvent {
  uint32_t bootId;
  uint32_t sequence;
  uint32_t createdMs;
  AlertState from;
  AlertState to;
};

template <size_t Capacity>
class AlertPolicy {
 public:
  explicit AlertPolicy(uint32_t bootId) : bootId_(bootId) {}

  bool observe(AlertState next, uint32_t nowMs) {
    if (!initialized_) {
      current_ = next;
      initialized_ = true;
      return false;
    }
    if (next == current_) return false;

    AlertEvent event = {bootId_, ++sequence_, nowMs, current_, next};
    current_ = next;
    enqueue(event);
    return true;
  }

  bool nextSendable(uint32_t nowMs, uint32_t minimumIntervalMs,
                    AlertEvent &event) const {
    if (count_ == 0) return false;
    if (hasDelivered_ && static_cast<uint32_t>(nowMs - lastDeliveredMs_) < minimumIntervalMs) {
      return false;
    }
    event = queue_[head_];
    return true;
  }

  bool markDelivered(uint32_t sequence, uint32_t nowMs) {
    if (count_ == 0 || queue_[head_].sequence != sequence) return false;
    head_ = (head_ + 1) % Capacity;
    --count_;
    hasDelivered_ = true;
    lastDeliveredMs_ = nowMs;
    return true;
  }

  AlertState current() const { return current_; }
  size_t queued() const { return count_; }
  uint32_t dropped() const { return dropped_; }

 private:
  void enqueue(const AlertEvent &event) {
    if (count_ == Capacity) {
      head_ = (head_ + 1) % Capacity;
      --count_;
      ++dropped_;
    }
    const size_t tail = (head_ + count_) % Capacity;
    queue_[tail] = event;
    ++count_;
  }

  AlertEvent queue_[Capacity] = {};
  uint32_t bootId_ = 0;
  uint32_t sequence_ = 0;
  uint32_t dropped_ = 0;
  uint32_t lastDeliveredMs_ = 0;
  size_t head_ = 0;
  size_t count_ = 0;
  AlertState current_ = AlertState::UNKNOWN;
  bool initialized_ = false;
  bool hasDelivered_ = false;
};
