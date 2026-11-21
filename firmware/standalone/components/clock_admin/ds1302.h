#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <optional>
#include <sys/time.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esphome/components/time/posix_tz.h"
#include "esphome/core/time.h"

namespace esphome::clock_admin::rtc {

inline constexpr gpio_num_t CE = GPIO_NUM_27;
inline constexpr gpio_num_t IO = GPIO_NUM_26;
inline constexpr gpio_num_t SCLK = GPIO_NUM_23;
inline constexpr uint8_t WRITE_PROTECT = 0x8E;
inline constexpr uint8_t TRICKLE_CHARGER = 0x90;
inline constexpr uint8_t CLOCK_BURST = 0xBE;
inline constexpr uint8_t VALIDITY_RAM = 0xC0;
inline constexpr uint8_t TIME_INITIALIZED = 0xA7;
inline bool ready = false;
inline const char *time_source = "Waiting for time";
inline time_t last_network_sync = 0;

inline void pause_bus() { esp_rom_delay_us(5); }

inline void begin() {
  gpio_set_level(SCLK, 0);
  gpio_set_direction(IO, GPIO_MODE_OUTPUT);
  gpio_set_level(CE, 1);
  pause_bus();
}

inline void end() {
  gpio_set_level(CE, 0);
  gpio_set_direction(IO, GPIO_MODE_INPUT);
  pause_bus();
}

inline void send_byte(uint8_t value, bool read_follows = false) {
  for (int bit = 0; bit < 8; ++bit) {
    gpio_set_level(IO, (value >> bit) & 1);
    pause_bus();
    gpio_set_level(SCLK, 1);
    pause_bus();
    // The RTC starts driving bit zero on the last command falling edge.
    if (read_follows && bit == 7) gpio_set_direction(IO, GPIO_MODE_INPUT);
    gpio_set_level(SCLK, 0);
    pause_bus();
  }
}

inline uint8_t receive_byte() {
  uint8_t value = 0;
  for (int bit = 0; bit < 8; ++bit) {
    value |= gpio_get_level(IO) << bit;
    gpio_set_level(SCLK, 1);
    pause_bus();
    gpio_set_level(SCLK, 0);
    pause_bus();
  }
  return value;
}

inline uint8_t read_register(uint8_t address) {
  begin();
  send_byte(address | 1, true);
  const auto value = receive_byte();
  end();
  return value;
}

inline void write_register(uint8_t address, uint8_t value) {
  begin();
  send_byte(address);
  send_byte(value);
  end();
}

inline bool setup() {
  gpio_set_level(CE, 0);
  gpio_set_level(SCLK, 0);
  gpio_set_direction(CE, GPIO_MODE_OUTPUT);
  gpio_set_direction(SCLK, GPIO_MODE_OUTPUT);
  gpio_set_direction(IO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(IO, GPIO_PULLDOWN_ONLY);

  // Probe the write-protect bit, then explicitly disable battery charging.
  write_register(WRITE_PROTECT, 0x80);
  if (read_register(WRITE_PROTECT) != 0x80) return false;
  write_register(WRITE_PROTECT, 0);
  write_register(TRICKLE_CHARGER, 0);
  write_register(WRITE_PROTECT, 0x80);
  ready = true;
  return true;
}

inline int decode_bcd(uint8_t value) {
  return (value >> 4) <= 9 && (value & 15) <= 9 ? (value >> 4) * 10 + (value & 15) : -1;
}

inline uint8_t encode_bcd(int value) { return ((value / 10) << 4) | (value % 10); }

inline std::optional<time_t> read_epoch() {
  if (!ready || read_register(VALIDITY_RAM) != TIME_INITIALIZED) return std::nullopt;
  std::array<uint8_t, 8> bytes{};
  begin();
  send_byte(CLOCK_BURST | 1, true);
  for (auto &byte : bytes) byte = receive_byte();
  end();
  if ((bytes[0] & 0x80) || (bytes[2] & 0xC0)) return std::nullopt;  // Halted or not in 24-hour mode

  const int second = decode_bcd(bytes[0]);
  const int minute = decode_bcd(bytes[1]);
  const int hour = decode_bcd(bytes[2]);
  const int day = decode_bcd(bytes[3]);
  const int month = decode_bcd(bytes[4]);
  const int year = decode_bcd(bytes[6]) + 2000;
  if (second < 0 || second > 59 || minute < 0 || minute > 59 || hour < 0 || hour > 23 ||
      year < 2019 || month < 1 || month > 12 || day < 1 || day > time::internal::days_in_month(year, month))
    return std::nullopt;

  ESPTime utc{};
  utc.year = year;
  utc.month = month;
  utc.day_of_month = day;
  utc.hour = hour;
  utc.minute = minute;
  utc.second = second;
  utc.recalc_timestamp_utc(false);
  return utc.timestamp;
}

inline std::string health_status() {
  static std::optional<time_t> previous;
  if (!ready || read_register(WRITE_PROTECT) != 0x80) {
    previous.reset();
    return "RTC not responding";
  }
  if (read_register(VALIDITY_RAM) != TIME_INITIALIZED) {
    previous.reset();
    return "RTC needs a manual time setting or network sync";
  }
  const auto epoch = read_epoch();
  if (!epoch.has_value()) {
    previous.reset();
    return "RTC stopped or returned an invalid date";
  }

  const char *state = !previous ? "Checking" : *epoch == *previous ? "Not advancing" :
                      *epoch < *previous ? "Time moved backward" : "Advancing";
  previous = epoch;
  const auto utc = ESPTime::from_epoch_utc(*epoch);
  const auto system_epoch = ::time(nullptr);
  char status[112];
  snprintf(status, sizeof(status), "%s | %04u-%02u-%02u %02u:%02u:%02u UTC | %+lld s vs clock",
           state, utc.year, utc.month, utc.day_of_month, utc.hour, utc.minute, utc.second,
           static_cast<long long>(*epoch - system_epoch));
  return status;
}

inline void write_epoch(time_t epoch) {
  if (!ready) return;
  const auto utc = ESPTime::from_epoch_utc(epoch);
  if (utc.year < 2019 || utc.year > 2099) return;
  write_register(WRITE_PROTECT, 0);
  begin();
  send_byte(CLOCK_BURST);
  for (const uint8_t byte : {encode_bcd(utc.second), encode_bcd(utc.minute), encode_bcd(utc.hour),
                            encode_bcd(utc.day_of_month), encode_bcd(utc.month), encode_bcd(utc.day_of_week),
                            encode_bcd(utc.year - 2000), uint8_t{0}})
    send_byte(byte);
  end();
  write_register(VALIDITY_RAM, TIME_INITIALIZED);
  write_register(WRITE_PROTECT, 0x80);
}

inline void forget_time() {
  if (!ready) return;
  write_register(WRITE_PROTECT, 0);
  write_register(VALIDITY_RAM, 0);
  write_register(WRITE_PROTECT, 0x80);
}

inline void restore_at_boot() {
  if (!setup()) {
    ESP_LOGI("ds1302", "RTC not detected");
    return;
  }
  if (const auto epoch = read_epoch()) {
    const timeval stored{*epoch, 0};
    settimeofday(&stored, nullptr);
    time_source = "DS1302 battery-backed clock";
    ESP_LOGI("ds1302", "Restored battery-backed clock time");
  } else {
    ESP_LOGI("ds1302", "RTC needs a manual time setting or network sync");
  }
}

}  // namespace esphome::clock_admin::rtc
