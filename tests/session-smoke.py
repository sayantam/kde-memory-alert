"""Exercise real processes and exported tray menus. Use --wayland for a live KWin check.

Requires python3-dbus and python3-gi. CTest runs on a private D-Bus session.
The optional live check briefly shows synthetic warnings and unloads its own KWin probes.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

import dbus
import dbus.mainloop.glib
import dbus.service
from gi.repository import GLib


def wait_for(predicate, description, timeout=5):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        GLib.MainContext.default().iteration(False)
        result = predicate()
        if result:
            return result
        time.sleep(0.02)
    raise AssertionError(f"Timed out: {description}")


class Probe(dbus.service.Object):
    def __init__(self, bus, directory):
        self.name = dbus.service.BusName("io.github.sayantam.MemoryAlertProbe", bus)
        super().__init__(bus, "/Probe")
        self.bus = bus
        self.directory = directory
        self.result = None

    @dbus.service.method("io.github.sayantam.MemoryAlertProbe", in_signature="s", out_signature="")
    def Report(self, value):
        self.result = json.loads(str(value))

    def snapshot(self, pid):
        self.result = None
        script = self.directory / "probe.js"
        script.write_text('''
const windows = workspace.windowList().filter(w => w.pid === PID);
callDBus("io.github.sayantam.MemoryAlertProbe", "/Probe", "io.github.sayantam.MemoryAlertProbe", "Report",
    JSON.stringify({active: workspace.activeWindow ? workspace.activeWindow.internalId.toString() : null,
        windows: windows.map(w => ({active: w.active, layer: w.layer, minimized: w.minimized,
            minimizable: w.minimizable, wantsInput: w.wantsInput,
            geometry: {x:w.frameGeometry.x, y:w.frameGeometry.y, width:w.frameGeometry.width, height:w.frameGeometry.height}}))}));
'''.replace("PID", str(pid)))
        tag = f"memory-alert-test-{time.monotonic_ns()}"
        sid = self.bus.call_blocking("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting",
                                     "loadScript", "ss", (str(script), tag))
        try:
            dbus.Interface(self.bus.get_object("org.kde.KWin", f"/Scripting/Script{sid}"),
                           "org.kde.kwin.Script").run()
            return wait_for(lambda: self.result, "KWin probe response")
        finally:
            dbus.Interface(self.bus.get_object("org.kde.KWin", "/Scripting"),
                           "org.kde.kwin.Scripting").unloadScript(tag)


class TestTrayHost(dbus.service.Object):
    """Minimal tray watcher for the private headless test session."""

    def __init__(self, bus):
        self.name = dbus.service.BusName("org.kde.StatusNotifierWatcher", bus)
        super().__init__(bus, "/StatusNotifierWatcher")
        self.items = []

    @dbus.service.method("org.kde.StatusNotifierWatcher", in_signature="s", out_signature="")
    def RegisterStatusNotifierItem(self, service):
        self.items.append(str(service))

    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature="ss", out_signature="v")
    def Get(self, interface, name):
        return self.GetAll(interface)[name]

    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface):
        assert interface == "org.kde.StatusNotifierWatcher"
        return {"ProtocolVersion": dbus.Int32(0),
                "IsStatusNotifierHostRegistered": dbus.Boolean(True),
                "RegisteredStatusNotifierItems": dbus.Array(self.items, signature="s")}


def tray_for(bus, process):
    def locate():
        for name in bus.list_names():
            if not str(name).startswith(":"):
                continue
            try:
                pid = bus.call_blocking("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                        "org.freedesktop.DBus", "GetConnectionUnixProcessID", "s", (name,))
                if pid != process.pid:
                    continue
                obj = bus.get_object(name, "/StatusNotifierItem", introspect=False)
                properties = dbus.Interface(obj, "org.freedesktop.DBus.Properties")
                properties.Get("org.kde.StatusNotifierItem", "Status")
                return str(name), properties
            except dbus.DBusException:
                continue
        return None
    return wait_for(locate, "tray registration")


def quit_from_tray(bus, name, properties, process):
    menu_path = str(properties.Get("org.kde.StatusNotifierItem", "Menu"))
    menu = dbus.Interface(bus.get_object(name, menu_path), "com.canonical.dbusmenu")
    _, layout = menu.GetLayout(0, -1, dbus.Array([], signature="s"))
    actions = [item for item in layout[2] if item[1].get("visible", True)]
    assert len(actions) == 1, f"Unexpected menu: {layout}"
    assert str(actions[0][1].get("label")).replace("&", "") == "Quit"
    menu.Event(actions[0][0], "clicked", dbus.Int32(0, variant_level=1), dbus.UInt32(0))
    assert process.wait(timeout=5) == 0


def assert_duplicate_exits(command, process, env, log):
    duplicate = subprocess.run(command, env=env, stdout=log, stderr=log, timeout=5)
    assert duplicate.returncode == 0
    assert process.poll() is None, "Original instance exited after duplicate launch"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("application", type=Path)
    parser.add_argument("harness", type=Path)
    parser.add_argument("--wayland", action="store_true")
    args = parser.parse_args()
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    tray_host = TestTrayHost(bus) if not args.wayland else None
    env = dict(os.environ, QT_QPA_PLATFORM="wayland" if args.wayland else "offscreen")
    names = ("io.github.sayantam.kde-memory-alert", "io.github.sayantam.memory-alert-harness")
    for name in names:
        if bus.name_has_owner(name):
            raise SystemExit(f"Quit the existing {name} instance before running this test.")
    with tempfile.TemporaryDirectory(prefix="memory-alert-test-") as directory:
        directory = Path(directory)
        log_path = directory / "process.log"
        with log_path.open("w+") as log:
            processes = []
            try:
                command = [str(args.application.resolve())]
                production = subprocess.Popen(command, env=env, stdout=log, stderr=log)
                processes.append(production)
                wait_for(lambda: bus.name_has_owner(names[0]), "production singleton registration")
                name, properties = tray_for(bus, production)
                assert_duplicate_exits(command, production, env, log)
                quit_from_tray(bus, name, properties, production)
                print("PASS: production startup, duplicate launch, and exported Quit action", flush=True)

                sample_file = directory / "sample"
                sample_file.write_text("50")
                command = [str(args.harness.resolve()), "--sample-file", str(sample_file)]
                harness = subprocess.Popen(command, env=env, stdout=log, stderr=log)
                processes.append(harness)
                wait_for(lambda: bus.name_has_owner(names[1]), "harness singleton registration")
                name, properties = tray_for(bus, harness)
                status = lambda: str(properties.Get("org.kde.StatusNotifierItem", "Status"))
                tooltip = lambda: str(properties.Get("org.kde.StatusNotifierItem", "ToolTip")[3])
                wait_for(lambda: status() == "Passive", "initial passive tray")
                assert_duplicate_exits(command, harness, env, log)
                probe = Probe(bus, directory) if args.wayland else None
                initial = probe.snapshot(harness.pid) if probe else None
                if probe:
                    assert not initial["windows"], initial

                def set_sample(value, expected, text):
                    replacement = directory / "next-sample"
                    replacement.write_text(value)
                    replacement.replace(sample_file)
                    wait_for(lambda: status() == expected and text in tooltip(), f"sample {value}")

                set_sample("90.04", "Passive", "90.0%")
                set_sample("90.06", "Active", "90.1%")
                if probe:
                    alert = wait_for(lambda: (value if (value := probe.snapshot(harness.pid))["windows"] else None), "mapped alert")
                    window = alert["windows"][0]
                    assert len(alert["windows"]) == 1
                    assert not window["active"] and not window["wantsInput"], alert
                    assert not window["minimizable"] and not window["minimized"], alert
                    # KWin's AboveLayer is 3 (distinct from the wire protocol's LayerTop=2).
                    assert window["layer"] == 3, alert
                    assert initial["active"] == alert["active"], "Desktop focus changed during the check"
                    print("PASS: Wayland warning is above ordinary windows, noninteractive, and preserves focus", flush=True)
                    print("Warning geometry:", window["geometry"], flush=True)
                assert_duplicate_exits(command, harness, env, log)
                set_sample("95", "Active", "95.0%")
                set_sample("error", "Active", "Reading stale")
                set_sample("90", "Passive", "90.0%")
                if probe:
                    wait_for(lambda: not probe.snapshot(harness.pid)["windows"], "hidden warning on recovery")
                set_sample("error", "Passive", "Reading stale")
                set_sample("95", "Active", "95.0%")
                quit_from_tray(bus, name, properties, harness)
                print("PASS: rounded boundaries, updates, stale state, recovery, duplicate launches, and Quit while alerting", flush=True)
            except BaseException:
                log.flush()
                print(log_path.read_text())
                print("Bus names:", bus.list_names())
                print("Registered items:", tray_host.items if tray_host else "live host")
                raise
            finally:
                for process in processes:
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait()


if __name__ == "__main__":
    main()
