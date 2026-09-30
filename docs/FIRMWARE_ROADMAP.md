# 💻 Pi-cemaker Firmware Roadmap & Implementation Plan

This document outlines the software architecture, toolchain, state machine, and development milestones for the **Pi-cemaker** RP2040 firmware.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This firmware roadmap reflects an active prototype in development and is **not complete yet**. It is **certainly NOT for sale**.

---

## 1. Toolchain & Framework Selection

- **Microcontroller:** Raspberry Pi RP2040 (Dual ARM Cortex-M0+ @ 133 MHz).
- **Core SDK:** Official **Raspberry Pi Pico C/C++ SDK** with **CMake** (or PlatformIO with `arduino-pico`).
- **USB Stack:** **TinyUSB** (`tinyusb_device`), utilizing native composite device descriptors for:
  - Custom HID class with Power Device usage page.
  - Standard CDC class for debugging and CLI commands.

---

## 2. Firmware Architecture & Dual-Core Allocation

```text
 ┌──────────────────────────────────────┐  ┌──────────────────────────────────────┐
 │               Core 0                 │  │               Core 1                 │
 │      (USB & Host Communication)      │  │        (Sensors & State Machine)     │
 ├──────────────────────────────────────┤  ├──────────────────────────────────────┤
 │ - TinyUSB Device Task (tud_task)     │  │ - 100 Hz ADC Oversampling & Filter  │
 │ - HID UPS Report Generator           │  │ - I2C Polling (MP2762A / INA219)     │
 │ - CDC Serial Command Line Interface  │  │ - Battery State of Charge (SoC) Calc │
 │ - Watchdog Feeding                   │  │ - Power Path Safety Supervision      │
 └──────────────────┬───────────────────┘  └──────────────────┬───────────────────┘
                    │                                         │
                    └─────────────── Inter-core FIFO ─────────┘
                                  (FreeRTOS or Pico SDK)
```

### Core 0: USB & Protocol Handling
- Runs `tud_task()` to maintain USB enumeration responsiveness.
- Formats and sends HID Input Reports whenever battery state changes or on 1000ms heartbeat intervals.
- Handles user CLI interaction over CDC serial (e.g. typing `status`, `calibrate`, `reboot`).

### Core 1: Telemetry & Battery Management
- **ADC Sampling:** Reads battery voltage, output voltage, input voltage, and temperature at 100 Hz with exponential moving average (EMA) noise filtering.
- **State of Charge (SoC) Algorithm:**
  - Open-circuit voltage (OCV) lookup table for 2S Li-ion.
  - Optional Coulomb counter integration when current sensor (INA219 or MP2762A ADC) is active.
- **Safety Watchdog:** Monitors over-temperature (>55°C) and under-voltage (<6.0V), triggering load shed or alarm states if necessary.

---

## 3. Development Milestones

### Phase 1: USB HID UPS & Core Firmware Implementation (Completed ✅)
- **Goal:** USB-IF HID Power Device and CDC serial interface with full state machine and desktop simulation.
- **Completed Actions:**
  - Implemented TinyUSB HID Power Device descriptor (`0x84`/`0x85`) and composite CDC ACM.
  - Implemented software simulation engine for automated battery discharge/recharge and AC loss testing.
  - Implemented 45-second zombie halt mitigation state machine in `power_mgr.c`.
  - Implemented interactive CDC CLI (`status`, `sim`, `shutdown`, `powercut`, `pulse-pwr`, `reboot-bootloader`).
  - Implemented unit test suite (`test_hid_and_power.c`) passing 22/22 tests on host.
  - See [`firmware/README.md`](../firmware/README.md) for build and simulation details.

### Phase 2: Sensor & Hardware Telemetry (In Progress 🟡)
- **Goal:** Real-time ADC oversampling and I2C power monitoring integration on prototype hardware.
- **Actions:**
  - Implemented 100 Hz ADC sampling and Exponential Moving Average (EMA) filtering for `V_BAT`, `V_BUS_IN`, `V_OUT`, and NTC thermistor.
  - Implemented 2S Li-ion open-circuit voltage (OCV) State-of-Charge (SoC) lookup curve.
  - Wire I2C driver to MP2762A registers on prototype board.

### Phase 3: Hardware Bring-Up & Power Path Validation
- **Goal:** Flash firmware onto the first Pi-cemaker prototype PCB.
- **Actions:**
  - Validate 0ms switchover from USB-C input to battery under full 5A load.
  - Validate MP2762A 2S charge cycle termination.
  - Verify thermal performance under continuous 25W load.

### Phase 4: Non-Volatile Storage & Production Polish
- **Goal:** Flash persistence and user customization.
- **Actions:**
  - Save calibration offsets and user shutdown percentage thresholds into RP2040 Flash.
  - Add bootloader jump command (`reboot-bootloader`) over CDC serial for zero-touch firmware updates.
