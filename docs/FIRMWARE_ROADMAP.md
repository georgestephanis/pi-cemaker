# 💻 Pi-cemaker Firmware Roadmap & Implementation Plan

This document outlines the software architecture, toolchain, state machine, and development milestones for the **Pi-cemaker** RP2040 firmware.

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

### Phase 1: USB HID UPS Proof of Concept (No Custom Hardware Needed)
- **Goal:** Emulate a working UPS using a standard $4 Raspberry Pi Pico board.
- **Actions:**
  - Create the TinyUSB HID Power Device descriptor.
  - Synthesize a simulated battery discharging from 100% to 0%.
  - Plug Pico into a Raspberry Pi 5 USB port and verify that `upower`, desktop battery indicators, and `systemd-logind` clean shutdown work.

### Phase 2: Sensor & I2C Integration
- **Goal:** Connect I2C power monitoring (INA219/INA226 or MP2762A) to the Pico.
- **Actions:**
  - Implement I2C telemetry driver.
  - Calibrate battery voltage reading against precision multimeter.
  - Implement 2S discharge curve mapping.

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
