#include <cassert>
#include <iostream>

#include "../firmware/common/alert_policy.h"

int main() {
  AlertPolicy<8> policy(42);
  AlertEvent event{};

  assert(!policy.observe(AlertState::STOP, 1000));
  assert(!policy.observe(AlertState::STOP, 2000));
  assert(policy.queued() == 0);

  assert(policy.observe(AlertState::RUN, 3000));
  assert(!policy.observe(AlertState::RUN, 4000));
  assert(policy.queued() == 1);
  assert(policy.nextSendable(4000, 30000, event));
  assert(event.bootId == 42 && event.sequence == 1);
  assert(event.from == AlertState::STOP && event.to == AlertState::RUN);
  assert(!policy.markDelivered(999, 5000));
  assert(policy.markDelivered(event.sequence, 5000));

  assert(policy.observe(AlertState::STOP, 6000));
  assert(!policy.nextSendable(34999, 30000, event));
  assert(policy.nextSendable(35000, 30000, event));
  assert(event.sequence == 2 && event.to == AlertState::STOP);
  assert(policy.markDelivered(event.sequence, 35000));

  AlertPolicy<8> offline(7);
  assert(!offline.observe(AlertState::STOP, 0));
  AlertState next = AlertState::RUN;
  for (uint32_t i = 1; i <= 10; ++i) {
    assert(offline.observe(next, i * 1000));
    next = next == AlertState::RUN ? AlertState::STOP : AlertState::RUN;
  }
  assert(offline.queued() == 8);
  assert(offline.dropped() == 2);
  assert(offline.nextSendable(20000, 0, event));
  assert(event.sequence == 3);

  std::cout << "Stage 11 alert policy tests: PASS\n";
  return 0;
}
