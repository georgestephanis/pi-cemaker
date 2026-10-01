# Design Decision Log (Lite board)

Every best-guess call made while building the first integrated board. Each entry says what was chosen, why, what it rests on, and **how to reverse it**. Status: `OPEN` = unverified guess, `SRC` = taken from a datasheet in `hardware/datasheets/` (run `fetch.sh`), `USER` = decided by the maintainer.

Nothing here has been built or tested. **No file from this board should be sent to a fab until every `OPEN` item marked BLOCKER is resolved.**

| # | Decision | Basis | Status | To reverse |
|---|---|---|---|---|
| D1 | Build the **Lite** variant (3 A / 5.1 V, 2-layer) first; Full follows by swapping the 5 V sheet | USER asked for both; Lite is the lower-risk first spin | USER | Add a Full 5 V sheet later; keep nets `VSYS`, `V5_B`, `EN_B` as the interface |
| D2 | Charger = **TI BQ25792** (not MP2762A) | Public datasheet; single inductor; NVDC SYS ≥ 7 V on mains | SRC | Replace sheet and footprint; keep nets `VBUS`, `VSYS`, `PACK_P`, I2C, INT, STAT |
| D3 | PD sink = **HUSB238 (DFN-10)**, hardware-strapped: VSET 10 kΩ = 12 V, ISET 22.6 kΩ = 3 A; I2C also wired | HUSB238 datasheet Tables 5, 6 | SRC | Change the two strap resistors (R_VSET, R_ISET); footprints are 0603 |
| D4 | **MCU = Raspberry Pi Pico module** in 2×20 female headers, not a bare RP2040 | Removes flash, crystal, LDO, USB filtering and their layout risk; Pico supplies 3V3 | OPEN | Replace J_PICO with a bare RP2040 sheet later; GPIO net names are unchanged |
| D5 | **Pack + BMS off-board**: 2-pin screw terminal `J3` (PACK+/PACK−); BMS sits on the cell assembly | Fewest parts on the first spin; BMS parts were unverified (#10) | OPEN | Add an on-board BMS sheet between `J3` and `JP3` |
| D6 | **5 V stage = TI LMR51450** buck (5 A, 500 kHz, RT open), 4.7 µH, 2×33 µF, FB 102 kΩ/19.1 kΩ | LMR51450 datasheet Table 9-2, eq. 8 (gives 5.07 V) | SRC | Change R_FBT to move Vout; use the same footprint for LMR51440 (4 A) |
| D7 | Buck UVLO: EN divider 62 kΩ / 21.5 kΩ (rising ≈ 4.9 V, falling ≈ 3.9 V) | LMR51450 eq. 14/16 | SRC | Swap R_ENT |
| D8 | `EN_KILL` N-FET (Q1) pulls buck EN low; gate has 100 kΩ pull-down so the **default is rail ON** | Fail-on rule in AGENTS.md | OPEN | Remove Q1; EN then rides the divider alone |
| D9 | Input protection: **no reverse FET**, fuse footprint F1 (fit 3.15 A or 0 Ω), TVS D1 **SMAJ13A** (clamp ≈ 21 V) | Old SMAJ24A clamp (~39 V) exceeds the 30 V limit of HUSB238/BQ25792 | OPEN | Add a P-FET or the ACDRV pair later; change D1 |
| D10 | Cell temperature: fixed 10 kΩ resistor `R_NTC_FIX` populated by default; thermistor header `J6` for a real NTC | BQ25792 TS pin must see a valid divider or charging is suspended | OPEN | DNP `R_NTC_FIX` and connect an NTC to `J6` |
| D11 | TS divider RT1 5.24 kΩ (REGN→TS), RT2 30.31 kΩ (TS→GND) | BQ25792 §9.3.9.5 (uses 5.23 k / 30.1 k E96 values) | SRC | Swap resistors |
| D12 | PROG = 6.04 kΩ → 2S, 1.5 MHz; inductor 1 µH | BQ25792 Table 9-1, §10.2.2.1 | SRC | Change R_PROG to 8.2 kΩ and L1 to 2.2 µH for 750 kHz |
| D13 | **No reverse-cell, balance, or ship-FET circuitry** on board; SDRV gets 1 nF to GND as the datasheet allows | Datasheet pin 24 note | SRC | Add external ship FET later |
| D14 | Power links (JP1 VSYS→buck, JP2 buck→output, JP3 pack→charger) are **bridged solder jumpers** (cut to isolate) | Rework/bring-up requirement (`DESIGN_FOR_TEST.md`) | USER | Replace with 0 Ω or solid trace |
| D15 | Output: USB-C power-only receptacle `J2` with 10 kΩ Rp to V5 (advertises 3 A) plus screw terminal `J4`; Pi data via the Pico's own micro-USB | Pi 5 sees a 3 A non-PD source; needs `usb_max_current_enable=1` (#7) | OPEN | Change CC resistors to 22 kΩ (1.5 A) or 56 kΩ (default) |
| D16 | BQ25792 `D+`/`D-` left floating, `QON` left to its internal pull-up (test point only), `ACDRV1/2` to GND, `VAC1/2` to VBUS | Datasheet pin table | SRC | n/a |
| D17 | GPIO map: GP4/GP5 I2C0, GP6 INT, GP7 STAT, GP8 PG, GP14 PWR_BTN, GP15 EN_KILL, GP16/17/18 LEDs, GP19 button | Matches firmware where possible | OPEN | Edit `power_mgr.h` / net labels together |
| D18 | **BQ25792 footprint is hand-built from a rendered image** of TI drawing RQM0029A at ~110 dpi | The land pattern is irregular (variable pad sizes, 0.4 mm pitch) | OPEN, **BLOCKER** | Replace with TI's CAD model (Ultra Librarian/SnapEDA) and rerun DRC |
| D19 | Inductor/cap/FET/diode part numbers are **generic placeholders** (footprints chosen, MPNs marked `verify`) | No stock or datasheet check done | OPEN, **BLOCKER** | Fill MPNs, check DC-bias derating, saturation current ≥ 8 A for L2 and ≥ 7 A for L1 |
| D20 | LMR51450 uses stock `WSON-12-1EP_3x3mm_P0.5mm_EP1.5x2.5mm`; AGND and PGND both go to GND at the pad | Not compared against TI drawing for DRR | OPEN, **BLOCKER** | Compare with the datasheet package page; adjust |
| D21 | KiCad's stock Pico symbol marks GND/AGND pins "power output"; the generator rewrites them to passive so ERC does not flag GND-to-GND | ERC pin_to_pin errors | OPEN | Remove the rewrite in `gen_sch.py` and set ERC severity |
| D22 | Board 112 × 78 mm, 2-layer, Pico on the right edge, USB-C connectors on the left edge, test points in rows | Placement guess | OPEN | Edit `P` table in `gen_pcb.py` |
| D23 | Autorouted with Freerouting 2.4.1; result accepted into the PCB as a draft | Router left 23-38 connections unrouted | OPEN, **BLOCKER** | `git checkout` the previous `.kicad_pcb`, or delete tracks and re-run `gen_pcb.py` |
| D24 | Power traces 1.0 mm (netclass Power), other 0.25 mm; 1 oz copper assumed | 1 mm 1 oz ≈ 2–3 A at ~10 °C rise; **not enough margin for 4 A** | OPEN, **BLOCKER** | Use pours / 2 oz, or widen by hand |

## Things deliberately not done
No autorouting result is trusted until DRC passes and a human reviews the thermal/current paths (4 A on 1 oz copper). No fab zip is produced while a BLOCKER is open.
