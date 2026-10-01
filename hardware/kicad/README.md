# KiCad project: Pi-cemaker Lite (DRAFT)

Generated, not hand-drawn. Edit `gen/design.py`, then run `gen/build.sh`. Reasoning for every guess is in [`../DECISIONS.md`](../DECISIONS.md).

| File | What |
|---|---|
| `gen/design.py` | Single source of truth: parts, values, footprints, pin-to-net map |
| `gen/gen_sch.py` | Writes `pi-cemaker.kicad_sch` + `pi-cemaker.kicad_sym` + `gen/netlist.json` |
| `gen/gen_fp.py` | Writes the hand-built BQ25792 footprint (`pi-cemaker.pretty/`) |
| `gen/gen_pcb.py` | Writes `pi-cemaker.kicad_pcb` (placement, outline, GND pours) |
| `gen/gen_bom.py` | Writes `../BOM_DRAFT.csv` |

## State (2026-10-01)
- **Schematic:** 110 parts, 51 nets, **KiCad ERC: 0 violations**. Layout is one flat sheet, grouped by function, with labels instead of wires (readable, not pretty). ERC clean only means the netlist is consistent; it does not mean the circuit is right.
- **PCB:** 112 × 78 mm, 2 layers, all parts placed, autorouted with Freerouting then GND-filled. **38 connections remain unrouted**, plus silk/courtyard/clearance violations. Power nets are 0.75–1.0 mm traces (borderline for 3–4 A on 1 oz). Not reviewed by a human.
- **Fab files: deliberately not produced.** `kicad-cli pcb export gerbers` works, but the BQ25792 footprint is a guess from an image, passives are unselected, and the board is not fully routed (DECISIONS D18–D20).

## Next steps, in order
1. Replace `VQFN-HR-29_RQM0029A_4x4mm` with TI's official footprint; compare U3's footprint with TI's DRR drawing.
2. Pick real MPNs; check DC-bias derating and inductor Isat; stock check at the assembler.
3. Re-place by hand (switching loops tight, inductors next to SW pins), route power by hand, then autoroute the rest; get DRC to zero.
4. Human review of the schematic against the two datasheets; then bring-up per `../DESIGN_FOR_TEST.md`.
5. Only then export Gerbers/CPL.
