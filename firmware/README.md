# 💻 Pi-cemaker Firmware

This directory contains the dual-core RP2040 firmware for **Pi-cemaker**, implementing native USB HID Power Device (`0x84` / `0x85`) and composite CDC ACM virtual serial telemetry.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This firmware is in active development and is **not complete yet**. It is **certainly NOT for sale** or production deployment.

---

## 🏗️ Architecture Overview

- **Dual-Core Task Separation:**
  - **Core 0 (USB & Host Interface):** Runs the TinyUSB device stack (`tud_task`), emits standard USB HID Power Device reports (`ACPresent`, `Charging`, `Discharging`, `RemainingCapacity`, `RunTimeToEmpty`, `Voltage`, `ShutdownImminent`), and handles the CDC interactive command-line interface.
  - **Core 1 (Sensors & Power State Machine):** Runs high-rate ADC sampling (100 Hz) with Exponential Moving Average (EMA) filtering for `V_BAT`, `V_BUS_IN`, `V_OUT`, and cell temperature. Executes the 45-second zombie halt post-shutdown power cut sequence and deep dormant sleep.
- **Standards Compliant:** Strictly conforms to USB-IF HID Power Device Class v1.0. Recognized natively by Linux kernel (`upower`, `systemd-logind`, `NUT`), Raspberry Pi OS, macOS, and Windows without custom host software.

---

## 📂 Source Structure

```text
firmware/
├── CMakeLists.txt              # Builds RP2040 UF2 target and host test targets
├── pico_sdk_import.cmake       # Standard Pico SDK discovery
├── include/
│   ├── tusb_config.h           # TinyUSB configuration (HID + CDC)
│   ├── usb_descriptors.h       # HID Power Device report structures & endpoint defs
│   ├── telemetry.h             # ADC definitions, SoC calculations, and simulation hooks
│   ├── power_mgr.h             # State machine, GPIOs, and 45s zombie mitigation
│   └── cli.h                   # CDC serial command-line interface
├── src/
│   ├── main.c                  # Core 0 & Core 1 scheduling, TinyUSB callbacks
│   ├── usb_descriptors.c       # USB HID report descriptor and composite descriptors
│   ├── telemetry.c             # ADC filtering, 2S Li-ion OCV lookup, simulation engine
│   ├── power_mgr.c             # Power state transitions, 5V rail control, auto-reboot
│   └── cli.c                   # Serial terminal command parser
└── tests/
    ├── test_hid_and_power.c    # Unit tests for HID reports, SoC curves, & power manager
    ├── sim_cli_runner.c        # Interactive desktop CLI simulator
    └── mock/tusb.h             # TinyUSB mock header for host builds
```

---

## 🛠️ Building & Running

### 1. Host Tests & Simulation (macOS / Linux, No Hardware Required)

You can build and run the test suite and CLI simulator on your desktop using standard `cmake` and `clang`/`gcc`:

```bash
cd firmware
cmake -B build -S .
cmake --build build

# Run unit tests (verifies HID descriptors, SoC table, and 45s zombie prevention)
./build/pi_cemaker_test

# Run interactive CLI simulator
./build/pi_cemaker_cli
```

### 2. RP2040 Target Build (Pico SDK)

To generate the `.uf2` firmware file for flashing to the RP2040 board:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
cd firmware
cmake -B build -S .
cmake --build build
```

This generates `firmware/build/pi_cemaker_fw.uf2`.

### 3. Flashing to RP2040
1. Hold the **BOOTSEL** button on Pi-cemaker while plugging into your computer via USB-C.
2. Drag and drop `pi_cemaker_fw.uf2` onto the `RPI-RP2` mass storage drive.
3. The RP2040 will reboot immediately and enumerate as both a UPS Power Device and a Virtual COM port.
4. Future updates can be initiated without pressing buttons by sending the `reboot-bootloader` command over serial.

---

## ⌨️ Serial CLI Commands

Connect using any serial terminal (e.g. `screen /dev/cu.usbmodem* 115200` or `minicom`):

| Command | Description |
|---|---|
| `status` | Print real-time voltages, SoC%, charging state, temperature, and flags |
| `sim on` / `sim off` | Toggle software simulation mode |
| `sim ac <0\|1>` | Simulate AC wall power disconnect (`0`) or reconnect (`1`) |
| `sim soc <0-100>` | Force a specific battery State of Charge percentage |
| `shutdown` | Trigger clean host shutdown sequence (45s window before 5V rail cut) |
| `powercut` | Immediately drop 5.1V rail to test PMIC reset |
| `poweron` | Re-enable 5.1V output rail |
| `pulse-pwr` | Pulse open-drain gate to Raspberry Pi 5 `PWR_BTN` header |
| `reboot-bootloader` | Jump directly into RP2040 USB bootloader for flashing |
