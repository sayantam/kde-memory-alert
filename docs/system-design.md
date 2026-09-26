# KDE Memory Alert — minimal system design

Status: approved by the user; [TODO list](TODO.md) implemented, with remaining manual checks recorded in [validation results](validation.md). Implements [the user spec](user-spec.md). The Wayland integration below incorporates findings from testing on the target desktop.

## Technology and structure

Use C++17, Qt 6 Widgets, KDE Frameworks 6, CMake, and KDE Extra CMake Modules. A single process and the Qt event loop are sufficient for reading a small local file every 10 seconds. No worker thread or background service is needed.

Target the user's Plasma 6.6.6 Wayland session on Kubuntu 26.04 LTS, with KDE Frameworks 6.24.0 and Qt 6.10.2. Desktop validation uses native Qt Wayland; X11 support is outside the first-version validation scope.

| Component | Responsibility |
| --- | --- |
| `MonitorOptions` | Own the defaults: `thresholdPercent = 90.0` and `pollInterval = 10s`. Pass these values into the controller; future configuration changes only their construction. |
| `MemoryReader` | Read and validate `/proc/meminfo`; return a measurement or an error. Keep parsing independently testable. |
| `MonitorController` | Sample immediately and through `QTimer`; round usage to one decimal place, compare it against the supplied threshold, and pass that same value to the window and tray. |
| `AlertWindow` | Reusable, nonmodal `QWidget` with a themed warning icon and a text label; show, refresh, or hide under controller control. |
| `Tray integration` | One `KStatusNotifierItem`, a current-status tooltip, and a custom menu containing only Quit. |
| `main` | Set application identity, enforce one instance, construct components, and run `QApplication`. |

Build dependencies: Qt Core/Gui/Widgets, KF6 StatusNotifierItem, DBusAddons, I18n, ECM, and KDE LayerShellQt 6.6. Qt Test is needed for automated checks. Use KDE translation placeholders for the alert's actual and threshold values rather than embedding defaults in text.

## Sampling and state

Read `MemTotal` and `MemAvailable` from `/proc/meminfo`, retaining their common units. Require both fields, a positive total, and available memory between zero and total. Calculate usage with floating-point division. `MemAvailable` estimates RAM available for new applications without swapping, including reclaimable memory. [Linux kernel documentation](https://docs.kernel.org/filesystems/proc.html)

| Successful sample (rounded usage) | Window | Tray status |
| --- | --- | --- |
| Usage <= threshold | Hidden | `Passive` |
| Usage > threshold | Shown; refresh reading | `Active` |

Round each successful measurement once to the nearest tenth of a percentage point, with halfway values rounded up (`std::round(rawUsagePercent * 10.0) / 10.0` for nonnegative usage). Compare this rounded value with the supplied threshold. Equality is a normal state: a rounded sample exactly at the threshold must neither trigger a warning nor keep an existing warning visible. With the default threshold, 90.04% rounds to 90.0% and hides the warning; 90.06% rounds to 90.1% and shows it.

Format that same rounded usage value to one decimal place and insert the supplied threshold into `Warning! Memory usage at <actual>% — exceeds <threshold>% threshold`. Use the same rounded value in the tray tooltip so the displayed reading and alert decision agree.

Create one window and reuse it. Repeated high samples update its text without creating or reactivating windows. Failed samples retain the previous state and mark the reading as stale; a subsequent successful sample restores normal presentation. No smoothing or separate recovery threshold is applied.

## KDE desktop integration

Use `KStatusNotifierItem` with a stable ID, the `Hardware` category, and themed icons. Set `Passive` at or below the threshold and `Active` above it. Disable standard menu actions and supply exactly one Quit action. Avoid associating the warning window with the tray item's default show/hide behavior. Plasma controls final icon presentation. [KDE tray API](https://api.kde.org/kstatusnotifieritem.html)

On Wayland, use KDE LayerShellQt to place the warning in `LayerTop`, with `KeyboardInteractivityNone` and activation disabled. The layer surface has no window-manager close or minimize controls. The initial desktop test demonstrated that ordinary Qt window hints alone were ignored by KWin for focus and stacking; layer-shell supplies the required compositor behavior. [KDE LayerShellQt API source](https://github.com/KDE/layer-shell-qt/blob/Plasma/6.6/src/interfaces/window.h)

Before showing a new warning, asynchronously read Plasma's panel and system-tray widget geometry through its D-Bus scripting interface, with a one-second timeout. Select the tray's screen, align the warning with the tray, and position it just outside the panel, clamped inside the screen. Support all four panel edges; prefer the pointer's screen when several trays exist. Supply screen-local coordinates to layer-shell with an exclusive zone of -1 because the calculated position already includes panel clearance; the warning reserves no screen space. If the tray is unavailable or scripting is restricted, retain the top-right fallback on the active screen. The query never writes Plasma settings. [Plasma scripting API](https://develop.kde.org/docs/plasma/scripting/api/)

Retain Qt's nonactivation/focus hints, omit close/minimize controls, and ignore ordinary close requests while alerting. Never request window activation. Use `QApplication::setQuitOnLastWindowClosed(false)` so hiding the warning preserves monitoring; Quit explicitly terminates the application. Validate the resulting layer surface and keyboard focus in the target KWin Wayland session. [Qt window flags and attributes](https://doc.qt.io/qt-6.10/qt.html)

Use `KDBusService` in `Unique` mode to prevent duplicate monitors. A second launch must not activate the alert window. [KDE single-instance API](https://api.kde.org/kdbusservice.html)

## Installation and autostart

Provide a per-user install path with an executable under `~/.local/bin`, an application desktop entry under the XDG data applications directory, and an autostart desktop entry under `$XDG_CONFIG_HOME/autostart` (normally `~/.config/autostart`). Generate desktop entries with the installed executable's absolute path so login does not depend on shell PATH settings. Restrict autostart to KDE with `OnlyShowIn=KDE;` and disable startup feedback with `StartupNotify=false`. Supply matching uninstall instructions. [XDG autostart specification](https://specifications.freedesktop.org/autostart/latest/)

## Validation boundary

Use small synthetic memory samples to check parsing, invalid input, repeated high readings, recovery, and failure followed by recovery. Explicitly verify the sequence 89% → 90% → 91% → 90% produces hidden → hidden → shown → hidden, and that a first sample exactly at 90% leaves the warning hidden. Verify raw samples of 90.04% → 90.06% → 90.04% produce rounded values of 90.0% → 90.1% → 90.0% and states hidden → shown → hidden. Include a halfway rounding case and confirm that displayed usage matches the value used for comparison. Pass an alternative threshold to verify both decisions and message values follow `MonitorOptions`.

On the target Plasma Wayland desktop, verify panel versus hidden-tray placement, persistent warning visibility, unchanged keyboard focus, Quit, duplicate-launch handling, and autostart after login. Exercise alert transitions using injected test measurements rather than exhausting real RAM. Compositor behavior and actual login startup require desktop validation; automated logic tests alone cannot establish them.
