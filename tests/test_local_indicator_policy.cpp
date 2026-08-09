#include <cassert>
#include <iostream>

#include "../firmware/common/local_indicator_policy.h"

int main() {
  LocalOutputs output = localOutputsFor(AlertState::UNKNOWN, true, false);
  assert(output.statusLed == IndicatorPattern::SLOW_BLINK);
  assert(output.buzzer == BuzzerPattern::OFF);

  output = localOutputsFor(AlertState::STOP, true, false);
  assert(output.statusLed == IndicatorPattern::OFF);
  assert(output.buzzer == BuzzerPattern::OFF);

  output = localOutputsFor(AlertState::RUN, true, false);
  assert(output.statusLed == IndicatorPattern::SOLID);
  assert(output.buzzer == BuzzerPattern::OFF);

  output = localOutputsFor(AlertState::RUN, false, false);
  assert(output.statusLed == IndicatorPattern::FAST_BLINK);
  assert(output.buzzer == BuzzerPattern::FAULT_PULSE);

  output = localOutputsFor(AlertState::SENSOR_FAULT, true, true);
  assert(output.statusLed == IndicatorPattern::FAST_BLINK);
  assert(output.buzzer == BuzzerPattern::OFF);

  std::cout << "Stage 12 local indicator policy tests: PASS\n";
  return 0;
}
