# 🛠️ Pi-cemaker Hardware Design Specification

This document details the hardware design, component trade-offs, schematic architecture, and mechanical layout considerations for **Pi-cemaker**.

---

## 1. Design Paths & Component Selection

When creating a simplified 2S 18650 UPS, there are two primary circuit topologies:

### Path A: All-In-One Power Bank IC (e.g., Injoinic IP2368 / IP5389)
- **Concept:** A single high-power IC that integrates synchronous bidirectional buck-boost, USB-PD 3.0 sink & source negotiation, and 2S–4S multi-cell battery charging.
- **Pros:**
  - Dramatically fewer discrete components (1 main IC + 4 MOSFETs + 1 inductor).
  - Lowest PCB footprint and BOM component count.
  - Native I2C register interface to read battery status, voltages, currents, and faults.
- **Cons:**
  - Chinese datasheets / errata can be less comprehensive.
  - Fixed internal state machines can limit custom low-level battery charge curve tweaking.

### Path B: Streamlined Discrete Power Path (Recommended)
- **Concept:** Follow the proven power architecture of the Web3 Pi UPS, but strip away role switching, the cellular card, and the extra microcontrollers:
  1. **USB-PD Sink:** **HUSB238** (negotiates 9V–20V input from charger via I2C or pin strapping).
  2. **2S Charger:** **MP2762A** (MPS) — highly efficient 2S Li-ion switching charger with integrated ADC and power path.
  3. **5V Output Regulator:** **TPS55289** (TI) or **Southchip SC8721** — synchronous buck-boost providing rock-solid 5.10V @ 5A.
  4. **Microcontroller:** **RP2040** (Raspberry Pi) — handles TinyUSB HID UPS + CDC serial, reads charger telemetry over I2C.
- **Pros:**
  - Robust, fully verified silicon with high documentation availability.
  - Complete control over charging parameters, cutoff voltages, and thermal safety.
  - Direct heritage and component familiarity from the Web3 Pi UPS project.

---

## 2. Battery Subsystem (2× 18650 in 2S1P)

### Battery Specifications
- **Chemistry:** Lithium-ion (NMC / INR).
- **Nominal Voltage:** 7.4V (3.7V per cell).
- **Full Charge Voltage:** 8.4V (4.20V per cell).
- **Discharge Cutoff:** 6.0V (3.0V per cell) to ensure cell longevity.
- **Form Factor:** Standard 18650 cells (65mm length × 18mm diameter).
- **Holders:** Through-hole or heavy-duty surface mount battery clips:
  - Dual 18650 battery holder: **Keystone 1048** (SMD) or **Keystone 1049** (THM).
  - Or individual battery leaf spring clips: **Keystone 1042** / **1043**.

### Protection & Cell Balancing
- **Hardware Protection Circuit:**
  - Dedicated 2S battery protection IC: **HY2120-LB** (or Seiko S-8252).
  - Dual N-channel power MOSFETs (e.g., **AO8822** or **AP4310**) in the low-side battery return path.
  - Trips autonomously on overcharge (>4.28V/cell), overdischarge (<2.90V/cell), and short circuit (>10A).
- **Passive Cell Balancing:** Resistor bleeding across each cell triggered near 4.20V to prevent cell mismatch drift over cycles.

---

## 3. Power Output & Raspberry Pi 5 USB-PD Profile

### Output Power Delivery
- **Voltage:** 5.10V (slight boost above nominal 5.0V to compensate for USB-C cable IR drop under 5A load).
- **Current:** 5.0A continuous (25.5W), capable of 6.0A transient pulses.
- **Raspberry Pi 5 Compatibility:**
  - Raspberry Pi 5 expects a 5V / 5A power contract over USB-C.
  - The CC lines (CC1/CC2) can be driven by a small dedicated USB-PD source controller (such as the **AP43771**, **CH224K**, or **CH32X033**), or standard resistor pull-ups if fixed 5V is supplied.

---

## 4. Current & Voltage Telemetry

To report accurate battery levels to Linux via USB HID:
- **Voltage Measurement:** Precision 0.1% resistor dividers to RP2040 ADC inputs:
  - `V_BAT` (scaled to 0–3.0V range for ADC0).
  - `V_BUS_IN` (scaled to 0–3.0V range for ADC1).
  - `V_OUT` (scaled to 0–3.0V range for ADC2).
- **Current Measurement:** High-side shunt resistor (e.g. 10mΩ) with an I2C current monitor such as the **TI INA219** or **INA226** (or utilizing the MP2762A internal current ADC).
- **Temperature Sensing:** 10k NTC thermistor placed in direct mechanical contact with the 18650 battery holders.

---

## 5. Physical Layout & Form Factor Options

### Form Factor Concept: "Inline Battery Sled"
- **Dimensions:** Approx. 85mm × 55mm (roughly the footprint of a Raspberry Pi 5 or a 2× 18650 holder).
- **Connectors:**
  - **USB-C Input:** Left/rear side for wall adapter.
  - **USB-C Output:** Right/front side facing the Raspberry Pi 5.
  - **JST-SH / Stemma QT:** Expansion I2C connector for optional external OLED display.
  - **Bootsel Button:** For easy drag-and-drop firmware flashing over USB.
- **PCB Stackup:** 4-layer PCB (Signal - GND - Power - Signal) recommended for optimal thermal dissipation and low-resistance 5A power routing.
