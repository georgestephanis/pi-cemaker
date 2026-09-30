# 📏 Pi-cemaker PCB Layout & Thermal Design Guidelines

This document details the PCB stackup, copper weight, trace sizing rules, thermal dissipation techniques, and component placement constraints for **Pi-cemaker** to safely deliver **5.1V @ 5.0A (25.5W continuous)** without thermal throttling or switching instability.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This document specifies evolving layout rules and is **not complete yet**. It is **certainly NOT for sale**.

---

## 1. PCB Stackup (4-Layer Recommended)

A 4-layer stackup with standard FR-4 substrate ($1.6\,\text{mm}$ overall thickness) is mandatory to satisfy the high current density and low EMI requirements:

```text
┌────────────────────────────────────────────────────────┐ Layer 1: Top Signal & Power (2 oz Cu)
│  - TPS55289 Buck-Boost, MP2762A Charger, Inductors     │
├────────────────────────────────────────────────────────┤ Dielectric: 0.2mm Prepreg (7628)
┌────────────────────────────────────────────────────────┐ Layer 2: Solid Ground Plane (1 oz Cu)
│  - Continuous, unbroken reference & thermal heat spreader│
├────────────────────────────────────────────────────────┤ Core: 1.0mm FR-4 Core
┌────────────────────────────────────────────────────────┐ Layer 3: Power Distribution Plane (1 oz Cu)
│  - V_SYS, V_OUT (5.1V), V_BUS_IN (9-20V), 3.3V Logic   │
├────────────────────────────────────────────────────────┤ Dielectric: 0.2mm Prepreg (7628)
┌────────────────────────────────────────────────────────┐ Layer 4: Bottom Signal & Thermal (2 oz Cu)
│  - 18650 Battery Clips, RP2040 MCU, Thermal Vias      │
└────────────────────────────────────────────────────────┘
```

- **Outer Layers (1 & 4):** $2\,\text{oz}$ copper ($70\,\mu\text{m}$) for low $I^2 R$ conduction losses on the 5A power loops and battery discharge rails.
- **Inner Layer 2 (GND):** Dedicated 100% solid copper ground pour beneath all power switching circuitry. No routing traces allowed on this layer.
- **Inner Layer 3 (Power):** Wide polygon planes for `V_SYS` and `V_OUT`.

---

## 2. Trace Sizing & Current Density (5.0A Continuous Rail)

Under standard IPC-2152 standards for a 20°C temperature rise with $2\,\text{oz}$ copper:
- **5.0A Output Rail (`V_OUT`):**
  - Minimum trace width on external layer: $\ge 2.5\,\text{mm}$ ($100\,\text{mils}$).
  - Recommended: **Polygon copper pours** spanning $\ge 4.0\,\text{mm}$ connecting inductor $L_2$ pad, output capacitors, and USB-C output receptacle $J_2$.
- **Battery Discharge Path (`V_BAT` / `V_SYS` at 6.0V cutoff):**
  - At $V_{BAT} = 6.0\,\text{V}$ supplying $25.5\,\text{W}$ at 92% efficiency, battery draw is:
    $$I_{BAT} = \frac{25.5\,\text{W}}{6.0\,\text{V} \times 0.92} \approx 4.62\,\text{A}$$
  - The traces connecting the 18650 positive/negative clips to the protection FETs and $V_{SYS}$ must also use $\ge 2.5\,\text{mm}$ copper pours.
- **Vias in Power Paths:**
  - Standard $0.3\,\text{mm}$ drill / $0.6\,\text{mm}$ annular ring via carries $\approx 1.2\,\text{A}$ safely.
  - Any layer transitions for `V_OUT`, `V_SYS`, or high-current GND must use an array of **at least 5 vias** in parallel ($5 \times 1.2\,\text{A} = 6.0\,\text{A}$ capacity).

---

## 3. High di/dt Switching Loops (EMI & Stability)

The synchronous buck-boost converter (TPS55289) and switching charger (MP2762A) switch at hundreds of kilohertz with nanosecond rise times.
1. **Loop Area Minimization:**
   - The loop formed by the input ceramic capacitors ($C_{IN}$), the internal high-side FETs of TPS55289, inductor $L_2$, the internal low-side FETs, and the output ceramic capacitors ($C_{OUT}$) must have the **smallest geometric area possible**.
   - Place $47\,\mu\text{F}$ 1206 capacitors directly adjacent to the IC pins before routing to the bulk polymer electrolytic capacitor.
2. **Switching Nodes ($SW_1, SW_2$):**
   - Keep the copper area of the switching nodes just large enough to carry 6A current without creating an expansive RF antenna.
   - Do not run sensitive analog lines (such as ADC divider taps or crystal oscillator tracks) anywhere near or beneath $SW_1$ and $SW_2$.

---

## 4. Thermal Dissipation Strategy

At full $25.5\,\text{W}$ output ($5.1\,\text{V} \times 5.0\,\text{A}$), with an expected $94\%$ converter efficiency, thermal dissipation is:
$$P_{diss} = 25.5\,\text{W} \times (1 - 0.94) \approx 1.53\,\text{W}$$

- **Thermal Via Array:**
  - Place a $4 \times 4$ grid of $0.3\,\text{mm}$ diameter thermal vias directly inside the exposed thermal ground pad of the `TPS55289` (QFN-21).
  - Place a $4 \times 3$ grid inside the thermal pad of the `MP2762A` (QFN-30).
  - Connect these vias directly to Layer 2 GND and Layer 4 bottom ground copper pour to dissipate heat across the entire bottom surface of the PCB.
- **Battery Heat Isolation:**
  - Place the power switching stages (TPS55289 and MP2762A) toward the front edge of the board, mechanically separated from the 18650 battery cells to prevent heating the Li-ion cells above 45°C.

---

## 5. Kelvin Sensing & Sensitive Signals

1. **Current Sense Shunt ($R_{SNS}$):**
   - Use a 4-terminal Kelvin connection layout. The sense lines must tap the inside pads of the shunt resistor and route as a differential pair to the charger IC.
2. **Voltage Feedback ($R_{FB1}, R_{FB2}$):**
   - The feedback trace must tap the $5.1\,\text{V}$ rail directly at the terminals of the output capacitor array (not at the inductor output pad) to prevent switching ripple from degrading regulation.
3. **Crystal Oscillator ($Y_1$):**
   - Place the $12\,\text{MHz}$ crystal within $5\,\text{mm}$ of RP2040 pins `XIN` / `XOUT`.
   - Surround the crystal traces with a continuous ground guard ring on Layer 1.

---

## 6. Mechanical Dimensions & Raspberry Pi 5 Mounting

- **Board Dimensions:** $85.0\,\text{mm} \times 56.0\,\text{mm}$ (matches the standard Raspberry Pi footprint).
- **Mounting Holes:**
  - 4× $M2.5$ mounting holes ($2.75\,\text{mm}$ hole diameter, $6.0\,\text{mm}$ copper keepout ring).
  - Hole spacing: $58.0\,\text{mm} \times 49.0\,\text{mm}$ from center to center.
- **Connector Placement:**
  - USB-C Input ($J_1$): Left edge (facing backwards).
  - USB-C Output ($J_2$): Right edge (directly aligning with Raspberry Pi 5 USB-C power port).
  - Buttons ($SW_1$ BOOTSEL, $SW_2$ USER): Board edge for fingertip access.
  - Status LEDs ($D_3, D_4, D_5$): Visible on the top edge.
