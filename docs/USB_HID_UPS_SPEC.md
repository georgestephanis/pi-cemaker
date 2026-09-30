# 🔌 USB HID Power Device (UPS) Specification

This document specifies the USB interface implementation that allows **Pi-cemaker** to register natively as an Uninterruptible Power Supply (UPS) on Linux, Raspberry Pi OS, macOS, and Windows with **zero custom drivers or background software**.

---

## 1. Why USB HID Power Device?

Traditional microcontroller UPS projects require writing a custom serial protocol, compiling a host daemon (in Python, Go, or Rust), and setting up custom `systemd` service units.

By implementing the official **USB Implementers Forum (USB-IF) HID Power Device Class**, Pi-cemaker achieves:
1. **Immediate OS Recognition:** The Linux kernel (`hid-generic` / `hid-ups`) detects the device instantly upon connection.
2. **Desktop Integration:** Raspberry Pi OS / GNOME / KDE automatically display a native battery percentage icon in the system tray.
3. **Automated Clean Shutdown:** `upower` and `systemd-logind` natively monitor `ShutdownImminent` and low battery percentages, triggering an orderly filesystem sync and graceful shutdown before power drops.
4. **Standard Tooling:** Fully compatible with **NUT (Network UPS Tools)** via the standard `usbhid-ups` driver for server automation.

---

## 2. USB Descriptor Architecture

Pi-cemaker enumerates as a **Composite USB Device**:
- **Interface 0:** Human Interface Device (HID) — Power Device & Battery System.
- **Interface 1 & 2:** Communication Device Class (CDC ACM) — Virtual Serial COM Port for debug logs and CLI configuration.

```text
[Pi-cemaker Composite USB Device]
       │
       ├── Interface 0: USB HID (Usage Page 0x84 Power Device / 0x85 Battery System)
       │       └── IN Endpoint 1 (Interrupt, 10ms polling interval)
       │
       └── Interface 1 & 2: USB CDC ACM (Virtual Serial COM Port)
               ├── IN Endpoint 2 (Bulk Serial TX)
               └── OUT Endpoint 2 (Bulk Serial RX)
```

---

## 3. HID Report Descriptor Structure

The HID report descriptor uses two standardized usage pages:
- **Usage Page `0x84`:** Power Device
- **Usage Page `0x85`:** Battery System

### Essential Usage Items Implemented

| Usage Name | Usage ID | Type | Description |
|---|---|---|---|
| **PowerDevice** | `0x84, 0x01` | Application | Declares the top-level UPS device |
| **PresentStatus** | `0x84, 0x02` | Named Array / Bitmap | Bitmask of current power states |
| ├─ `ACPresent` | `0x85, 0xD0` | Bit | 1 if wall power (USB-C in) is connected |
| ├─ `Charging` | `0x85, 0x44` | Bit | 1 if battery is actively charging |
| ├─ `Discharging` | `0x85, 0x45` | Bit | 1 if running on battery backup |
| ├─ `FullyCharged`| `0x85, 0x46` | Bit | 1 if battery reaches 100% |
| ├─ `BelowRemainingCapacityLimit` | `0x85, 0x42` | Bit | 1 if battery is below warning threshold (e.g., 20%) |
| └─ `ShutdownImminent` | `0x84, 0x69` | Bit | 1 if battery is critically low (triggers OS shutdown) |
| **RemainingCapacity** | `0x85, 0x66` | Dynamic Value | Battery charge remaining (0 to 100%) |
| **RunTimeToEmpty** | `0x85, 0x68` | Dynamic Value | Estimated seconds of battery life remaining |
| **Voltage** | `0x85, 0xBB` | Dynamic Value | Measured battery pack voltage in millivolts |
| **ConfigVoltage** | `0x84, 0x40` | Static Value | Nominal battery pack voltage (7400 mV for 2S) |

---

## 4. Host OS Behavior & Verification

### 1. Verification on Raspberry Pi OS via `upower`

When plugged into a Raspberry Pi 5, running `upower -e` reveals:
```bash
$ upower -e
/org/freedesktop/UPower/devices/ups_hiddev0
```

Inspecting the device with `upower -i /org/freedesktop/UPower/devices/ups_hiddev0`:
```text
  native-path:          /sys/devices/platform/axi/.../hiddev0
  vendor:               Web3 Pi
  model:                Pi-cemaker 2S UPS
  power supply:         yes
  updated:              Wed 30 Sep 2026 03:00:00 PM EDT (2 seconds ago)
  has history:          yes
  has statistics:       yes
  ups
    present:             yes
    state:               charging
    warning-level:       none
    energy:              24.5 Wh
    energy-empty:        0 Wh
    energy-full:         25.2 Wh
    percentage:          97%
    capacity:            100%
```

### 2. Automated Shutdown Sequence (`systemd-logind`)

1. **Mains Power Loss:** `ACPresent` drops to 0, `Discharging` sets to 1.
2. **Warning Level:** Battery percentage drops below 20%. `BelowRemainingCapacityLimit` sets to 1. Desktop sends user notification.
3. **Critical Threshold:** Battery percentage drops below 5%. Pi-cemaker asserts `ShutdownImminent = 1`.
4. **Graceful Action:** `systemd-logind` receives the signal and executes:
   ```text
   Power key / Shutdown imminent detected: Initiating system poweroff...
   ```
   Filesystems sync cleanly, avoiding database or SD card corruption.
