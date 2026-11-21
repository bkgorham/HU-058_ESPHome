# Changelog

Versions describe firmware changes. Entries before this changelog was created were reconstructed from the project conversation. Git commits can contain several firmware revisions.

The latest local version, 1.9.3, compiled successfully with ESPHome 2026.10.2. Its latest features still require physical-clock testing.

## 1.9.3

Changed automatic temperature rotation to a whole-minute input, default 5 minutes. Preserved saved intervals, rounding partial minutes upward.

## 1.9.2

Added the next local ring time and remaining duration to the snoozed alarm status.

## 1.9.1

Aligned Alarm and Timer with the Controls layout using grouped headings, alternating compact rows, right-aligned inputs, and consistent action buttons.

## 1.9.0

Added timer pause/resume and long-press cancellation. Moved timer configuration and live remaining-time status to a separate section on Alarm and Timer.

## 1.8.0

Added a saved automatic temperature rotation interval with side-by-side minutes and seconds inputs. Defaults to five minutes. Enabling rotation or changing the interval resets the wait.

## 1.7.2

Enlarged the timer duration formatting buffer to resolve a compiler truncation warning.

## 1.7.1

Changed timer duration entry to right-aligned minutes and seconds boxes.

## 1.7.0

Added configurable alarm snooze. Button 1 snoozes a daily alarm. Button 2 cancels pending snooze with three display flashes.

## 1.6.2

Added Button 1 increments on the timer ready screen, using the configured duration as the increment.

## 1.6.1

Added minutes:seconds duration entry and flashing 00:00 during the timer completion alert.

## 1.6.0

Added the countdown timer, prepare/start button sequence, configurable duration and alert length, and website controls.

## 1.5.0

Added optional five-minute temperature rotation, connection and time-source status, last network sync, and saved LDR calibration endpoints.

## 1.4.1

Moved DS1302 diagnostics into a dedicated Real-Time Clock group.

## 1.4.0

Added DS1302 communication, ticking, timestamp, and clock-difference diagnostics.

## 1.3.0

Added DS1302 battery-backed UTC timekeeping, boot restoration, manual/network writes, and RTC wiring documentation.

## 1.2.1

Enabled both colon dots in offline mode.

## 1.2.0

Added offline time and timezone setup through the setup hotspot.

## 1.1.5

Changed outdoor and indoor temperature display durations to three seconds each.

## 1.1.4

Restored a dedicated LED color selector in Colors and Effects.

## 1.1.3

Prefilled Administration fields with the current username, hostname, SSID, and timezone.

## 1.1.2

Added clearer post-reset connection instructions.

## 1.1.1

Added new-password confirmation in the browser and device handler.

## 1.1.0

Added saved custom domain settings and corrected the header title casing.

## 1.0.2

Added a gradient header with the title, full hostname, and firmware version.

## 1.0.1

Added shared hostname navigation and removed the duplicate Controls banner.

## 1.0.0

Established firmware versioning for the standalone clock, including existing web setup, alarms, sensors, weather, display effects, scheduling, and OTA.
