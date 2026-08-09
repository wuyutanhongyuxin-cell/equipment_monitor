#pragma once

#include "alert_policy.h"

enum class IndicatorPattern : uint8_t {
  OFF,
  SOLID,
  SLOW_BLINK,
  FAST_BLINK
};

enum class BuzzerPattern : uint8_t {
  OFF,
  FAULT_PULSE
};

struct LocalOutputs {
  IndicatorPattern statusLed;
  BuzzerPattern buzzer;
};

inline LocalOutputs localOutputsFor(AlertState state, bool sensorHealthy, bool muted) {
  const bool fault = !sensorHealthy || state == AlertState::SENSOR_FAULT ||
                     state == AlertState::OFFLINE;
  if (fault) {
    return {IndicatorPattern::FAST_BLINK,
            muted ? BuzzerPattern::OFF : BuzzerPattern::FAULT_PULSE};
  }

  switch (state) {
    case AlertState::RUN:
      return {IndicatorPattern::SOLID, BuzzerPattern::OFF};
    case AlertState::STOP:
      return {IndicatorPattern::OFF, BuzzerPattern::OFF};
    default:
      return {IndicatorPattern::SLOW_BLINK, BuzzerPattern::OFF};
  }
}
