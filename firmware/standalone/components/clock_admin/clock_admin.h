#pragma once

#include <array>

#include "esphome/components/web_server/web_server.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::clock_admin {

class ClockAdmin : public Component, public AsyncWebHandler {
 public:
  ClockAdmin(web_server::WebServer *web_server, datetime::TimeEntity *alarm_time,
             switch_::Switch *alarm_enabled, switch_::Switch *alarm_skip, text_sensor::TextSensor *alarm_status,
             number::Number *alarm_duration, number::Number *buzzer_volume,
             number::Number *buzzer_frequency, button::Button *test_buzzer, button::Button *silence_alarm)
      : web_server_(web_server), alarm_time_(alarm_time), alarm_enabled_(alarm_enabled),
        alarm_skip_(alarm_skip), alarm_status_(alarm_status), alarm_duration_(alarm_duration),
        buzzer_volume_(buzzer_volume), buzzer_frequency_(buzzer_frequency), test_buzzer_(test_buzzer), silence_alarm_(silence_alarm) {}
  float get_setup_priority() const override { return setup_priority::WIFI + 1.0f; }
  void setup() override;
  void apply_network_hostname();
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 private:
  struct Credentials {
    char username[33];
    char password[65];
  };
  web_server::WebServer *web_server_;
  datetime::TimeEntity *alarm_time_;
  switch_::Switch *alarm_enabled_;
  switch_::Switch *alarm_skip_;
  text_sensor::TextSensor *alarm_status_;
  number::Number *alarm_duration_;
  number::Number *buzzer_volume_;
  number::Number *buzzer_frequency_;
  button::Button *test_buzzer_;
  button::Button *silence_alarm_;
  ESPPreferenceObject preference_;
  Credentials credentials_{};
  ESPPreferenceObject timezone_preference_;
  using SavedHostname = std::array<char, 31>;
  ESPPreferenceObject hostname_preference_;
  SavedHostname hostname_{};
};

}  // namespace esphome::clock_admin
