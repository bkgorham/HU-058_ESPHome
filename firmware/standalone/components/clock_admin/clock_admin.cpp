#include "clock_admin.h"
#include "ds1302.h"

#include <cstring>
#include <sys/time.h>
#include <esp_netif.h>
#include <mdns.h>

#include "esphome/components/time/posix_tz.h"

#include "esphome/components/captive_portal/captive_portal.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/application.h"

namespace esphome::clock_admin {
namespace {

constexpr char PAGE[] = R"html(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Wi-Fi Clock</title>
<style>
html{color-scheme:light dark;--page:#fff;--text:#182230;--link:#174ea6;--field:#fff;--border:#6b7280;--header-start:#edf5fb;--header-end:#e8f4ee}
@media(prefers-color-scheme:dark){
  html{--page:#151922;--text:#f1f5f9;--link:#a8c7ff;--field:#222938;--border:#8895aa;--header-start:#1b2a3b;--header-end:#20332e}
}
body{margin:0;font:16px system-ui,sans-serif;background:var(--page);color:var(--text)}
a{color:var(--link)}
.site-header{background:linear-gradient(110deg,var(--header-start),var(--header-end));border-bottom:1px solid var(--border)}
.site-identity{display:flex;align-items:center;justify-content:space-between;gap:.75rem;padding:1.15rem 1rem .65rem}
.site-title{margin:0;font-size:clamp(1.05rem,4vw,1.75rem);line-height:1.2}
.site-meta{flex-shrink:0;max-width:52%;text-align:right;font-size:clamp(.65rem,2.3vw,.85rem);line-height:1.2;overflow-wrap:anywhere}
.site-meta span{display:block}
nav{display:flex;flex-wrap:wrap;gap:.25rem;padding:0 .65rem}
nav a{padding:.7rem .55rem;text-decoration:none;border-bottom:3px solid transparent}
nav a:hover{text-decoration:underline}
nav a[aria-current]{font-weight:700;border-bottom-color:var(--link)}
h2{margin-top:0;font-size:1.35rem}
form h3{margin-bottom:.5rem;font-size:1.1rem}
iframe{color-scheme:only light;background:#fff;display:block;width:100%;height:70vh;border:0}
main{max-width:64rem;padding:1rem;margin:0}
main form{max-width:36rem}
label{display:grid;gap:.3rem;margin:.9rem 0}
input,select,button{font:inherit;padding:.45rem;background:var(--field);color:var(--text);border:1px solid var(--border)}
form{margin:0 0 2rem}
</style>
<header class="site-header">
<div class="site-identity">
<h1 class="site-title">ESP32 WiFi Clock</h1>
<div class="site-meta">
<span id="clock-hostname"></span>
<span>v)html" ESPHOME_PROJECT_VERSION R"html(</span>
</div>
</div>
<nav aria-label="Clock pages">
<a href="/" id="controls">Controls</a><a href="/alarm" id="alarm-tab">Alarm</a><a href="/admin" id="admin-tab">Administration</a>
</nav>
</header>
<main id="controls-page" hidden>
<h2>Controls</h2>
<iframe title="Clock controls" id="panel"></iframe>
</main>
<main id="alarm" hidden>
<h2>Alarm</h2>
<form action="/alarm/settings" method="post">
<label><span><input type="checkbox" name="enabled"> Enable daily alarm</span></label>
<label>Daily alarm time<input type="time" name="time" required></label>
<label><span><input type="checkbox" name="skip"> Skip next alarm</span></label>
<label>Alarm duration (seconds)<input type="number" name="duration" required min="1" max="300" step="1"></label>
<label>Volume (%)<input type="number" name="volume" required min="0" max="100" step="1"></label>
<label>Tone Frequency (Hz)<input type="number" name="frequency" required min="1000" max="5000" step="100"></label>
<button>Save alarm settings</button>
</form>
<p id="alarm-status" role="status"></p>
<form action="/alarm/test" method="post"><button>Test Buzzer</button></form>
<form action="/alarm/silence" method="post"><button>Silence Alarm</button></form>
</main>
<main id="admin" hidden>
<h2>Administration</h2>
<p>Firmware revision <strong>)html" ESPHOME_PROJECT_VERSION R"html(</strong></p>
<form action="/admin/hostname" method="post">
<h3>Hostname</h3>
<p id="current-hostname"></p>
<label>Hostname<input name="hostname" maxlength="30" pattern="[A-Za-z0-9]+(?:-*[A-Za-z0-9]+)*" placeholder="wifi-clock"></label>
<p>Use up to 30 letters, digits, and hyphens. Leave blank to restore the default MAC-based name.</p>
<button>Save hostname</button>
</form>
<form action="/admin/domain" method="post">
<label>Domain name<input id="clock-domain" name="domain" maxlength="222" placeholder="local" autocomplete="off"></label>
<p>Leave blank to use local. For a custom domain, configure your router or DNS server to point the full name to this clock.</p>
<button>Save domain</button>
</form>
<form action="/admin/timezone" method="post">
<h3>Timezone</h3>
<label>Timezone<select name="zone" required>
<option value="" disabled selected>Choose a timezone</option>
<option value="eastern">Eastern</option>
<option value="central">Central</option>
<option value="mountain">Mountain</option>
<option value="arizona">Arizona</option>
<option value="pacific">Pacific</option>
<option value="alaska">Alaska</option>
<option value="hawaii">Hawaii</option>
<option value="utc">UTC</option>
</select></label>
<button>Save timezone</button>
</form>
<form action="/admin/time" method="post">
<h3>Set Local Date and Time</h3>
<label>Local date and time<input name="local_time" type="datetime-local" required min="2019-01-01T00:00" max="2099-12-31T23:59"></label>
<button>Set time</button>
</form>
<form action="/admin/wifi" method="post">
<h3>Wi-Fi</h3>
<label>Network name (SSID)<input name="ssid" required maxlength="32" autocomplete="off"></label>
<label>Network password<input name="password" type="password" maxlength="64" autocomplete="new-password"></label>
<p>The clock reconnects after saving. If it cannot connect, join its setup hotspot and open <a href="http://192.168.101.1/">192.168.101.1</a>.</p>
<button>Save Wi-Fi</button>
</form>
<form action="/admin/login" method="post">
<h3>Website login</h3>
<label>New username<input name="username" required maxlength="32" pattern="[A-Za-z0-9_.\-]+" autocomplete="username"></label>
<label>New password<input name="password" type="password" required minlength="8" maxlength="64" autocomplete="new-password"></label>
<label>Confirm new password<input name="confirm_password" type="password" required minlength="8" maxlength="64" autocomplete="new-password"></label>
<button>Save login</button>
</form>
<form action="/admin/reset" method="post">
<h3>Factory reset</h3>
<p>Erase saved Wi-Fi, web login, hostname, and all other clock settings. The clock restarts in Wi-Fi setup mode.</p>
<label><span><input type="checkbox" name="confirm" value="yes" required> Erase all saved settings</span></label>
<button>Reset All Settings</button>
</form>
</main>
<script>
const admin = location.pathname === '/admin';
const alarm = location.pathname === '/alarm';
document.querySelector(admin ? '#admin-tab' : alarm ? '#alarm-tab' : '#controls').setAttribute('aria-current', 'page');
fetch('/admin/hostname', {cache: 'no-store'})
  .then(response => response.ok ? response.json() : Promise.reject())
  .then(({hostname, domain, username, ssid, zone, timezone}) => {
    const address = hostname + '.' + domain;
    document.querySelector('#clock-hostname').textContent = address;
    document.title = address + ' · ' + (admin ? 'Administration' : alarm ? 'Alarm' : 'Controls');
    if (admin) {
      document.querySelector('#current-hostname').textContent = 'Current address ' + address;
      document.querySelector('#clock-domain').value = domain;
      document.querySelector('input[name="hostname"]').value = hostname;
      document.querySelector('input[name="username"]').value = username;
      document.querySelector('input[name="ssid"]').value = ssid;
      const selector = document.querySelector('select[name="zone"]');
      if (zone) {
        selector.value = zone;
      } else {
        const current = new Option(timezone, '', true, true);
        current.disabled = true;
        selector.add(current, 0);
      }
    }
  })
  .catch(() => {});
function addDisplayColorPicker(doc, table) {
  const root = table?.shadowRoot;
  if (!root) return;
  const row = doc.createElement('div');
  row.className = 'entity-row';
  row.innerHTML = '<div></div><label for="display-color">LED Color</label>' +
                  '<div><input type="color" id="display-color" disabled><span role="status"></span></div>';
  const picker = row.querySelector('input');
  const status = row.querySelector('[role="status"]');
  const light = '/light/Display';
  picker.addEventListener('change', async () => {
    const [r, g, b] = picker.value.slice(1).match(/../g).map(channel => parseInt(channel, 16));
    status.textContent = '';
    try {
      const response = await fetch(light + '/turn_on?' + new URLSearchParams({r, g, b}), {method: 'POST'});
      if (!response.ok) throw new Error();
    } catch {
      status.textContent = ' Unable to set LED color.';
    }
  });
  const placePicker = () => {
    const heading = [...root.querySelectorAll('.tab-header')]
      .find(header => header.textContent.trim() === 'Colors and Effects');
    const group = heading?.nextElementSibling;
    if (group?.classList.contains('tab-container') && row.parentNode !== group) group.prepend(row);
  };
  const style = doc.createElement('style');
  style.textContent = '.colorpicker { display: none; }';
  root.append(style);
  new MutationObserver(placePicker).observe(root, {childList: true, subtree: true});
  placePicker();
  fetch(light, {cache: 'no-store'})
    .then(response => {
      if (!response.ok) throw new Error();
      return response.json();
    })
    .then(({color}) => {
      if (color && [color.r, color.g, color.b].every(Number.isInteger)) {
        picker.value = '#' + [color.r, color.g, color.b]
          .map(channel => channel.toString(16).padStart(2, '0')).join('');
      }
      picker.disabled = false;
    })
    .catch(() => { status.textContent = ' Unable to read the LED color.'; });
}

if (admin) {
  document.querySelector('#admin').hidden = false;
  const login = document.querySelector('form[action="/admin/login"]');
  const password = login.elements.password;
  const confirmation = login.elements.confirm_password;
  const checkPassword = () => confirmation.setCustomValidity(
    confirmation.value && confirmation.value !== password.value ? 'The passwords do not match.' : '');
  login.addEventListener('input', checkPassword);
  login.addEventListener('submit', event => {
    checkPassword();
    if (!login.reportValidity()) event.preventDefault();
  });
} else if (alarm) {
  document.querySelector('#alarm').hidden = false;
  const form = document.querySelector('form[action="/alarm/settings"]');
  const refresh = async (initial) => {
    const status = document.querySelector('#alarm-status');
    try {
      const response = await fetch('/alarm/state', {cache: 'no-store'});
      if (!response.ok) throw new Error();
      const state = await response.json();
      status.textContent = state.status;
      if (initial) {
        form.elements.enabled.checked = state.enabled;
        form.elements.skip.checked = state.skip;
        form.elements.time.value = state.time;
        form.elements.duration.value = state.duration;
        form.elements.volume.value = state.volume;
        form.elements.frequency.value = state.frequency;
        form.querySelector('button').disabled = false;
      }
    } catch {
      status.textContent = 'Unable to read alarm settings. Reload to retry.';
    }
  };
  form.querySelector('button').disabled = true;
  refresh(true);
  setInterval(() => refresh(false), 5000);
} else {
  document.querySelector('#controls-page').hidden = false;
  const panel = document.querySelector('#panel');
  panel.addEventListener('load', async () => {
    const doc = panel.contentDocument;
    const app = doc?.querySelector('esp-app');
    if (app) {
      await panel.contentWindow.customElements.whenDefined('esp-app');
      await app.updateComplete;
      if (app.shadowRoot) {
        const style = doc.createElement('style');
        style.textContent = 'header { display: none; }';
        app.shadowRoot.append(style);
        const table = app.shadowRoot.querySelector('esp-entity-table');
        if (table) {
          await panel.contentWindow.customElements.whenDefined('esp-entity-table');
          await table.updateComplete;
          addDisplayColorPicker(doc, table);
        }
      }
    }
    const body = doc?.body;
    if (!body) return;
    const fit = () => {
      const height = body.offsetTop + body.offsetHeight + 2;
      if (height > 2) panel.style.height = height + 'px';
    };
    new ResizeObserver(fit).observe(body);
    fit();
  });
  panel.src = '/?controls=1';
}
</script></html>)html";

constexpr char SETUP_ADDRESS[] = "192.168.101.1";
constexpr char OFFLINE_SETUP[] = R"html(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Clock Setup</title>
<style>
:root{color-scheme:light dark}body{max-width:33rem;margin:2rem auto;padding:0 1rem;font:16px system-ui,sans-serif}
label{display:grid;gap:.35rem;margin:1rem 0}input,select,button{font:inherit;padding:.45rem}
</style>
<h1>ESP32 WiFi Clock setup</h1>
<p><a href="/wifi-setup">Connect to Wi-Fi</a> for automatic time and outdoor weather.</p>
<h2>Use Offline</h2>
<p>Set the clock directly from this hotspot. A battery-backed RTC keeps time across a power loss. Without it, reconnect to set the time again.</p>
<form method="post" action="/offline-time">
<label>Timezone<select name="zone" required>
<option value="" disabled selected>Choose a timezone</option>
<option value="eastern">Eastern</option>
<option value="central">Central</option>
<option value="mountain">Mountain</option>
<option value="arizona">Arizona</option>
<option value="pacific">Pacific</option>
<option value="alaska">Alaska</option>
<option value="hawaii">Hawaii</option>
<option value="utc">UTC</option>
</select></label>
<label>Local date and time<input name="local_time" type="datetime-local" required min="2019-01-01T00:00" max="2099-12-31T23:59"></label>
<button>Set time and use offline</button>
</form>
<script>
const now = new Date();
now.setMinutes(now.getMinutes() - now.getTimezoneOffset());
document.querySelector('input[name="local_time"]').value = now.toISOString().slice(0, 16);
</script></html>)html";

struct Zone {
  const char *name;
  int west_hours;
  bool daylight_saving;
};

constexpr Zone ZONES[] = {
    {"eastern", 5, true}, {"central", 6, true}, {"mountain", 7, true}, {"arizona", 7, false},
    {"pacific", 8, true}, {"alaska", 9, true}, {"hawaii", 10, false}, {"utc", 0, false},
};

time::ParsedTimezone timezone_for(uint8_t index) {
  const auto &zone = ZONES[index];
  time::ParsedTimezone tz{};
  tz.std_offset_seconds = zone.west_hours * 3600;
  if (zone.daylight_saving) {
    tz.dst_offset_seconds = tz.std_offset_seconds - 3600;
    tz.dst_start = {7200, 0, time::DSTRuleType::MONTH_WEEK_DAY, 3, 2, 0};
    tz.dst_end = {7200, 0, time::DSTRuleType::MONTH_WEEK_DAY, 11, 1, 0};
  }
  return tz;
}

std::optional<time_t> local_epoch(const std::string &value, const time::ParsedTimezone &tz) {
  if (value.size() != 16) return std::nullopt;
  int year, month, day, hour, minute, consumed = 0;
  if (sscanf(value.c_str(), "%4d-%2d-%2dT%2d:%2d%n", &year, &month, &day, &hour, &minute, &consumed) != 5 ||
      consumed != 16 || year < 2019 || year > 2099 || month < 1 || month > 12 ||
      day < 1 || day > time::internal::days_in_month(year, month) ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59) return std::nullopt;

  struct tm wall{};
  wall.tm_year = year - 1900;
  wall.tm_mon = month - 1;
  wall.tm_mday = day;
  wall.tm_hour = hour;
  wall.tm_min = minute;
  time_t days = day - 1;
  for (int y = 1970; y < year; ++y) days += time::internal::is_leap_year(y) ? 366 : 365;
  for (int m = 1; m < month; ++m) days += time::internal::days_in_month(year, m);
  const time_t naive = ((days * 24 + hour) * 60 + minute) * 60;
  for (const int offset : {tz.dst_offset_seconds, tz.std_offset_seconds}) {
    if (!tz.has_dst() && offset != tz.std_offset_seconds) continue;
    const auto epoch = naive + offset;
    struct tm roundtrip{};
    if (time::epoch_to_local_tm(epoch, tz, &roundtrip) && roundtrip.tm_year == wall.tm_year &&
        roundtrip.tm_mon == wall.tm_mon && roundtrip.tm_mday == wall.tm_mday &&
        roundtrip.tm_hour == wall.tm_hour && roundtrip.tm_min == wall.tm_min) {
      return epoch;
    }
  }
  return std::nullopt;
}

bool set_clock_epoch(time_t epoch) {
  const timeval updated{epoch, 0};
  if (settimeofday(&updated, nullptr) != 0) return false;
  rtc::write_epoch(epoch);
  return true;
}

bool valid_hostname(const std::string &value) {
  return value.empty() ||
         (value.size() <= 30 && value.front() != '-' && value.back() != '-' &&
          value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-") == std::string::npos);
}

bool valid_domain(const std::string &value) {
  if (value.empty() || value.size() > 222 || value.back() == '.') return false;
  size_t start = 0;
  while (start < value.size()) {
    const size_t end = value.find('.', start);
    const size_t length = end == std::string::npos ? value.size() - start : end - start;
    if (length == 0 || length > 63 || value[start] == '-' || value[start + length - 1] == '-')
      return false;
    const size_t invalid = value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-", start);
    if (invalid != std::string::npos && invalid < start + length) return false;
    if (end == std::string::npos) return true;
    start = end + 1;
  }
  return false;
}

bool valid_username(const std::string &username) {
  return !username.empty() && username.size() <= 32 &&
         username.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-") ==
             std::string::npos;
}

bool printable_password(const std::string &password) {
  for (unsigned char ch : password) {
    if (ch < 32 || ch > 126) return false;
  }
  return true;
}

}  // namespace

void ClockAdmin::setup() {
  hostname_preference_ = global_preferences->make_preference<SavedHostname>(0x4855484F);
  if (hostname_preference_.load(&hostname_)) {
    hostname_.back() = '\0';
    if (!valid_hostname(hostname_.data())) hostname_.fill('\0');
  } else {
    // Preserve a hostname saved by the earlier hostname-prefix implementation.
    std::array<char, 26> old_prefix{};
    auto previous = global_preferences->make_preference<decltype(old_prefix)>(0x4855484E);
    if (previous.load(&old_prefix)) {
      old_prefix.back() = '\0';
      if (valid_hostname(old_prefix.data())) memcpy(hostname_.data(), old_prefix.data(), old_prefix.size());
    }
  }
  domain_preference_ = global_preferences->make_preference<SavedDomain>(0x4855444D);
  domain_preference_.load(&domain_);
  domain_.back() = '\0';
  if (!valid_domain(domain_.data())) memcpy(domain_.data(), "local", sizeof("local"));

  auto *base = web_server_base::global_web_server_base;
  preference_ = global_preferences->make_preference<Credentials>(0x48554341);
  if (preference_.load(&credentials_)) {
    credentials_.username[sizeof(credentials_.username) - 1] = '\0';
    credentials_.password[sizeof(credentials_.password) - 1] = '\0';
    if (valid_username(credentials_.username) && strlen(credentials_.password) >= 8 &&
        printable_password(credentials_.password)) {
      current_username_ = credentials_.username;
      base->set_auth_username(credentials_.username);
      base->set_auth_password(credentials_.password);
    }
  }
  timezone_preference_ = global_preferences->make_preference<uint8_t>(0x4855545A);
  uint8_t zone;
  if (timezone_preference_.load(&zone) && zone < std::size(ZONES)) {
    time::set_global_tz(timezone_for(zone));
    timezone_override_ = ZONES[zone].name;
  }
  base->add_handler_without_auth(&offline_setup_);
  base->add_handler(this);
}

bool ClockAdmin::OfflineSetupHandler::canHandle(AsyncWebServerRequest *request) const {
  if (!captive_portal::global_captive_portal->is_active() || wifi::global_wifi_component->is_connected())
    return false;
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  const auto path = request->url_to(buffer);
  if (request->method() == HTTP_POST) return path == "/offline-time";
  return request->method() == HTTP_GET &&
         (path == "/" || path == "/generate_204" || path == "/hotspot-detect.html" || path == "/connecttest.txt");
}

void ClockAdmin::OfflineSetupHandler::handleRequest(AsyncWebServerRequest *request) {
  const auto host = request->get_header("Host");
  if (request->method() == HTTP_GET) {
    if (!host.has_value() || *host != SETUP_ADDRESS) {
      request->redirect(std::string("http://") + SETUP_ADDRESS + "/");
    } else {
      request->send(200, "text/html", OFFLINE_SETUP);
    }
    return;
  }

  const auto origin = request->get_header("Origin");
  if (!host.has_value() || *host != SETUP_ADDRESS || !origin.has_value() ||
      *origin != std::string("http://") + SETUP_ADDRESS) {
    request->send(403, "text/plain", "Open the setup page at http://192.168.101.1/.");
    return;
  }
  const auto name = request->arg("zone");
  for (uint8_t zone = 0; zone < std::size(ZONES); ++zone) {
    if (name != ZONES[zone].name) continue;
    const auto tz = timezone_for(zone);
    const auto epoch = local_epoch(request->arg("local_time"), tz);
    if (!epoch.has_value()) break;
    owner_.defer([this, zone, tz, epoch = *epoch]() {
      if (!captive_portal::global_captive_portal->is_active() || wifi::global_wifi_component->is_connected())
        return;
      if (!set_clock_epoch(epoch)) {
        ESP_LOGE("clock_admin", "Unable to set offline clock time");
        return;
      }
      time::set_global_tz(tz);
      owner_.timezone_override_ = ZONES[zone].name;
      if (!owner_.timezone_preference_.save(&zone) || !global_preferences->sync())
        ESP_LOGE("clock_admin", "Unable to save offline timezone");
    });
    request->send(200, "text/html", "<p>Time submitted. Check the clock display. You can now disconnect from the setup hotspot.</p><a href='/'>Back to setup</a>");
    return;
  }
  request->send(400, "text/html", "<p>Choose a timezone and a valid local date and time.</p><a href='/'>Back to setup</a>");
}

void ClockAdmin::apply_network_hostname() {
  if (hostname_[0] == '\0') return;
  if (auto *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"))
    esp_netif_set_hostname(sta, hostname_.data());
  mdns_hostname_set(hostname_.data());
  mdns_instance_name_set(hostname_.data());
}

bool ClockAdmin::canHandle(AsyncWebServerRequest *request) const {
  if (captive_portal::global_captive_portal->is_active()) return false;
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  auto path = request->url_to(buffer);
  if (request->method() == HTTP_GET)
    return path == "/" || path == "/admin" || path == "/alarm" || path == "/alarm/state" || path == "/admin/hostname";
  return request->method() == HTTP_POST && (path == "/admin/wifi" || path == "/admin/login" ||
                                          path == "/admin/timezone" || path == "/admin/time" ||
                                          path == "/alarm/settings" || path == "/alarm/test" ||
                                          path == "/alarm/silence" || path == "/admin/hostname" || path == "/admin/domain" ||
                                          path == "/admin/reset");
}

void ClockAdmin::handleRequest(AsyncWebServerRequest *request) {
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  auto path = request->url_to(buffer);
  if (request->method() == HTTP_GET) {
    if (path == "/" && request->arg("controls") == "1") {
      web_server_->handle_index_request(request);
    } else if (path == "/admin/hostname") {
      auto json = json::build_json([&](JsonObject root) {
        root["hostname"] = hostname_[0] ? hostname_.data() : App.get_name().c_str();
        root["domain"] = domain_.data();
        root["username"] = current_username_;
        char ssid[wifi::SSID_BUFFER_SIZE];
        root["ssid"] = std::string(wifi::global_wifi_component->wifi_ssid_to(ssid));
        root["zone"] = timezone_override_ ? timezone_override_ : "";
        root["timezone"] = default_timezone_;
      });
      request->send(200, "application/json", json.c_str());
    } else if (path == "/alarm/state") {
      const auto alarm = alarm_time_->state_as_esptime();
      char time_value[8];
      snprintf(time_value, sizeof(time_value), "%02u:%02u", alarm.hour, alarm.minute);
      auto json = json::build_json([&](JsonObject root) {
        root["time"] = time_value;
        root["enabled"] = alarm_enabled_->state;
        root["skip"] = alarm_skip_->state;
        root["duration"] = alarm_duration_->state;
        root["volume"] = buzzer_volume_->state;
        root["frequency"] = buzzer_frequency_->state;
        root["status"] = alarm_status_->state;
      });
      request->send(200, "application/json", json.c_str());
    } else {
      request->send(200, "text/html", PAGE);
    }
    return;
  }

  const auto host = request->get_header("Host");
  const auto origin = request->get_header("Origin");
  if (!host.has_value() || !origin.has_value() || *origin != "http://" + *host) {
    request->send(403, "text/plain", "Open the Administration page on this clock to save changes.");
    return;
  }
  if (path == "/admin/hostname") {
    const auto value = request->arg("hostname");
    if (!request->hasArg("hostname") || !valid_hostname(value)) {
      request->send(400, "text/plain", "Enter up to 30 letters, digits, or hyphens. Start and end with a letter or digit.");
      return;
    }
    SavedHostname hostname{};
    memcpy(hostname.data(), value.data(), value.size());
    const std::string next = (value.empty() ? App.get_name().str() : value) + "." + domain_.data();
    const std::string message = "Hostname change submitted. After restart, open http://" + next + "/";
    request->send(200, "text/plain", message.c_str());
    this->defer([this, hostname]() {
      if (hostname_preference_.save(&hostname) && global_preferences->sync()) App.safe_reboot();
      else ESP_LOGE("clock_admin", "Unable to save hostname");
    });
    return;
  }
  if (path == "/admin/domain") {
    auto value = request->arg("domain");
    if (value.empty()) value = "local";
    if (!request->hasArg("domain") || !valid_domain(value)) {
      request->send(400, "text/plain", "Enter a domain such as local or home.arpa, with no scheme or trailing dot.");
      return;
    }
    SavedDomain domain{};
    memcpy(domain.data(), value.data(), value.size());
    request->send(200, "text/plain", "Domain change submitted. The clock will restart.");
    this->defer([this, domain]() {
      if (domain_preference_.save(&domain) && global_preferences->sync()) App.safe_reboot();
      else ESP_LOGE("clock_admin", "Unable to save domain");
    });
    return;
  }
  if (path == "/admin/reset") {
    if (request->arg("confirm") != "yes") {
      request->send(400, "text/plain", "Confirm that you want to erase all saved settings.");
      return;
    }
    const std::string default_name = App.get_name().str();
    const std::string message =
        "Factory reset submitted. Join the " + default_name +
        " setup hotspot and open http://192.168.101.1/. After saving Wi-Fi, rejoin your home network "
        "and open http://" + default_name + ".local/ using the original website login.";
    request->send(200, "text/plain", message.c_str());
    this->defer([]() {
      if (global_preferences->reset()) {
        rtc::forget_time();
        const timeval unset{};
        settimeofday(&unset, nullptr);
        App.safe_reboot();
      }
    });
    return;
  }
  if (path == "/alarm/settings") {
    const auto value = request->arg("time");
    ESPTime alarm{};
    const auto duration = parse_number<int>(request->arg("duration"));
    const auto volume = parse_number<int>(request->arg("volume"));
    const auto frequency = parse_number<int>(request->arg("frequency"));
    if (value.size() != 5 || !ESPTime::strptime(value.c_str(), value.size(), alarm) ||
        alarm.hour > 23 || alarm.minute > 59 || !duration.has_value() || *duration < 1 || *duration > 300 ||
        !volume.has_value() || *volume < 0 || *volume > 100 || !frequency.has_value() ||
        *frequency < 1000 || *frequency > 5000 || *frequency % 100 != 0) {
      request->send(400, "text/plain", "Check the alarm time, duration, volume, and tone frequency.");
      return;
    }
    const bool enabled = request->hasArg("enabled");
    const bool skip = request->hasArg("skip");
    this->defer([this, alarm, enabled, skip, seconds = *duration, level = *volume, hz = *frequency]() {
      auto time_call = alarm_time_->make_call();
      time_call.set_hour(alarm.hour);
      time_call.set_minute(alarm.minute);
      time_call.set_second(0);
      time_call.perform();
      alarm_duration_->make_call().set_value(seconds).perform();
      buzzer_volume_->make_call().set_value(level).perform();
      buzzer_frequency_->make_call().set_value(hz).perform();
      if (skip) alarm_skip_->turn_on();
      else alarm_skip_->turn_off();
      if (enabled) alarm_enabled_->turn_on();
      else alarm_enabled_->turn_off();
    });
    request->redirect("/alarm");
    return;
  }
  if (path == "/alarm/test" || path == "/alarm/silence") {
    auto *button = path == "/alarm/test" ? test_buzzer_ : silence_alarm_;
    this->defer([button]() { button->press(); });
    request->redirect("/alarm");
    return;
  }
  if (path == "/admin/timezone") {
    const auto name = request->arg("zone");
    for (uint8_t zone = 0; zone < std::size(ZONES); ++zone) {
      if (name != ZONES[zone].name) continue;
      request->redirect("/admin");
      this->defer([this, zone]() {
        if (timezone_preference_.save(&zone) && global_preferences->sync()) {
          time::set_global_tz(timezone_for(zone));
          timezone_override_ = ZONES[zone].name;
        } else {
          ESP_LOGE("clock_admin", "Unable to save timezone");
        }
      });
      return;
    }
    request->send(400, "text/plain", "Choose a timezone.");
    return;
  }
  if (path == "/admin/time") {
    const auto local_time = request->arg("local_time");
    request->redirect("/admin");
    this->defer([local_time]() {
      const auto epoch = local_epoch(local_time, time::get_global_tz());
      if (!epoch.has_value() || !set_clock_epoch(*epoch)) ESP_LOGW("clock_admin", "Invalid or nonexistent local time");
    });
    return;
  }
  const auto password = request->arg("password");
  if (path == "/admin/wifi") {
    const auto ssid = request->arg("ssid");
    const bool hex_key = password.size() == 64 &&
                         password.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
    if (ssid.empty() || ssid.size() > 32 || ssid.find('\0') != std::string::npos ||
        !request->hasArg("password") || !printable_password(password) ||
        (!password.empty() && !hex_key && (password.size() < 8 || password.size() > 63))) {
      request->send(400, "text/plain", "Enter a valid SSID and Wi-Fi password.");
      return;
    }
    request->send(200, "text/html", "<p>Wi-Fi change submitted. The clock is restarting.</p><a href='/admin'>Administration</a>");
    this->defer([ssid, password]() {
      wifi::global_wifi_component->save_wifi_sta(ssid, password);
      App.safe_reboot();
    });
    return;
  }

  if (!request->hasArg("confirm_password") || request->arg("confirm_password") != password) {
    request->send(400, "text/plain", "The new passwords must match.");
    return;
  }
  const auto username = request->arg("username");
  if (!valid_username(username) || password.size() < 8 || password.size() > 64 ||
      !printable_password(password)) {
    request->send(400, "text/plain", "Enter a valid username and a password of 8 to 64 printable characters.");
    return;
  }
  Credentials replacement{};
  memcpy(replacement.username, username.data(), username.size());
  memcpy(replacement.password, password.data(), password.size());
  request->send(200, "text/html", "<p>Login change submitted. After the clock restarts, return to Administration and sign in again.</p><a href='/admin'>Administration</a>");
  this->defer([this, replacement]() {
    if (preference_.save(&replacement) && global_preferences->sync()) {
      App.safe_reboot();
    } else {
      ESP_LOGE("clock_admin", "Unable to save website login");
    }
  });
}

}  // namespace esphome::clock_admin
