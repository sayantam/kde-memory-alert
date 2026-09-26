# KDE Memory Alert — first-version user spec

Status: reflected in the approved system design. Implementation complete; manual validation items are recorded in [validation results](validation.md).

Build a small background application for Kubuntu 26.04 LTS, KDE Plasma 6.6.6 on Wayland, KDE Frameworks 6.24.0, and Qt 6.10.2.

## Required behavior

- Measure system RAM usage as `100 × (MemTotal − MemAvailable) / MemTotal`. Swap usage and CPU utilization are outside scope.
- Round usage to one decimal place before comparing it with the threshold. All threshold rules below use this rounded value.
- Sample immediately at launch, then every 10 seconds while running. Changes are reflected on the next sample.
- Use a fixed threshold of 90% in this version. Define the threshold and sampling interval once, separately from monitoring logic and message text, so later configuration can replace the defaults.
- Below or equal to the threshold: run in the background with no alert window and a passive tray icon in Plasma's hidden-items area.
- Above the threshold: show the tray icon in the panel and display one persistent warning window without taking keyboard focus.
- Display `Warning! Memory usage at <actual>% — exceeds <threshold>% threshold`. Populate both values from application data. Update the actual percentage on every successful sample while the alert is visible.
- Dismiss the warning automatically and return the tray icon to its passive state only when a sample is equal or below the threshold.
- Provide a tray menu containing only **Quit**.
- Start automatically at KDE login from the first version. Quit stops the current session's monitoring; it does not disable autostart at the next login.

Tray visibility follows Plasma's default automatic policy. Explicit user choices to always show or always hide an icon take precedence.

## Proposed interaction details

- The warning stays above ordinary windows while active. It has no dismiss or minimize control; ordinary close requests are ignored while alerting. Quit remains available from the tray.
- Position the warning near the system tray, on the tray's screen and just outside its panel.
- Tray interaction does not toggle the warning off. The hidden-items area provides access to Quit while memory is below the threshold.
- Display and compare the same usage value, rounded to one decimal place (nearest tenth, with halfway values rounded up). For example, 90.04% becomes 90.0% and does not alert; 90.06% becomes 90.1% and alerts at the default threshold.
- Repeated launches keep one running instance and do not bring a window into focus.
- If a memory sample fails, retain the last alert state, mark any displayed reading as stale, log the error, and retry at the next interval. Before the first valid sample, show an unavailable status in the tray tooltip without inventing a usage reading.

## Scope

The first version includes the monitor, warning window, tray integration, per-user installation, and login autostart. Settings UI, configuration files, sounds, history, graphs, additional thresholds, and hysteresis are deferred.
