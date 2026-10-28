#pragma once
#include <cstdint>

namespace bthome_c3btn {
// mask bit n -> BTHome button n+1 gets a "press" event
void send_buttons(uint8_t mask);
// stop advertising and power down the BLE controller
void end();
}  // namespace bthome_c3btn