# First-version validation

Validated on 2026-09-26 using the target Kubuntu/Plasma desktop: KWin 6.6.6 on Wayland, Qt 6.10.2, KDE Frameworks 6.24.0, LayerShellQt 6.6.4, and GCC 15.2.0.

## Completed

| Check | Result |
| --- | --- |
| Clean Release configure/build | Passed in `build-release`; no compiler warnings. |
| Qt Test suite | 34 cases passed, including parsing, invalid data, rounding, threshold equality, timing, state transitions, stale readings, recovery, presentation, and tray placement. |
| Process integration | Passed on an isolated D-Bus session: production startup, duplicate launch, exported Quit menu, and harness normal/alert/recovery transitions. |
| Live Wayland integration | Passed with synthetic samples: one warning on KWin's above-normal layer, keyboard input disabled, original focused window unchanged, no minimize support, and automatic removal on recovery. |
| Tray-relative placement | Passed on the left-side tray: warning geometry `(401, 2246, 504, 88)` places it 12 pixels beyond the panel and centered vertically on the tray. Unit cases cover all four edges, screen offsets, screen-edge clamping, and unavailable layout data. |
| Live tray protocol | Passed: passive/active status, current tooltip, one Quit action, duplicate launches in both states, and successful Quit while alerting. |
| Desktop entries | Application and autostart entries passed `desktop-file-validate`. |
| Installation | Release executable and both desktop entries installed under the user's `.local` and `.config` directories. |
| Custom install paths | Custom XDG data/config directories and an executable path containing spaces passed staged install checks. |
| Uninstall | Removed only the three installed files in a temporary installation, preserving an unrelated file and shared directories. |
| Autostart generation | The installed entry generates one systemd user service with the correct absolute executable and KDE session condition. |

The release build's `ctest --test-dir build-release --output-on-failure` reports **2/2 test suites passed**. The live check is separate:

```sh
/usr/bin/python3 tests/session-smoke.py \
  "$HOME/.local/bin/kde-memory-alert" build-release/tests/memory-alert-harness --wayland
```

Quit the existing app and harness first. This check briefly shows synthetic warnings and stops its own processes afterward. No real memory pressure is generated. Temporary KWin inspection scripts are unloaded after each query.

Two defects found during validation were fixed: ordinary Qt window hints did not prevent focus stealing or provide above-normal stacking on this Wayland session, so the warning now uses LayerShellQt; explicit Quit initially could be vetoed by the persistent warning's close handler, so Quit now releases the warning before exiting.

## Manual confirmation still pending

- Visually confirm Plasma puts the normal icon in hidden items and the alert icon in the panel under its automatic visibility policy.
- Use the interactive harness while typing in another application to confirm the warning is readable, remains visible, and does not interrupt typing. Confirm tray interaction does not dismiss it. Automated checks verified focus and window state, but do not replace this user-facing check.
- At a convenient time, log out and back into KDE to verify one monitor starts automatically. Select Quit, then confirm it starts again at a later login. No logout was performed during implementation.

The implementation is ready for these checks. The corresponding TODO items remain open; a generated autostart service is not recorded as proof of a completed login cycle.
