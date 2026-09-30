# 🧩 Pi-cemaker Proof of Concept: Commercial Modules on a Minimal Carrier

A hand-solderable first build that wires off-the-shelf modules together. It exists to validate the **firmware, the USB HID UPS behaviour, and the Pi 5 integration** before any custom power PCB is drawn. It is not the product design.

> [!CAUTION]
> **PROTOTYPE. Not reviewed by anyone but the author and an AI.** It works with a 2S Li-ion pack and up to ~4 A at 5 V. Use a fuse, a BMS board, and matched cells, and do not leave it unattended while charging. Module specs below are from memory or web search: **check each against its datasheet before buying or powering up.** Items marked **(verify)** are the ones most likely to be wrong.

---

## Why this shape

- **Float-charge topology.** The charger feeds the battery, the battery always feeds the 5 V converter. There is no power-path switchover to design, so no glitch on the Pi's rail. Cost: the battery is cycled while loaded, and the charger must supply *load + charge* current. Fine for a PoC. The custom board's NVDC charger (MP2762A) is what fixes this (see issue #8).
- **Data over USB-A, power separately.** Pico USB goes to a Pi USB-A host port: no `config.txt` edit, no dwc2 overlay (see issue #6). Power goes in on GPIO 5 V pins or a USB-C pigtail, so set `usb_max_current_enable=1` (issue #7).
- **Same GPIO map as the firmware**, so `firmware/` runs on a stock Pico with only the changes listed under *Firmware deltas*.

---

## Block diagram

```text
 USB-C PD charger ──▶ [M1 PD trigger, 12 V] ──▶ [F1 fuse] ──▶ [M2 CC/CV 2S charger, 8.4 V, ≤1.5 A]
                              │ (divider)                                   │
                              ▼                                             ▼
                        Pico GP27 (VBUS)                     [M3 2S BMS w/ balance] ◀──▶ [BH: 2× 18650]
                                                                            │ (pack+, GND)
                                        ┌───────────────────────────────────┤
                                        ▼                                   ▼
                              [F2 fuse 7.5 A]              (divider 100k/33k ▶ Pico GP26; optional M5 INA226)
                                        ▼
                              [M4 5 V 5 A buck, ENABLE] ──▶ (divider ▶ Pico GP28)
                                   ▲ EN_FET (Q1)  ◀── Pico GP15           │
                                        │                                  ▼
                                        └──▶ Pi 5: GPIO pin 2/4 (5 V) + pin 6 (GND)   [or USB-C pigtail]

 [M6 Raspberry Pi Pico] ── micro-USB ──▶ Pi 5 USB-A host port   (HID UPS + CDC)
 Pico GP14 ─▶ Q2 ─▶ Pi 5 J2 (power button header)     GP16/17/18 ─▶ 3 LEDs     GP19 ◀─ button
 Pico VSYS ◀── 5 V from M4 via Schottky D1 (see Power for the Pico)
```

---

## Module list (suggested parts, all **verify**)

| Ref | Function | Suggested | Notes |
|---|---|---|---|
| M1 | USB-C PD sink → fixed 12 V | Adafruit HUSB238 breakout (I2C, STEMMA QT) or any 12 V PD trigger board | HUSB238 defaults high unless strapped; 12 V is plenty and keeps the charger efficient. The I2C-capable breakout lets firmware later read the contract. |
| M2 | 2S CC/CV charger | Adjustable CC/CV buck module (XL4015-class, ≥5 A rating) set to **8.40 V**, current limit **1.0–1.5 A** | Set both *before* connecting the pack, using a meter. It must tolerate load + charge current. It has no charge termination; CV at 8.40 V plus the BMS is the protection. A dedicated 2S charger board with termination is better if you can find one. |
| M2-alt | Validates the final chip | MPS EV2762A-V-00A eval board (MP2762A, I2C, preset 8.4 V) | Costs more but tests the real part and gives SYS/telemetry over I2C. |
| M3 | 2S protection + balance | 2S 10 A BMS board with balance (HX-2S-JH20 style: HY2120 protection + 2× HY2213 balancers) | Buy a board rated for ≥10 A continuous. Check the overcharge threshold is ≤4.25 V/cell. Wire B-, B1 (mid), B+ exactly as the board's silkscreen shows. |
| BH | Cell holders | Two single 18650 holders, series-wired (or a 2S holder) | Use mechanically keyed holders. Matched, same-batch cells only. |
| F1, F2 | Fuses | F1 3 A on input, F2 7.5 A on pack | Inline blade fuse holders are fine. |
| M4 | 5 V buck with enable | Pololu D36V50F5 (5 V, 5.5 A, 6–50 V in, ENABLE pin) or equivalent | **(verify)** minimum input voltage and dropout at a low pack (6 V), and ENABLE logic levels. Output is 5.0 V, not 5.1 V. Raise shutdown threshold so the pack never sits near 6 V under load (issue #4). |
| M5 | Battery current/voltage (optional) | INA226 or INA219 breakout, 10–20 mΩ shunt in pack return | Gives real current for SoC/runtime. Put the shunt on the low side of the pack, after the BMS. I2C to Pico GP4/GP5. |
| M6 | MCU | Raspberry Pi Pico (RP2040) | Matches the firmware's Pico SDK build. |
| Q1 | Enable pull-down | 2N7002 / BSS138 | See *EN wiring*. |
| Q2 | Pi power-button pull-down | 2N7002 / BSS138 | Drain to Pi J2 pin, source to GND. |
| D1 | Pico power | Schottky (e.g. SS34) | From M4 5 V to Pico VSYS. |
| R | Dividers, LEDs | 100 k / 33 k (pack), 100 k / 15 k (VBUS), 100 k / 100 k (VOUT); 3× LED + 1 k; 100 nF on each ADC pin | 1% is plenty for a PoC. Large dividers need the 100 nF cap at the ADC pin. |

---

## Wiring (netlist level)

### Power

| Net | From | To |
|---|---|---|
| `VIN12` | M1 out+ | F1 → M2 in+ ; VBUS divider top |
| `GND` | M1 out−, M2, M3 P−/B−, M4, M6 GND, Pi GND | star at the BMS P− / shunt |
| `PACK+` | M3 P+ | F2 → M4 in+ ; VBAT divider top |
| `PACK−` | M3 P− | (shunt M5) → `GND` |
| charger out | M2 out+ → M3 **C+**/P+ as the BMS board specifies (charge and discharge share P+/P− on most 2S boards) | M2 out− → P− |
| `5V_OUT` | M4 out+ | Pi GPIO pin 2 **and** pin 4 (5 V), M4 out− → Pi pins 6 and 9 (GND). Use ≥18 AWG, twist the pair, keep it short. |

Note 5 V into the GPIO header bypasses the Pi's PD/PMIC input and its fuse/reverse protection. Set `usb_max_current_enable=1`.

### Power for the Pico

Feed Pico VSYS from `5V_OUT` through D1. Do **not** also let Pico USB back-feed M4's output: the Pico's own VBUS diode isolates USB from VSYS. Do not connect the Pi's USB 5 V to anything else, but the Pico powered from both is the normal Pico use case.

### Pico pin map (same as `power_mgr.h` / `telemetry.h`)

| Pico GPIO | Signal | Connects to |
|---|---|---|
| GP26 / ADC0 | `V_BAT` | `PACK+` 100 k / 33 k divider (+100 nF) |
| GP27 / ADC1 | `V_BUS_IN` | `VIN12` 100 k / 15 k divider (+100 nF) |
| GP28 / ADC2 | `V_OUT` | `5V_OUT` 100 k / 100 k divider (+100 nF) |
| GP29 | (not on Pico header) | NTC: move to another ADC-capable pin or drop for PoC, see note |
| GP15 | `EN_5V` | Q1 gate (see *EN wiring*) |
| GP14 | `PWR_BTN` | Q2 gate → Pi J2 |
| GP16/17/18 | LEDs | via 1 k to LEDs |
| GP19 | User button | to GND, internal pull-up |
| GP4/GP5 | I2C0 | M5 (INA226), optionally M1 |

**GP29 note:** on a stock Pico, GP29 is the VSYS/3 sense and is not broken out as a free ADC pin. Drop the NTC for the PoC (taping a NTC to the cells and reading it on a spare ADC pin is optional) or use an RP2040 board that exposes all four ADC channels.

### EN wiring (important)

M4's ENABLE is typically pulled up to its **input** voltage (pack voltage, up to 8.4 V) **(verify)**. A Pico GPIO must never see that. Use Q1:

```text
M4 ENABLE ──── Q1 drain          Q1 source ── GND          Q1 gate ◀── GP15 (100 k pull-down on gate)
```

- Q1 **off** = M4 enabled (default, also the state while the Pico is in reset or BOOTSEL: **fail-on**).
- Drive GP15 **high** = Q1 on = ENABLE pulled low = 5 V **cut**.
- This inverts the firmware's `PIN_5V_EN` polarity. See *Firmware deltas*.

### Pi power button

Q2 drain to the Pi 5 J2 header pin that goes to the button (other pin to GND). GP14 high → Q2 on → button "pressed". Pulse about 500 ms.

---

## Minimal carrier PCB

The "minimal PCB" is optional. Modules can be wired with flying leads for the first power-up. A carrier just tidies the signal side and keeps the Pico, two MOSFETs, dividers and headers in one place.

- **2 layers, 1 oz, ~50 × 40 mm.** Only signals and the Pico live on it. **No power current flows on the carrier.** All power modules wire to screw terminals or pin headers off-board and their cables carry the amps.
- Carrier contents: Pico socket (2× 20-pin headers), Q1, Q2, D1, three dividers with caps, LEDs, button, JST-SH/pin headers for INA226 and PD-breakout I2C, and a 2-pin header each for EN and PWR_BTN.
- Bottom layer: solid GND pour. Everything is hand-solderable (0805 / through-hole, SOT-23 for the two FETs).
- Power-stage wiring stays off the PCB deliberately: it removes every layout risk that drove the 4-layer conclusion in the full design.

A KiCad schematic for the carrier is tracked in a follow-up issue. It should be captured and ERC-checked in KiCad rather than written by hand.

---

## Firmware deltas for the PoC

Small, and to be done behind a build flag (`POC_MODULAR`):

1. `PIN_5V_EN` becomes **active-high = cut** (inverted, open-drain behaviour via Q1). Default and reset state = rail on.
2. Drop the `ADC_PIN_TEMP` read (no GP29).
3. No MP2762A/HUSB238 I2C yet; optional INA226 driver for battery current.
4. Shutdown threshold must be raised before trusting a low-battery shutdown on this hardware (issue #4), and the zombie/power-cut logic should follow issues #1 and #13 before relying on it.

---

## Bring-up order

1. **Bench each module alone** with a meter: M1 → 12 V; M2 set to 8.40 V (no pack) and current limit; M4 output 5.0 V with a dummy load.
2. **BMS + pack** with no load: check the mid-tap and pack voltages with the pack in place and the BMS balance leads connected in the correct order.
3. Connect M2 → BMS/pack and watch charge current and temperature for 10–15 min.
4. Connect M4 to a **dummy load** (resistor/electronic load), not the Pi. Ramp 0 → 3 A, then 4 A. Watch the BMS and M4 temperature.
5. Power-loss test: pull M1's input while under load and scope `5V_OUT` for dips. Nothing should move (battery is always in the path).
6. Flash the firmware, check the CLI, then connect the Pico USB to the Pi and look at `lsusb` and the HID descriptor (issue #5).
7. Connect the Pi last. Set `usb_max_current_enable=1` and `POWER_OFF_ON_HALT=1`.

## Known limitations

- Battery is cycled while loaded, which wears it faster. Charger must supply load plus charge, so a mains brown-out to 9 V PD output may not sustain 4 A load plus charging. Limit charge current.
- No true charge termination on the CC/CV buck option.
- 5 V (not 5.1 V) output with GPIO-header power: watch cable drop and Pi undervoltage warnings.
- Not a safe design for unattended or enclosed use.
