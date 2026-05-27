#include "battery_lightbar.h"

#include <cstdint>
#include <cstdio>

#include "config.h"
#include "state_mgr.h"
#include "pico/time.h"

extern uint8_t interrupt_in_data[63];

namespace {
constexpr uint8_t kBatteryByteIndex = 52;
constexpr uint8_t kMaxPowerPercent = 10; // DualSense packs 0..10 in low nibble.
constexpr uint8_t kUnknownLevel = 0xFF;

uint8_t last_level = kUnknownLevel;
uint8_t last_brightness = 0;
bool last_enabled = false;
uint64_t last_debug_print_us = 0;
constexpr uint64_t kDebugPrintPeriodUs = 2'000'000;

uint8_t scale_color(const uint16_t value, const uint8_t brightness_percent) {
    return static_cast<uint8_t>((value * brightness_percent) / 100);
}
} // namespace

void battery_lightbar_note_report() {
    const auto &cfg = get_config();
    if (!cfg.battery_lightbar) {
        last_level = kUnknownLevel;
        last_brightness = 0;
        last_enabled = false;
        return;
    }

    uint8_t level = interrupt_in_data[kBatteryByteIndex] & 0x0F;
    const uint8_t power_raw = interrupt_in_data[kBatteryByteIndex];
    const uint8_t power_state = (power_raw >> 4) & 0x0F;
    if (level > kMaxPowerPercent) {
        level = kMaxPowerPercent;
    }

    const uint8_t brightness = cfg.lightbar_brightness;
    if (last_enabled && level == last_level && brightness == last_brightness) {
        return;
    }

    // 0% -> red, 100% -> green, linear interpolation without blue channel.
    const uint16_t red_raw = static_cast<uint16_t>(255 * (kMaxPowerPercent - level) / kMaxPowerPercent);
    const uint16_t green_raw = static_cast<uint16_t>(255 * level / kMaxPowerPercent);

    state_set_led_rgb(
        scale_color(red_raw, brightness),
        scale_color(green_raw, brightness),
        0
    );
    state_send_output();

    const uint64_t now = time_us_64();
    if ((now - last_debug_print_us) >= kDebugPrintPeriodUs || level != last_level) {
        // Debug battery parsing and LED mapping from incoming 0x31 report payload.
        printf(
            "[BattLightbar] raw52=0x%02X raw53=0x%02X state=%u level=%u/%u brightness=%u rgb=(%u,%u,%u)\n",
            power_raw,
            interrupt_in_data[53],
            static_cast<unsigned>(power_state),
            static_cast<unsigned>(level),
            static_cast<unsigned>(kMaxPowerPercent),
            static_cast<unsigned>(brightness),
            static_cast<unsigned>(scale_color(red_raw, brightness)),
            static_cast<unsigned>(scale_color(green_raw, brightness)),
            0u
        );
        last_debug_print_us = now;
    }

    last_level = level;
    last_brightness = brightness;
    last_enabled = true;
}

void battery_lightbar_on_disconnect() {
    last_level = kUnknownLevel;
    last_brightness = 0;
    last_enabled = false;
    last_debug_print_us = 0;
}
