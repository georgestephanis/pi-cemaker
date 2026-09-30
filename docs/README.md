# 🫀 Pi-cemaker

**A minimalist, open-source 2× 18650 USB-C PD Uninterruptible Power Supply for Raspberry Pi 5**

> *"Keeps your Pi's heartbeat going when the mains flatline."*

---

## ⚡ Overview

**Pi-cemaker** is a streamlined, single-board DC Uninterruptible Power Supply (UPS) designed specifically for the Raspberry Pi 5 (and other 5V USB-C single-board computers). It sits inline between your USB-C PD power adapter and your Raspberry Pi, using **two standard 18650 Li-ion cells** (2S configuration) for seamless battery backup with **zero millisecond switchover delay**.

Unlike traditional complex DIY or commercial UPS designs that require multi-board stacks, proprietary microcontrollers, and custom host background daemons, **Pi-cemaker** is engineered for radical simplicity:

- **Single-Board Simplicity:** Everything (charger, protection, buck-boost converter, and MCU) lives on a single compact 2-layer or 4-layer PCB.
- **Native USB HID UPS (Plug & Play):** Emulates standard USB HID Power Device Class (`0x84` / `0x85`). Linux (`upower`, `systemd-logind`, `NUT`) natively recognizes it as a system battery—giving you desktop battery status and automated clean shutdown out of the box with **zero software to install**.
- **Ubiquitous Cells:** Uses two standard, user-replaceable 18650 cells in onboard clips (or an external 2S pack).
- **True 27W Power Delivery:** Supplies a solid 5V @ 5A (25W) to the Raspberry Pi 5 to prevent under-voltage throttling and allow full 1.6A downstream USB peripheral support.
- **Single RP2040 MCU:** Powered by the accessible, well-supported Raspberry Pi RP2040 (or RP2350), running open-source C/C++ firmware built on TinyUSB.

---

## 📐 System Architecture at a Glance

```text
USB-C PD In (9–20V)                                             Raspberry Pi 5
       │                                                               ▲
       ▼                                                               │ Single USB-C Cable
┌──────────────┐     Power Path      ┌─────────────────┐               │ (5V/5A Power +
│ USB-PD Sink  │────────────────────▶│ 5V / 5A (25W)   │───────────────┤  Native USB HID UPS)
│   (HUSB238)  │                     │ Buck-Boost DC-DC│               │
└──────┬───────┘                     └────────┬────────┘               │
       │                                      ▲                        │
       ▼                                      │                        │
┌──────────────┐    Charge / Discharge        │                        │
│  2S Charger  │◀─────────────────────────────┘                        │
│  (MP2762A or │                                                       │
│   IP2368)    │◀───▶ [ 18650 Cell 1 ] + [ 18650 Cell 2 ] (2S, 7.4V)   │
└──────┬───────┘                                                       │
       │ I2C Telemetry                                                 │
       ▼                                                               │
┌─────────────────────────────────────────────────────────────┐        │
│                 Single MCU (RP2040 / RP2350)                │◀───────┘
│  - Reads V_bat, I_bat, V_in, V_out, Temp                    │
│  - Exposes USB HID Power Device (UPS) + CDC serial          │
└─────────────────────────────────────────────────────────────┘
```

---

## 📑 Project Documentation

All design documentation lives in this `docs/` folder:

1. [**System Architecture (`docs/ARCHITECTURE.md`)**](ARCHITECTURE.md) — Detailed block diagram, power path analysis, state machine, and power domain mapping.
2. [**Hardware Design & Schematics (`docs/HARDWARE_DESIGN.md`)**](HARDWARE_DESIGN.md) — Component selection (Integrated vs Discrete), schematic design guidelines, battery clips, thermal considerations, and BOM.
3. [**USB HID UPS Specification (`docs/USB_HID_UPS_SPEC.md`)**](USB_HID_UPS_SPEC.md) — USB HID Power Device (`0x84`) & Battery System (`0x85`) descriptor layout, Linux `upower`/`NUT` integration, and auto-shutdown sequences.
4. [**Firmware Roadmap (`docs/FIRMWARE_ROADMAP.md`)**](FIRMWARE_ROADMAP.md) — Firmware architecture, TinyUSB stack setup, ADC filtering, state estimation, and test harness.

---

## 🛠️ Tech Specs

| Parameter | Specification | Notes |
|---|---|---|
| **Input Power** | USB-C PD (9V, 12V, 15V, 20V) | Negotiated via HUSB238 or integrated PD PHY |
| **Output Power** | 5.1V @ 5.0A (up to 25.5W) | Fixed high-power profile for Raspberry Pi 5 |
| **Battery Configuration** | 2S1P (2× 18650 Li-ion cells) | 7.2V–7.4V nominal, 8.4V max charge, 6.0V cutoff |
| **Switchover Time** | 0 ms (seamless) | Ideal diode / power-path topology |
| **Runtime** | ~1.5 to 3.5 hours | Based on typical Pi 5 loads (5–12W) with 3000mAh cells |
| **Microcontroller** | Raspberry Pi RP2040 | Dual ARM Cortex-M0+, 133MHz, native USB |
| **Host Protocol** | USB HID Power Device (Usage Page `0x84`) | Native kernel support on Linux / RPi OS / macOS |
| **Auxiliary Port** | Virtual USB-CDC Serial COM port | For raw telemetry, CLI diagnostics, & firmware updates |
| **Protection** | 2S BMS (Over-charge, over-discharge, short circuit) + Thermal | Hardware-level protection independent of MCU |

---

## 🚀 Getting Started with Development

To start developing in this repository:
- Review [`AGENTS.md`](../AGENTS.md) for coding conventions, safety rules, and architecture guidelines.
- Explore [`docs/HARDWARE_DESIGN.md`](HARDWARE_DESIGN.md) to evaluate the **Path A (Integrated IP2368)** vs. **Path B (Discrete MP2762A + Buck-Boost)** hardware choices.
- Explore [`docs/USB_HID_UPS_SPEC.md`](USB_HID_UPS_SPEC.md) to inspect the TinyUSB HID Power Device implementation.

---

## 📄 License
- **Hardware:** CERN-OHL-S v2 (Permissive Open Hardware)
- **Firmware:** MIT or GPL-3.0
