# 🏗️ Pi-cemaker System Architecture

This document specifies the electrical architecture, power domains, switchover dynamics, and communication interfaces for **Pi-cemaker**.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This architecture document reflects an active prototype in development and is **not complete yet**. It is **certainly NOT for sale**.

---

## 1. High-Level Design Principles

1. **Radical Simplification:** Collapse multi-MCU architectures (CH32X + RP2040 + ESP32) into a **single RP2040** (or RP2350) microcontroller.
2. **Zero-Glitch Power Switchover:** Provide true online or seamless offline DC backup. When USB-C input power drops, the battery must take over within microseconds with zero voltage sag on the 5V rail to prevent the Raspberry Pi 5 PMIC from tripping.
3. **Standard USB-IF HID Power Device Class:** Avoid proprietary daemons and custom binary protocol drivers. The UPS reports state using standard USB HID usage tables (`0x84` Power Device / `0x85` Battery System) so standard OS utilities (`upower`, `systemd-logind`, `NUT`) work out of the box.
4. **Standard 2S Li-ion (18650):** 2 series 18650 cells provide 6.0V–8.4V. This is above the 5.0V output rail, allowing high-efficiency synchronous buck operation during battery discharge, or narrow-range buck-boost.

---

## 2. Power Path Architecture

### The Power Flow Topology

```text
[USB-C Charger] (9V-20V PD)
       │
       ▼
 [HUSB238 / Input PD Sink]
       │
       ▼
 [V_BUS_IN: 9-20V] ───────────────┬───────────────────────────────┐
                                  │                               │
                                  ▼                               ▼
                           [Ideal Diode OR /]             [2S Battery Charger]
                           [Power Path Switch]            (MP2762A / BQ25792)
                                  │                               │
                                  │                               ▼
                                  │                      [2S 18650 Battery Pack]
                                  │                      (6.0V - 8.4V, with 2S BMS)
                                  │                               ▲
                                  │                               │
                                  ▼ (Discharge Diode/FET)─────────┘
                           [V_SYS: 6.0V - 20V]
                                  │
                                  ▼
                         [Synchronous Buck-Boost]
                           (TPS55289 / SC8721)
                                  │
                                  ▼
                           [V_OUT: 5.1V @ 5A]
                                  │
                                  ▼
                           [Raspberry Pi 5]
```

### Power Domains

1. **`V_BUS_IN` (Input Domain):**
   - 9V to 20V DC from external USB-PD charger.
   - Negotiated via HUSB238 I2C/standalone pin strapping.
   - Protected by reverse-voltage and over-voltage input MOSFETs.

2. **`V_BAT` (Battery Domain):**
   - 2S Li-ion battery (nominal 7.4V, charged 8.4V, cutoff 6.0V).
   - Capacity: typically 2× 3000–3500 mAh (total energy: ~22–26 Wh).
   - Protected by onboard 2S hardware protection circuit (HY2120-LB or similar).

3. **`V_SYS` (Internal System Rail):**
   - Powered by `V_BUS_IN` when wall power is present.
   - Seamlessly powered by `V_BAT` when wall power is removed.
   - Voltage range: ~6.0V to 20V depending on source.

4. **`V_OUT` (Regulated Output Rail):**
   - Regulated strictly to **5.10V ± 1.5%** at up to **5.0A continuous (25.5W)**, with 6.0A peak capability.
   - Powers the Raspberry Pi 5 via a captive or standard USB-C cable.

5. **`V_MCU_3V3` (Logic Rail):**
   - 3.3V generated from `V_SYS` via low-quiescent LDO or miniature buck (e.g., RT9193 or AP63200) to ensure the RP2040 and sensors stay powered during both charging and discharging.

---

## 3. Microcontroller Architecture (RP2040)

The RP2040 is the central nerve center for Pi-cemaker:

```text
                            ┌─────────────────────────────────┐
                            │          RP2040 MCU             │
                            │                                 │
 [USB-C to Pi 5 Data] ◀────▶│ USB D+/D- (TinyUSB HID + CDC)   │
                            │                                 │
 [I2C0: GPIO4/5]      ◀────▶│ I2C Master                      │
                            │   ├── Charger (MP2762A)         │
                            │   └── Stemma QT / Expansion     │
                            │                                 │
 [ADC0 / GPIO26]      ◀─────│ Battery Voltage Sense (V_BAT)   │
 [ADC1 / GPIO27]      ◀─────│ Input Voltage Sense (V_BUS_IN)  │
 [ADC2 / GPIO28]      ◀─────│ Output Voltage Sense (V_OUT)   │
 [ADC3 / GPIO29]      ◀─────│ NTC Thermistor Sense (TEMP)     │
                            │                                 │
 [GPIO15 Output]      ──────▶│ 5V Rail Enable / Power Cycle   │
 [GPIO14 Output]      ──────▶│ Pi 5 Power Button (Open-Drain) │
 [GPIO16/17/18 Out]   ──────▶│ Status LEDs (Power, Bat, Fault)│
                            │                                 │
 [GPIO19 Input]       ◀─────│ User Button (Tactile Switch)   │
                            └─────────────────────────────────┘
```

---

## 4. Communication & Control Topology

### 1. USB Connection to Host (Raspberry Pi 5)
- Connected via the **same USB-C cable** that carries 5V power to the Pi (using USB 2.0 D+/D- lines).
- Exposes two endpoints via composite USB:
  - **Interface 0 (HID):** USB HID Power Device class. Carries standardized telemetry reports (`ACPresent`, `Charging`, `RemainingCapacity`, `RunTimeToEmpty`, etc.).
  - **Interface 1 (CDC ACM):** Virtual Serial COM port for telemetry logging, debugging, firmware flash triggers, and configuration commands.

### 2. Autonomous Operation
- The power path is **hardware-managed**. If the RP2040 is halted, in bootloader mode, or frozen, the power path still works autonomously without dropping power to the Raspberry Pi.
