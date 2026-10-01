# AGENTS.md — Pi-cemaker

Guidance for AI agents and developers. Read **Status** first: most docs describe intent, not what exists.

## 1. Status (read this first)

Pi-cemaker is a prototype 2S (2× 18650) UPS for the Raspberry Pi 5, with an RP2040 that presents a USB HID Power Device plus CDC serial.

| Area | Reality |
|---|---|
| Firmware (`firmware/`) | Runs on the host as a simulator; 29 tests pass against a **mock** TinyUSB. The real RP2040 build has **never been built**. No I2C, no temperature read, fake current values, dormant sleep was removed. |
| Hardware docs (`hardware/*.md`) | Prose and ASCII art. No netlist. Several claims are wrong (see the issue list). |
| KiCad (`hardware/kicad/`) | Stub: outline and labels only. Probably does not open (`;` comments in `.kicad_pcb`). |
| Docs (`docs/`) | Aspirational. Some say "implemented" for things that are not. |
| Proof of concept | `hardware/POC_MODULAR.md`: hand-solderable build from commercial modules. The intended first step. |

The 2026-09 deep review is tracked as GitHub issues labelled `review` (#1–#19). **Check them before changing anything in the area, and do not copy claims from the docs into code or new docs without verifying them** (see §5).

```bash
gh issue list --label review
```

## 2. Repo map

```
docs/        README (symlinked from /README.md), ARCHITECTURE, HARDWARE_DESIGN, USB_HID_UPS_SPEC,
             FIRMWARE_ROADMAP, CRITICAL_CONSIDERATIONS
firmware/    CMakeLists.txt, include/, src/{main,usb_descriptors,telemetry,power_mgr,cli}.c,
             tests/{test_hid_and_power.c, sim_cli_runner.c, mock/tusb.h}
hardware/    SCHEMATICS.md, PCB_LAYOUT_GUIDELINES.md, BOM.{md,csv}, POC_MODULAR.md, kicad/
```

## 3. Commands

Host build and tests (no hardware, no Pico SDK):

```bash
cd firmware
cmake -B build -S . && cmake --build build
./build/pi_cemaker_test      # expect 22/22
./build/pi_cemaker_cli       # interactive CLI simulator
```

Device build needs `PICO_SDK_PATH`. **Untested**: the host test targets are also defined when the SDK is present, which will likely break that build (issue #18). Fix that before relying on the device build.

There is no CI, no linter config, and no KiCad ERC/DRC yet. `kicad-cli` is not installed on the dev machine.

## 4. Code map and gotchas

- `power_mgr.c`: 9-state machine ticked every 10 ms. `power_mgr_tick` re-asserts the 5 V enable every tick in most states, so manual `force_5v_enable` calls (CLI `powercut`) get overwritten (#2). Mains return during the 45 s countdown cancels the cut (#1).
- `telemetry.c`: OCV lookup on loaded terminal voltage (#4). Only 3 of 4 ADC channels are read. `BATTERY_DESIGN_CAP_MAH` is 6000 but the pack is **3000 mAh** (2S is series, capacities do not add).
- `usb_descriptors.c`: HID usage IDs are suspect (#5). Do not add features on top without checking the USB-IF tables.
- `main.c`: HID report built in two places; keep them in sync or refactor into one builder. Core 1 shares unsynchronised state with core 0 (#14 proposes removing core 1).
- Pin map lives in `power_mgr.h` and `telemetry.h`: GP15 EN, GP14 PWR_BTN, GP16/17/18 LEDs, GP19 button, GP26–29 ADC. The `POC_MODULAR` build inverts EN polarity.
- Host vs device code is split with `#if PICO_ON_DEVICE`. Keep host builds compiling and tests passing.

## 5. Working rules

**Safety invariants** (hold these even when simplifying):
- Never bypass hardware battery limits. Cutoff and protection thresholds belong to the BMS and charger, not to firmware alone.
- Default and reset state of the 5 V enable must be **rail ON** (fail-on). A hung or rebooting MCU must not drop the Pi's power.
- The Pi's 5 V rail must not dip during source switchover (the design goal is no glitch).
- No `malloc` in steady-state loops. Keep USB polling non-blocking.
- `ShutdownImminent` only when capacity is verified critical, with hysteresis (#4).

**Verify before you state.** This repo has already gone wrong from plausible but unchecked statements (wrong HID usage IDs, wrong Type-C current advertisement, wrong balancer part, `otg_mode`). For anything hardware-, USB-, or Pi-specific, check the datasheet or official doc and cite it in the commit/issue. Mark unverified claims `(verify)`.

**Doc hygiene.**
- Don't call something "implemented" or "completed" unless it runs on the target. Update `FIRMWARE_ROADMAP.md` when status changes.
- Pick one part per function. Remove the alternatives (BQ25792/IP2368/SC8721/HY2212 etc.) from docs rather than listing them.
- Don't hand-edit BOM numbers into both `BOM.md` and `BOM.csv`; the CSV is the source until the schematic can generate it.
- No real upower/NUT output is in the docs; label illustrative output as such.

**Change discipline.**
- Add a test for every firmware bug fixed. Existing tests mostly check struct sizes, so they prove little.
- Hardware: no layout work before schematic capture + ERC. Current direction is 2-layer, 1 oz, about 3–4 A output (#12, #19).
- Prefer deleting complexity to adding it. Open simplification proposals are in #13, #14, #19.

## 6. Decisions still open (see #19)

1. 3–4 A vs 5 A output, and what the Pi sees on its USB-C port (#7).
2. Plain buck vs buck-boost for 5 V.
3. Primary data link: USB-A host port (zero config) vs USB-C with the dwc2 overlay (#6).
4. Zombie halt: implemented as PWR_BTN pulse (#13); needs hardware validation and a safe "halted" detector.
5. One charger part: MP2762A (NVDC) vs alternatives.

## 7. References

USB-IF HID Usage Tables for Power Devices (pdcv10.pdf), Raspberry Pi docs on Pi 5 power/USB PD and OTG, MP2762A and HUSB238 datasheets, TinyUSB `hid_device.c`. Project docs: `docs/README.md`, `hardware/POC_MODULAR.md`.
