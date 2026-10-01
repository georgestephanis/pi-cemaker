# Two Board Designs: "Lite" (3–4 A) and "Full" (5 A)

Both share the same architecture and firmware. They differ only in the 5 V stage and how hard the layout is. This is a **paper design**: every part number and rating below is from memory or web search and marked **(verify)** where it matters. Nothing here has been captured in KiCad. Related: #12 (2-layer), #19 (decisions), #7 (Pi USB-C current).

## Shared architecture (simplifications from the review)

```text
USB-C in ─ HUSB238A (PD sink, strap 12 V, I2C) ─ MP2762A (NVDC 2S charger, I2C) ─┬─ 2S BMS (HY2120 + balancers) ─ 2×18650
                                                                                  └─ VSYS (tracks pack, ~6–9 V (verify)) ─ 5 V stage ─ USB-C out → Pi 5
RP2040: I2C0 → MP2762A + HUSB238A (all telemetry, no resistor dividers)
        GPIO VBUS_PRESENT (digital, also dormant wake) · GPIO EN (fail-on, via small N-FET) · GPIO PWR_BTN (N-FET → Pi J2) · 3 LEDs · button
        USB D+/D- → J3 aux host cable (primary data link, zero-config)
```

Dropped versus the current docs: input P-FET/zener stage (#9), four precision ADC dividers (#11), dual-core (#14), per-cell reverse-polarity FETs (#10; use keyed holders). Policy: shutdown at ~3.3 V/cell with hysteresis (#4); zombie halt handled by `POWER_OFF_ON_HALT=1` plus a PWR_BTN pulse (#13).

Common caveats: confirm MP2762A inductor count/phase and VSYS range from its datasheet (#8); EN must never see more than 3.3 V at the GPIO (#15); only USB PD can signal 5 A, so the Pi needs `usb_max_current_enable=1` or a PD source controller (#7).

---

## Design A — Lite: 3–4 A (~15–20 W), 2-layer

**5 V stage: a plain synchronous buck, 5 A-class part.**
- Candidates **(verify all)**: TI LMR51450 / TPS54540-class, or a 4–5 A integrated-FET buck with ≥21 V input rating. One inductor (~3.3 µH, ≥7 A sat), 22–47 µF output ceramics, feedback divider to 5.1 V.
- Works because VSYS stays above 5.1 V + dropout. That is **only true if the shutdown threshold is raised** so the loaded pack never nears ~6 V. Cost: the last ~5–8 % of pack energy is unused.

| Item | Choice |
|---|---|
| Output | 5.1 V, 3 A continuous / 4 A peak. CC Rp 10 k advertises 3 A; document `usb_max_current_enable=1` |
| Layers | **2**, 1 oz both sides, 1.6 mm |
| Layout | All ICs top. Bottom is a near-solid GND pour with stitching vias; no signals under the SW node or inductor. VSYS/VOUT as wide top pours |
| Charge current | ≤1.5 A default (~0.5 C), configurable |
| Size | ~85 × 56 mm feasible; holders on bottom if space is tight |
| Estimated added BOM vs Full | lower: one inductor, cheaper IC, fewer bulk caps |

Risks: buck dropout near end of discharge; 1 oz copper at 4 A is fine on short wide pours (keep runs short, ≥3 mm wide). Thermal: ~1 W in the buck, handled by pad vias to the bottom pour.

## Design B — Full: 5 A (25 W), 4-layer

**5 V stage: 4-switch buck-boost (TPS55289-class) (verify output-current rating, ≥6 A switch limit, and I2C).**
- Buck-boost lets the pack discharge down to the BMS cutoff without dropout, and a 5 A load stays regulated.
- I2C-programmable output allows cable-drop compensation (set ~5.2 V at the converter).

| Item | Choice |
|---|---|
| Output | 5.1 V, 5 A continuous. Add a PD source controller for a real 5 V/5 A PDO (**verify** part, e.g. TI TPS25730 family) or use the config override |
| Layers | **4**: F signal+power / GND / power plane / B signal+GND. 2 oz outer if the layout is dense |
| Layout | Tight hot loop around the converter; feedback away from the inductor; inductor ~1.5 µH, saturation ≥12 A (verify) |
| Charge current | up to 2.5 A (configurable; default lower) |
| Size | ~85 × 56 mm likely tight; consider 90 × 60 |
| Thermal | ~2 W in the converter; copper pours + vias; keep ≥15 mm from cells |

Risks: DC-bias derating of ceramics (size output caps from measurement, #16); cable drop at 5 A (0.5–0.75 V); 2-layer at 5 A is *possible with 2 oz* but not recommended for a first spin.

---

## Which to build first

| | Lite | Full |
|---|---|---|
| Hand-solder / low-risk layout | better | worse |
| Pi 5 with NVMe/USB drives | fine ≤ ~15 W | needed for peak loads |
| Usable battery energy | ~92 % | ~100 % |
| Layers / cost | 2 / lower | 4 / higher |

**Recommendation:** build Lite after the modular PoC (`POC_MODULAR.md`); Full is the same schematic with the 5 V stage swapped and the layer count raised, so most work carries over.

## Next steps
1. Datasheet-verify every **(verify)** item (MP2762A VSYS/inductors, buck and buck-boost ratings, HUSB238A pin strapping).
2. Capture one KiCad schematic with the 5 V stage as a swappable sheet (#17).
3. Lay out Lite on 2 layers first and measure ripple/thermals.
