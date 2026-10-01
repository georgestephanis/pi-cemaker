# ⚠️ Critical Design Considerations & Edge Cases

This document details the real-world operational edge cases, hardware quirks, and protective design patterns that must be accounted for in **Pi-cemaker**.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This document details evolving design considerations and is **not complete yet**. It is **certainly NOT for sale**.

---

## 1. The "Zombie Halt" State & Cold-Boot Power Cycling

### The Problem
When the UPS signals a low battery, the Raspberry Pi OS executes an orderly shutdown:
1. Filesystems sync and unmount cleanly.
2. The Linux kernel halts the processor.
3. **The Trap:** In the halt state, the Raspberry Pi 5 PMIC remains energized, continuously drawing **~1.5W to 2.0W** from the 5V rail.
   - If the UPS continues supplying power, the battery will be drained completely to the BMS cutoff point.
   - More critically: **When mains wall power returns, the Raspberry Pi 5 will NOT turn back on automatically** if its 5V rail never dropped. It remains frozen in the halted state.

### The Solution: Post-Shutdown Power Cut Sequence
1. **Signal:** Pi-cemaker asserts `ShutdownImminent = 1` over USB HID.
2. **Timer Window:** Pi-cemaker starts a non-blocking **45-second countdown** (ample time for Linux to halt cleanly).
3. **Power Cut:** After 45 seconds, the RP2040 drives `PIN_5V_EN` (`GPIO15`) LOW, cutting 5.1V power to the Pi 5 completely.
4. **Auto-Reboot on Mains Return:** When wall power returns:
   - The RP2040 detects `V_BUS_IN > 7.5V`.
   - The RP2040 drives `PIN_5V_EN` (`GPIO15`) HIGH.
   - 5.1V power is restored to the Pi 5 PMIC, prompting a clean cold-boot automatically.

### Hardware Wake Option: RPi 5 `PWR_BTN` / `GLOBAL_EN` Header
The Raspberry Pi 5 includes a dedicated 2-pin JST-SH power button header (adjacent to the physical power button).
- Pi-cemaker provides a 2-pin JST-SH header (`J4`) driven by an RP2040 open-drain N-FET (`Q6`: 2N7002 driven by `GPIO14`).
- This allows Pi-cemaker to pulse the Pi 5 power button to wake it up or cleanly request a shutdown via hardware if USB is disconnected.

---

## 2. Raspberry Pi 5 USB-C Port Configuration (`config.txt`)

### The DWC2 Controller
The USB-C power port on the Raspberry Pi 5 is wired to an internal Synopsys DWC2 USB controller. By default in standard Raspberry Pi OS, this port does not run in USB Host mode.

### Configuration Requirement
> [!WARNING]
> Unverified and under review (issue #6). `otg_mode=1` is older Pi 4-era guidance. Raspberry Pi's current OTG whitepaper uses the dwc2 overlay (`dtoverlay=dwc2,dr_mode=...`), and the Pi 5 has no OTG_ID line, so the role must be forced. Here the Pi must be the **host** (the UPS is the USB device). Confirm the exact host-mode syntax on a real Pi 5 before documenting it for users. This also means the single-cable path is **not** zero-config; the auxiliary cable (J3) into a USB-A host port is.

### Hardware Fallback / Alternative Host Link
Not all users want to edit `config.txt` (or they may be running specialized appliances like LibreELEC or bare-metal hypervisors).
- Pi-cemaker should include an auxiliary 4-pin JST-SH (or standard USB-A/C auxiliary header) wired in parallel to the RP2040 USB lines.
- This allows connecting a secondary standard USB cable into one of the Pi 5's dedicated USB 3.0 / USB 2.0 host ports if desired.

---

## 3. Reverse Polarity Protection for Loose 18650 Cells

### The Hazard
Unlike enclosed battery packs (e.g. Sony NP-F), standard cylindrical 18650 battery clips allow users to insert one or both cells backwards.
- In a 2S series connection, inserting one cell backwards creates a destructive short across the other cell.
- Standard silicon diodes drop too much voltage and dissipate excessive heat under 5A discharge loads.

### Recommended Protections
1. **Active MOSFET Protection:** Place low-$R_{DS(on)}$ P-channel MOSFETs (or N-channel low-side FETs) configured as ideal reverse-polarity blocking switches for each cell bay.
2. **Mechanical Keying:** Select battery holders that feature raised mechanical plastic shoulders on the positive terminal (preventing the flat negative base of an 18650 from making contact if inserted backwards).

---

## 4. 2S Cell Mismatch & Balancing

### The Problem
When users insert two loose 18650 cells, they may have different capacities, cycle ages, or starting charge states.
- During charging, the lower-capacity cell will reach 4.25V first.
- The hardware protection IC will trip over-voltage and stop charging, leaving the other cell at 3.90V (preventing full pack capacity utilization).

### The Solution
- Include a dedicated 2S passive balancing circuit (e.g. **HY2212** balance IC with 50–100mA bleed resistors).
- When a cell reaches 4.20V, the bleed resistor shunts charge current around that cell, allowing the lagging cell to catch up.
- In documentation and silkscreen, instruct users to always use **matched pairs** of identical 18650 cells.

---

## 5. Quiescent Current & RP2040 Dormant Sleep

### The Danger of Deep Parasitic Drain
If mains power is lost while a node is unattended for days, the battery will discharge to its 6.0V cutoff:
- If the RP2040 and sensor circuitry remain active, they will consume ~20–30mA.
- This parasitic drain will pull the cells below 2.5V/cell into dangerous deep discharge over 1–2 weeks, permanently damaging the Li-ion cells.

### The Solution: Dormant Sleep State
Once output power is cut due to battery depletion:
1. RP2040 turns off the DC-DC regulator, status LEDs, and sensor rails.
2. RP2040 transitions its clocks and oscillators into **Dormant Mode** (power draw `< 100 µA`).
3. The RP2040 only wakes up when an external edge interrupt is triggered by `V_BUS_IN` (mains power reconnected) or by the physical user button.
