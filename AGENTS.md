# AGENTS.md — Pi-cemaker

This document provides operational context, architectural reference, build procedures, and coding guidelines for AI coding agents and developers working in this repository.

---

## 1. Project Overview & Architecture

**Pi-cemaker** is an open-source, minimalist 2× 18650 DC Uninterruptible Power Supply (UPS) for Raspberry Pi 5:
- **Battery Subsystem:** 2S1P Li-ion (2× 18650 cells, 7.4V nominal, 8.4V max charge, 6.0V cutoff) with hardware protection (HY2120-LB).
- **Power Path:** Seamless 0ms switchover between USB-C PD input (9–20V) and battery power.
- **Output:** 5.1V @ 5A (25.5W) continuous via high-efficiency synchronous buck-boost converter.
- **Microcontroller:** Single RP2040 (or RP2350).
- **Host Link:** Single USB-C cable delivering both 5V power and USB data.
- **Host Protocol:** Standard USB-IF **HID Power Device Class** (Usage Page `0x84` Power Device / `0x85` Battery System) with composite USB-CDC serial. Linux (`upower`, `systemd-logind`, `NUT`) auto-detects it without any custom host daemons.

---

## 2. Repository Layout

```
.
├── docs/                     # Core documentation & engineering specs
│   ├── README.md             # Primary project README
│   ├── ARCHITECTURE.md       # Power domains, block diagram, telemetry
│   ├── HARDWARE_DESIGN.md    # Schematic guidance, components, BOM, PCB layout
│   ├── USB_HID_UPS_SPEC.md   # HID report descriptor & Linux integration spec
│   ├── FIRMWARE_ROADMAP.md   # Firmware architecture, TinyUSB stack, milestones
│   └── CRITICAL_CONSIDERATIONS.md # Edge cases, zombie halt reboot, cell balance
├── firmware/                 # RP2040 C/C++ firmware (Pico SDK / TinyUSB)
│   ├── CMakeLists.txt        # Build system (RP2040 UF2 & host tests)
│   ├── include/              # Headers (tusb_config, descriptors, telemetry, power_mgr, cli)
│   ├── src/                  # Sources (main, descriptors, telemetry, power_mgr, cli)
│   └── tests/                # Host unit tests and interactive CLI simulator runner
├── hardware/                 # Circuit schematics, BOM, PCB layout guidelines, KiCad
│   ├── SCHEMATICS.md         # 6 detailed subsystem schematics and calculations
│   ├── BOM.md / BOM.csv      # Complete Bill of Materials with MPNs
│   ├── PCB_LAYOUT_GUIDELINES.md # 4-layer stackup & 5A thermal guidelines
│   └── kicad/                # KiCad 7/8 project, schematic, and board layout
├── README.md                 # Symlink to docs/README.md
├── AGENTS.md                 # Agent instructions and architectural invariants
└── .gitignore
```

---

## 3. Key Development Guidelines

### Embedded Safety & Power Protection
- **Battery Safety:** Never override or bypass hardware battery thresholds (max 8.4V charge, min 6.0V discharge cutoff).
- **5V Rail Stability:** The 5.1V output to the Raspberry Pi 5 must never experience drops below 4.75V during power source switchover; otherwise the Raspberry Pi 5 PMIC will trigger an under-voltage restart.
- **No Dynamic Memory in Real-Time Loops:** Do not use `malloc` / `new` inside steady-state USB polling or ADC filtering loops.
- **Zombie Halt Power-Cycle:** Ensure the post-shutdown power cut timer (45 seconds after `ShutdownImminent`) drops the 5V rail completely so the Pi 5 will auto-boot when mains power returns.
- **RP2040 Dormant Sleep:** Transition the MCU into dormant sleep (<100µA) once the battery is depleted to avoid destructive parasitic discharge.

### USB HID UPS Compliance
- Strictly follow the USB HID Power Device usage tables defined in [`docs/USB_HID_UPS_SPEC.md`](docs/USB_HID_UPS_SPEC.md).
- Ensure `ShutdownImminent` is only asserted when battery capacity is verified critical (<5%) to prevent false shutdowns.
- Keep USB polling tasks non-blocking to prevent USB bus timeouts.

---

## 4. Documentation References
- Overview & Specs: [`docs/README.md`](docs/README.md)
- Power Architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- Hardware Design: [`docs/HARDWARE_DESIGN.md`](docs/HARDWARE_DESIGN.md)
- HID Power Device Spec: [`docs/USB_HID_UPS_SPEC.md`](docs/USB_HID_UPS_SPEC.md)
- Firmware Plan: [`docs/FIRMWARE_ROADMAP.md`](docs/FIRMWARE_ROADMAP.md)
- Critical Considerations & Edge Cases: [`docs/CRITICAL_CONSIDERATIONS.md`](docs/CRITICAL_CONSIDERATIONS.md)
