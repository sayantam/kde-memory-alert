# KDE Memory Alert — first-version TODO

Status: implementation complete; automated and live Wayland checks passed. Manual desktop/login confirmation remains open; see [validation results](validation.md).

Follow the approved [system design](system-design.md) and [user spec](user-spec.md). Implement the steps in order, checking off each step after its acceptance checks pass. Record any validation that still requires the user's desktop rather than marking it complete.

## 1. Set up the build and application skeleton

- [x] Check available compiler, CMake, ECM, Qt 6, and required KF6 development dependencies; document missing packages and build commands.
- [x] Create the C++17 CMake project, application identity, source layout, and build-output ignore rules. Enable Qt Test through CTest when testing is enabled.
- [x] Add `MonitorOptions` with the sole default definitions for the 90% threshold and 10-second interval.

Acceptance: a clean configure and build succeeds with the required dependencies; defaults are supplied to components rather than repeated in their logic or messages.

## 2. Read and evaluate memory usage

- [x] Implement `/proc/meminfo` reading and independently testable parsing of `MemTotal` and `MemAvailable`, with explicit errors for missing, malformed, or invalid values.
- [x] Calculate usage and round it once to one decimal place, using nearest-tenth rounding with halfway values rounded up. Alert only when that rounded value exceeds the supplied threshold.
- [x] Add focused tests for valid and invalid samples, equality, rounding boundaries, halfway rounding, and an alternative threshold.

Acceptance: 90.04% becomes 90.0% and does not alert; 90.06% becomes 90.1% and alerts. Exactly 90% is normal. Invalid samples never become fabricated readings.

## 3. Connect monitoring to the warning and tray

- [x] Implement the controller with an immediate first sample and a repeating timer using `MonitorOptions`.
- [x] Create one reusable warning window with a themed icon and translated message placeholders. Display the same rounded usage used for the decision, and obtain the message threshold from options.
- [x] Apply the approved focus and window flags; omit dismiss/minimize controls and ignore ordinary close requests while alerting. Hide the window automatically on recovery.
- [x] Add the KDE tray item, consistent tooltip, passive/active state changes, and a menu containing only Quit. Tray interaction must not dismiss the warning.
- [x] Keep the application running when the warning is hidden. Retain state on sampling errors, mark existing readings stale, log errors, and retry; handle an initial failure with an unavailable tooltip.

Acceptance: a first runnable monitor shows and updates one warning above the threshold, hides it at or below the threshold, and provides Quit in both tray states.

## 4. Verify transitions and single-instance behavior

- [x] Add `KDBusService` single-instance handling so a second launch neither creates another monitor nor activates the warning.
- [x] Add an internal test seam for controlled memory samples, including failures, without adding a production settings UI or altering production defaults.
- [x] Test immediate sampling, timer-driven updates, repeated high samples, message/tooltip consistency, recovery, initial failure, and failure followed by recovery.
- [x] Verify the sequences 89% → 90% → 91% → 90% and 90.04% → 90.06% → 90.04%, including an alternative threshold to catch embedded defaults.

Acceptance: automated checks pass; an internal desktop test harness can exercise the actual warning and tray without consuming large amounts of RAM.

## 5. Add per-user installation and login autostart

- [x] Add installation support for the executable, application desktop entry, and KDE-only XDG autostart entry, using the installed executable's absolute path and disabling startup feedback.
- [x] Respect XDG data/config directory locations and provide uninstall instructions for the installed files.
- [x] Check a staged installation and validate the generated desktop entries before installing the first copy for desktop testing.

Acceptance: installed entries resolve to the executable; autostart is included in the first version; Quit does not remove or disable it.

## 6. Validate on Plasma Wayland

- [x] Position the warning beside the system tray; verify all four edge calculations and the live left-side panel placement without changing keyboard focus.
- [ ] Verify normal startup stays hidden, with the tray item accessible in Plasma's hidden-items area.
- [ ] Use controlled samples to verify the active tray icon, persistent warning above ordinary windows, changing readings, and automatic recovery.
- [ ] While typing in another application, verify warning appearance and updates preserve keyboard focus. Confirm close/minimize/tray interactions cannot dismiss an active warning.
- [x] Verify Quit and repeated launches in both normal and alert states.
- [ ] Verify a real KDE login starts one monitor automatically; after Quit, verify a subsequent login starts it again. Coordinate any logout with the user.

Acceptance: the agreed behavior works in the target native Wayland session. Any unmet focus or visibility requirement remains an open issue until resolved; automated tests do not substitute for these checks.

Automated live-session checks passed for passive/active tray status, above-normal stacking, disabled keyboard input, unchanged active window, stale readings, and recovery. The unchecked visual and typing checks still need user confirmation. LayerShellQt replaced reliance on ordinary Qt window hints after the initial Wayland test failed.

## 7. Finish the first working copy

- [x] Complete the README with dependencies, build/test/install/run/uninstall instructions, the metric and rounding rules, fixed defaults, tray behavior, and Quit/autostart behavior.
- [x] Record automated and desktop validation results, including any checks awaiting user verification.
- [x] Review the completed implementation against the approved spec and report remaining issues, if any.

Acceptance: the user can build, install, and run the application using the documented instructions, with validation status clearly stated. Settings, sounds, graphs, history, and other deferred features remain future work.
