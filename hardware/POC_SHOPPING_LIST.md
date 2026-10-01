# PoC Shopping List and Carrier PCBA Spec

Companion to [`POC_MODULAR.md`](POC_MODULAR.md). Everything here is **unverified**: part numbers are from memory, and no module has been checked against its datasheet. Do the datasheet checks in the last section before ordering. No prices are given because I can't look them up reliably.

## 1. Fastest path: buy modules, no custom PCB

| # | Item | Qty | What to look for |
|---|---|---|---|
| 1 | Raspberry Pi Pico (RP2040), headers optional | 1 | Plain Pico, not Pico W (the W's wireless chip is unused) |
| 2 | USB-C PD trigger board, **12 V fixed** | 1 | Selectable 12 V output, ≥3 A. Adafruit's HUSB238 breakout is the I2C-capable option |
| 3 | Adjustable CC/CV buck charger module | 1 | ≥5 A rated, voltmeter-able, set to **8.40 V** and **1.0–1.5 A** with a meter before connecting cells |
| 4 | 2S Li-ion BMS with balancing, **10 A+** | 1 | HY2120-type protection, balance leads, overcharge ≤4.25 V/cell |
| 5 | 5 V step-down, **5 A+**, with ENABLE pin | 1 | Pololu D36V50F5 is the candidate; check minimum input and dropout |
| 6 | 18650 cells, matched pair, protected cells not needed | 2 | Same brand, batch and charge level. Genuine cells only |
| 7 | 18650 holders (keyed, 5 A rated) | 2 | Or one 2S holder |
| 8 | INA226 (or INA219) breakout + 10–20 mΩ shunt | 1 | Optional: needed only for current/runtime |
| 9 | Fuses: 3 A input, 7.5 A pack, with inline holders | 2 | Blade fuses are easy |
| 10 | 2N7002 or BSS138 | 2 | SOT-23 on a breakout, or through-hole equivalent (2N7000) |
| 11 | Resistors: 100 k ×5, 33 k, 15 k, 1 k ×3, 100 k gate pull-downs | ~12 | 1 % |
| 12 | 100 nF ceramic | 3 | ADC filter caps |
| 13 | Schottky diode (SS34 or 1N5819) | 1 | Pico VSYS feed |
| 14 | LEDs ×3, tactile button, perfboard | 1 set | |
| 15 | 18 AWG silicone wire, ferrules/terminal blocks | | Power wiring |
| 16 | Micro-USB to USB-A cable | 1 | Pico to Pi 5 USB-A port |
| 17 | Bench supply or electronic load, multimeter | | Needed for bring-up (dummy load before the Pi) |

Total: about 17 line items, nothing smaller than SOT-23.

## 2. Optional: pre-assembled carrier PCB (what I can and can't give you)

The carrier holds only signal parts (Pico socket, 2 FETs, 3 dividers, LEDs, button, headers) and carries **no power current**, which makes it the safe thing to have assembled.

**What I can't deliver yet:** a fab-ready package (Gerbers, BOM, pick-and-place CPL). Those must come from a KiCad design that passes ERC/DRC, and this machine has no KiCad (`kicad-cli` not found). A hand-written Gerber/CPL that has never been opened in a viewer is the kind of thing that wastes a board run, so I haven't faked one.

**Spec for the order** (JLCPCB-style assembly, 2-layer, ~50 × 40 mm, 1.6 mm, 1 oz, HASL lead-free, one side assembled):

| Ref | Part | Package | Notes |
|---|---|---|---|
| U1 | Pico socket | 2× 1×20 2.54 mm female header | Hand-solder; not assembled |
| Q1, Q2 | 2N7002 | SOT-23 | |
| R1–R3 | 100 k 1 % | 0603 | V_BAT / V_BUS / V_OUT divider tops |
| R4 | 33 k 1 % | 0603 | V_BAT bottom |
| R5 | 15 k 1 % | 0603 | V_BUS bottom |
| R6 | 100 k 1 % | 0603 | V_OUT bottom |
| R7, R8 | 100 k | 0603 | Q1/Q2 gate pull-downs |
| R9–R11 | 1 k | 0603 | LED resistors |
| C1–C3 | 100 nF | 0603 | ADC pin filters |
| D1 | SS34 | SMA | Pico VSYS feed |
| D2–D4 | LED green / amber / red | 0603 | |
| SW1 | Tactile switch | 6 mm | |
| J1–J6 | 2.54 mm headers / 5 mm screw terminals | THT | Power-in, 5 V, EN, PWR_BTN, I2C, sense inputs. Hand-solder |

Missing for a complete design: footprints, net names, EN and PWR_BTN polarity per [`POC_MODULAR.md`](POC_MODULAR.md), I2C pull-ups, and a GP29/NTC decision (stock Pico has no free ADC3 pin).

**To get the real files:** install KiCad 8 (`brew install --cask kicad`), and I'll capture the carrier, run ERC/DRC, and export Gerbers, BOM and CPL. Alternatively, order the module build above and tidy it on perfboard first.

## 3. Check before ordering

1. Pololu D36V50F5: minimum input voltage, dropout at a 6–7 V pack, ENABLE logic and pull-up level.
2. CC/CV charger: can it hold 8.40 V accurately with no termination, and handle load plus charge current?
3. BMS: continuous current rating, balance-lead order, overcharge and overdischarge thresholds.
4. PD trigger: it outputs 12 V under load and is rated for the 3 A input fuse.
5. Pi 5: `usb_max_current_enable=1` and the power-button wake behaviour (see issues #7, #13).
6. Pico: whether ADC0–2 on GP26–28 are on the header you buy (they are on a stock Pico).
