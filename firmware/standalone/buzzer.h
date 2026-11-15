#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"

namespace clock_buzzer {

inline void stop(gpio_num_t pin) {
  gpio_reset_pin(pin);
  gpio_set_direction(pin, GPIO_MODE_INPUT);
  gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

inline void start(gpio_num_t pin, uint32_t frequency, uint32_t volume) {
  if (volume == 0) return;
  if (frequency != 0) {
    ledc_timer_config_t timer{};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_8_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = frequency;
    timer.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer) != ESP_OK) return;

    ledc_channel_config_t channel{};
    channel.gpio_num = pin;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = LEDC_CHANNEL_0;
    channel.timer_sel = LEDC_TIMER_0;
    channel.duty = 256 - (128 * volume + 50) / 100;
    if (ledc_channel_config(&channel) != ESP_OK) stop(pin);
    return;
  }
  gpio_set_level(pin, 0);
  gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
}

}  // namespace clock_buzzer
