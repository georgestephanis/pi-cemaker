# 🔌 USB HID Power Device (UPS) Specification

This document specifies the USB interface implementation that allows **Pi-cemaker** to register natively as an Uninterruptible Power Supply (UPS) on Linux, Raspberry Pi OS, macOS, and Windows with **zero custom drivers or background software**.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This specification reflects an active prototype in development and is **not complete yet**. It is **certainly NOT for sale**.

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

### Implemented Reports Breakdown

#### Input Report 1: UPS Status (`REPORT_ID_UPS_STATUS = 0x01`, 7 bytes packed)

| Byte Offset | Usage Name | Usage ID | Type | Description |
|---|---|---|---|---|
| `Byte 0` | **Report ID** | — | Constant | `0x01` |
| `Byte 1` | **PresentStatus** | `0x84, 0x02` | Logical Collection | 8-bit status bitmask: |
| ├─ `Bit 0` | `ACPresent` | `0x85, 0xD0` | Bit | 1 if USB-C wall power is connected |
| ├─ `Bit 1` | `Charging` | `0x85, 0x44` | Bit | 1 if battery is actively charging |
| ├─ `Bit 2` | `Discharging` | `0x85, 0x45` | Bit | 1 if running on battery backup |
| ├─ `Bit 3` | `FullyCharged` | `0x85, 0x46` | Bit | 1 if battery reaches 100% |
| ├─ `Bit 4` | `BelowRemainingCapacityLimit`| `0x85, 0x42` | Bit | 1 if battery $\le 20\%$ (warning threshold) |
| ├─ `Bit 5` | `ShutdownImminent` | `0x84, 0x69` | Bit | 1 if battery $\le 5\%$ (initiates OS shutdown) |
| ├─ `Bit 6` | `BatteryPresent` | `0x85, 0xD1` | Bit | 1 if battery pack is connected |
| └─ `Bit 7` | `NeedReplacement / Fault` | `0x85, 0x4B` | Bit | 1 on over-temp ($>55^\circ\text{C}$) or cell fault |
| `Byte 2` | **RemainingCapacity** | `0x85, 0x66` | uint8_t | Battery charge remaining (0 to 100%) |
| `Bytes 3-4` | **RunTimeToEmpty** | `0x85, 0x68` | uint16_t | Estimated seconds of battery life remaining |
| `Bytes 5-6` | **Voltage** | `0x85, 0xBB` | uint16_t | Measured battery pack voltage in millivolts |

#### Feature Report 2: Static Device Configuration (`REPORT_ID_UPS_CONFIG = 0x02`, 5 bytes packed)

| Byte Offset | Usage Name | Usage ID | Type | Description |
|---|---|---|---|---|
| `Byte 0` | **Report ID** | — | Constant | `0x02` |
| `Bytes 1-2` | **ConfigVoltage** | `0x84, 0x40` | uint16_t | Nominal 2S battery pack voltage (7400 mV) |
| `Bytes 3-4` | **DesignCapacity** | `0x85, 0x67` | uint16_t | Nominal pack capacity (3000 mAh; 2S1P of 3000 mAh cells) |

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
