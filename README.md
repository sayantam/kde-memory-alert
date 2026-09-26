# KDE Memory Alert

A small KDE background app that warns when system RAM usage exceeds 90%. It samples immediately at launch and every 10 seconds, starts at KDE login, and stays in the hidden tray area while usage is normal.

The warning appears above ordinary windows without taking keyboard focus. It updates while memory remains high and disappears automatically when usage returns to the threshold or below. The tray menu contains only **Quit**. Quit stops monitoring until the next launch or login.

Target desktop: Kubuntu 26.04 LTS, Plasma 6.6.6 **Wayland**, KDE Frameworks 6.24.0, Qt 6.10.2. KDE LayerShellQt supplies the warning's noninteractive Wayland surface. X11 is outside the validated scope.

## Build and test

Install the development dependencies:

```sh
sudo apt-get install --no-install-recommends \
  build-essential cmake ninja-build extra-cmake-modules qt6-base-dev \
  libkf6statusnotifieritem-dev libkf6dbusaddons-dev libkf6i18n-dev \
  liblayershellqtinterface-dev desktop-file-utils python3-dbus python3-gi
```

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local" -DBUILD_TESTING=ON \
  -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Tests use an offscreen Qt backend and private D-Bus sessions. They exercise parsing, rounding, alert transitions, stale readings, timer updates, message values, single-instance behavior, and the exported tray Quit action. They do not stress real RAM. To omit test dependencies and test tools, configure with `-DBUILD_TESTING=OFF`.

## Install and run

```sh
cmake --install build
"$HOME/.local/bin/kde-memory-alert"
```

Installation is per-user; do not use sudo. It installs:

| File | Default location |
| --- | --- |
| Executable | `~/.local/bin/kde-memory-alert` |
| Launcher | `~/.local/share/applications/io.github.sayantam.kde-memory-alert.desktop` |
| Login autostart entry | `~/.config/autostart/io.github.sayantam.kde-memory-alert.desktop` |

The data/config locations honor absolute `XDG_DATA_HOME` and `XDG_CONFIG_HOME` values at the first CMake configuration. They can also be set explicitly with `-DMEMORY_ALERT_DATA_HOME=/absolute/path` and `-DMEMORY_ALERT_CONFIG_HOME=/absolute/path`. Desktop entries embed the configured executable path, so autostart does not depend on your shell PATH. Reconfigure and reinstall if that path changes; do not override it only at install time with `cmake --install --prefix`.

The launcher is also available as **KDE Memory Alert** in the application menu. Repeated launches leave the existing instance running without showing or focusing a window. At normal usage, open Plasma's hidden tray items to find Quit. Plasma's per-icon “always show” or “always hidden” settings override automatic tray visibility.

Autostart runs at the next KDE login. You can disable it in System Settings → Autostart. Selecting Quit leaves the autostart entry installed.

To uninstall, first select Quit, then use the same build directory used for installation:

```sh
cmake --build build --target uninstall
```

This removes only the three application files; it leaves shared directories intact. Keep the build directory until uninstall, or remove the three files at the configured locations manually.

## Memory and defaults

Usage is `100 × (MemTotal − MemAvailable) / MemTotal`, read from `/proc/meminfo`. Available RAM includes reclaimable memory, so this measures RAM pressure rather than treating all cache as used. Swap and CPU utilization are not included.

The app rounds usage to one decimal place, with halfway values rounded up, then compares and displays that same value. With the default threshold:

| Raw usage | Displayed usage | Warning |
| --- | --- | --- |
| 90.00% | 90.0% | Hidden |
| 90.04% | 90.0% | Hidden |
| 90.05% | 90.1% | Visible |
| 90.06% | 90.1% | Visible |

The message is `Warning! Memory usage at <actual>% — exceeds <threshold>% threshold`. Threshold and polling defaults are defined once in [src/monitoroptions.h](src/monitoroptions.h); messages and decisions receive those values. There is no settings UI or config file in this version.

The warning appears beside the system tray on its screen, aligned with the tray and just outside the panel. It supports panels on all four edges and reads the current Plasma layout before each new alert. If multiple screens have trays, it prefers the one on the pointer's screen. If Plasma cannot supply a tray location, it falls back to the active screen's top-right corner. The warning has no dismiss or minimize controls. Quit remains available in the tray. A failed sample preserves the last alert state and marks the reading stale until a successful sample arrives. Before the first valid sample, the tooltip reports that usage is unavailable. Errors are written to standard error (normally captured by the user session journal for autostart).

## Desktop validation

The test-only harness uses synthetic readings with the production window, tray, and controller. Quit the production application before using it:

```sh
QT_QPA_PLATFORM=wayland ./build/tests/memory-alert-harness
```

Choose a sample, then switch to another application and type. The sample changes after five seconds, allowing you to check focus preservation, warning placement, tray transitions, recovery, and stale readings. The harness polls every second; production continues to use ten seconds. Its separate executable is never installed.

For an automated live-session check, close both app and harness, then run:

```sh
/usr/bin/python3 tests/session-smoke.py \
  build/kde-memory-alert build/tests/memory-alert-harness --wayland
```

This briefly shows warnings and inspects their KWin state through a temporary script, which it unloads after each query. Avoid changing focus while it runs. It stops only its own processes and leaves desktop settings unchanged.

See [docs/validation.md](docs/validation.md) for completed checks and remaining manual checks. In particular, a real logout/login cycle is needed to verify login autostart; the test suite does not log you out.

Project documents: [user spec](docs/user-spec.md), [system design](docs/system-design.md), [TODO list](docs/TODO.md).

License: [ISC](LICENSE.md).
