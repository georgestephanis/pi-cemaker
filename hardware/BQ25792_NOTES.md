# BQ25792 Design Notes (from the datasheet)

Source: TI BQ25792 SLUSDG1C (Aug 2022), fetched with `datasheets/fetch.sh`. Section numbers refer to that revision. This resolves several open questions in #8, #9 and #11. **Still unverified:** the 5 V stage ICs and everything not listed here.

## Facts that change the design
| Item | Datasheet fact | Effect |
|---|---|---|
| Package | 29-pin 4×4 mm VQFN (RQM); pin 29 PMID, 28 SW1, 27 GND, 26 SW2, 25 SYS | Footprint must be custom or from TI; not in KiCad stock libraries |
| Topology | 4-switch buck-boost, **one inductor** between SW1 and SW2 | Answers #8: no second phase. 1 µH at 1.5 MHz, or 2.2 µH at 750 kHz (§10.2.2.1) |
| I2C | 7-bit address **0x6B** | Fixed; no conflict with INA226 default 0x40 |
| Cell count / defaults | Set by the PROG resistor to GND at power-up: **2S, 1.5 MHz = 6.04 kΩ**; 2S, 750 kHz = 8.2 kΩ (Table 9-1) | 2S defaults: ICHG 2 A, VSYSMIN 7 V, VREG 8.4 V |
| System rail | NVDC: SYS follows the battery, never below VSYSMIN (7 V default for 2S) while input is present | **Good for the Lite buck**: SYS is ≥7 V on mains. On battery only, SYS = VBAT (can sag toward 6 V) |
| Defaults | After a watchdog timeout or reset ICHG, VSYSMIN, VREG return to 2 A / 7 V / 8.4 V | Firmware must refresh the watchdog or disable it; 2 A charge is above the 1–1.5 A I want for 3000 mAh cells, so set ICHG at boot |
| Input | VBUS abs max 30 V, VAC1/VAC2 30 V; SYS 23 V (not switching) | A SMAJ24A TVS (clamp ~39 V) does **not** protect this part; choose a lower-clamp TVS or none (#9) |
| Input FETs | ACDRV1/2 drive optional external back-to-back N-FETs; tie ACDRV to GND if not fitted; VAC1/VAC2 to VBUS if no ACFET | The charger has no integrated reverse blocking FET in this mode, so reverse-input protection needs the external N-FET pair or a single P-FET |
| Ship FET | External N-FET on SDRV, or a 1 nF cap from SDRV to GND when unused | Needed for a real ship mode |
| TS | 10 kΩ NTC (103AT) with divider from REGN | Frees the RP2040 NTC input and removes the GP29 problem |
| Pins to 6 V max | QON, D+/D-, CE, STAT, SCL, SDA, INT, ILIM_HIZ, PROG, TS, REGN | Logic pins must be pulled to REGN/3V3, never to SYS |
| Caps (datasheet-recommended) | VBUS 2×10 µF + 0.1 µF; PMID 3×10 µF + 0.1 µF; SYS 5×10 µF + 0.1 µF; BAT 2×10 µF; REGN 4.7 µF; BTST1/2 47 nF each | Start the BOM from these, then derate for DC bias |
| BATP | 100 Ω in series to pack+ | |
| ILIM_HIZ | 1 V + 0.8 Ω × I_in; tie to REGN for max | Use a divider footprint for input-limit trimming |
| STAT, INT | Open drain, 10 kΩ pull-up | INT → RP2040 GPIO |

## Consequences for earlier issues
- **#8:** single inductor confirmed; V_SYS is 7 V-min NVDC with the BQ25792 (the MP2762A question is moot if we use this part).
- **#11:** the charger ADC covers VBUS, VBAT, VSYS, IBUS, IBAT and TS over I2C; drop the RP2040 dividers.
- **#9:** drop the "inrush" P-FET/zener stage as designed; decide between the charger's ACDRV pair and a single input FET.
- **Lite 5 V stage:** a buck from SYS (7–9 V on mains, 6–8.4 V on battery) is workable if the shutdown threshold stays well above 6 V.

## Not done
No schematic, footprint, layout or fab file has been produced. A fab-ready board needs custom symbols and footprints for the BQ25792 and the 5 V stage parts, a routed layout that passes DRC, and an autorouter or manual routing. KiCad 10.0.6 and its CLI are installed and work; Java is present, but no autorouter is.
