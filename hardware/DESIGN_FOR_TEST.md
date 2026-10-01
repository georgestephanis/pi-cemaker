# Design-for-Test and Rework Requirements (assembled prototype)

Requirements for the first assembled board (Lite or Full, see [`DESIGN_OPTIONS.md`](DESIGN_OPTIONS.md)). Goal: a board that arrives assembled, can be brought up one rail at a time, and can be corrected with a soldering iron and a knife instead of a respin. Planning spec only; nothing is captured in KiCad yet.

## 1. Bring-up isolation (power-up in stages)
- Every rail boundary has a **0 Ω link or solder jumper** (cut to isolate, bridge to restore): input -> charger, charger -> BMS/pack, VSYS -> 5 V stage, 5 V -> output connector, 3V3 LDO -> MCU. Default state: closed.
- A cuttable trace (two-pad "solder bridge", normally closed) on every signal I might need to disconnect from the MCU: EN, PWR_BTN, I2C.
- **Battery connect through a jumper/screw header**, so the board can first run from a bench supply on VBAT with no cells.
- A 2-pin **current-measure header** in series with VSYS-in and with the pack (replace with a jumper in normal use).

## 2. Test points (labelled on silk, ≥1 mm pad or loop, GND points near every group)
Power: VBUS_IN, VSYS, VBAT (pack+), cell mid-point (B1), VOUT, 3V3, GND ×6 spread across the board.
Switching nodes: SW of the charger and of the 5 V stage (probe with a spring tip; keep stubs short).
Control: EN_5V, PWR_BTN_GATE, PG/INT/STAT from the charger, PD contract-good, RP2040 RUN and BOOTSEL.
Comms: I2C SDA/SCL, USB D+/D-, SWD (3 pads or Tag-Connect footprint, no header), UART TX/RX.
Analog: NTC node, any ADC input.

## 3. Rework and tuning features
- **Footprint-compatible options** where unsure: dual footprints for feedback resistors (output voltage), charge-current and input-limit set resistors, PD voltage strap, and NTC bias; stuff-option pads for the pull-ups/pull-downs.
- Output voltage and charge current settable by **swapping 0603 resistors** and by I2C, so first power-up doesn't depend on firmware.
- Inductor and output capacitor footprints sized for the next bigger part (Lite -> Full upgrade path).
- Optional unpopulated pads: second inductor (charger phase), extra bulk caps, TVS, series snubber (RC) at each SW node, output load resistor.
- Separate **3V3 LDO input link** so the MCU can run on USB alone for firmware flashing with no power stage energised.
- Reverse-polarity and fuse: a fuse footprint on the pack and on input; polyfuses are acceptable.
- Mark pin 1 and polarity on silk; label all links with their default (CLOSED/OPEN).

## 4. Safety for the first power-up
Keep cells out for the first board. Bring up from a current-limited bench supply, in the order: 3V3/MCU -> PD input -> charger with no pack -> 5 V stage into a dummy load -> pack. Use a fire-safe surface and never leave cells charging unattended.

## 5. Assembly files and order checklist
- Deliverables from KiCad (needs ERC/DRC clean): Gerbers, drill, BOM with manufacturer part numbers and LCSC/JLC part numbers, CPL (pick-and-place).
- Stock check every part in the assembler's library (extended parts cost setup fees; prefer basic parts where possible).
- Hand-solder after assembly: through-hole connectors, cell holders, large inductor if it's not machine-placeable.
- Order 5 boards, assemble 2, so bare boards remain for rework experiments.
- Verify polarity/orientation of every IC against the CPL preview before paying.

## 6. Blockers before capture
1. **Charger choice.** The MP2762A datasheet may need an MPS login. TI's **BQ25792** (1–4 cell NVDC, public datasheet, EVM design files) would be far safer to capture from memory-free. Decide: BQ25792 or MP2762A.
2. **Lite or Full** (or capture both with a swappable 5 V sheet).
3. KiCad 8 installed (`brew install --cask kicad`), and the datasheets/EVM schematics for the chosen ICs fetched into the repo workspace so values come from sources, not memory.
