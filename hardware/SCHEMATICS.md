# 📐 Pi-cemaker Circuit Schematics & Electrical Design

This document details the circuit schematic design, mathematical component calculations, netlist relationships, and design rationales for each electrical subsystem of **Pi-cemaker**.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This hardware schematic design is a work-in-progress and is **not complete yet**. It is **certainly NOT for sale** or production deployment.

---

## 📑 Subsystem Breakdown

1. [**Subsystem 1: USB-PD Sink Input & Inrush Protection**](#subsystem-1-usb-pd-sink-input--inrush-protection)
2. [**Subsystem 2: 2S Li-ion Battery Protection & Passive Balancing**](#subsystem-2-2s-li-ion-battery-protection--passive-balancing)
3. [**Subsystem 3: 2S Switching Charger & Seamless Power Path (MP2762A)**](#subsystem-3-2s-switching-charger--seamless-power-path-mp2762a)
4. [**Subsystem 4: High-Efficiency Synchronous Buck-Boost 5.1V @ 5A (TPS55289)**](#subsystem-4-high-efficiency-synchronous-buck-boost-51v--5a-tps55289)
5. [**Subsystem 5: Raspberry Pi 5 Host Interface & USB-C Power/Data**](#subsystem-5-raspberry-pi-5-host-interface--usb-c-powerdata)
6. [**Subsystem 6: RP2040 Microcontroller & 3.3V Logic Rail**](#subsystem-6-rp2040-microcontroller--33v-logic-rail)

---

## Subsystem 1: USB-PD Sink Input & Inrush Protection

### Purpose
Accepts 9V, 12V, 15V, or 20V from standard USB-C PD 3.0 power adapters. Suppresses hot-plug inductive voltage spikes, protects against reverse connection, and negotiates the highest available voltage profile.

```text
[USB-C Receptacle: J1]
  VBUS (Pins A4/A9/B4/B9) ─────────┬────────▶ [D1: SMAJ24A TVS] ───▶ [Q1: DMP3010LK3 P-FET] ───▶ V_BUS_IN
                                   │                                 (Reverse Polarity)
  GND  (Pins A1/A12/B1/B12) ───────┴───────────────────────────────────────────────────────────▶ GND
  CC1  (Pin A5) ───────────────────▶ [HUSB238: CC1 Pin]
  CC2  (Pin B5) ───────────────────▶ [HUSB238: CC2 Pin]
```

### Component Details
- **USB-C Input Connector (J1):** 16-pin / 24-pin USB-C Receptacle (e.g. TYPE-C-31-M-12), rated for 20V @ 5A.
- **TVS Diode (D1):** `SMAJ24A` (24V standoff, 38.9V clamping at 10.3A peak surge). Protects against inductive flyback during adapter hot-plugging.
- **Reverse Polarity / Inrush MOSFET (Q1):** `DMP3010LK3` (P-channel, -30V, $R_{DS(on)} < 10\,\text{m}\Omega$, TO-252 / PowerDI5060).
  - Gate pulled to GND via 100kΩ resistor ($R_1$).
  - 15V Zener diode ($D_2$: BZX84C15) between Gate and Source to protect gate oxide when input is 20V.
- **PD Sink Controller (U1):** `HUSB238` (QFN-16 / DFN-10):
  - Configured via pin strapping or I2C.
  - Pin `VSEL` pulled to GND via resistor network to request 12V or 15V (optimal for 2S charging).
  - Pin `EN_MOS` directly enables Q1 once a valid contract is negotiated.
- **Bulk Input Capacitors:**
  - $2 \times 10\,\mu\text{F}$ 50V X7R ceramic (1206) in parallel with $1 \times 100\,\text{nF}$ 50V (0402).

---

## Subsystem 2: 2S Li-ion Battery Protection & Passive Balancing

### Purpose
Protects two series 18650 cells against overcharge, overdischarge, short circuits, reverse cell insertion, and cell mismatch.

```text
                [CELL 1 (+) ] ──────[Q_REV1: P-FET]───────────▶ BATT+ (6.0V - 8.4V)
                      │                                            │
               [HY2212 Bal 1] (Bleed 68Ω)                          │
                      │                                            │
             [CELL 1 (-) / CELL 2 (+)] ───[MID_TAP]                │
                      │                       │                    │
               [HY2212 Bal 2] (Bleed 68Ω)     │                    │
                      │                       │                    │
                [CELL 2 (-) ] ──────[Q_REV2: P-FET]                │
                      │                       │                    │
                      ▼                       ▼                    │
            [ HY2120-LB 2S BMS ] ─────────────┘                    │
                 │          │                                      │
              (OC FET)   (OD FET)                                  │
                 │          │                                      │
                 ▼          ▼                                      ▼
           [ Dual AO8822 N-FETs ] ──────────────────────────────▶ BATT- (System Return)
```

### Component Details
- **Battery Holders (BH1, BH2):** Keystone 1048 (SMD) or Keystone 1049 (THM) heavy-duty nickel-plated phosphor bronze leaf spring clips.
- **Hardware Protection BMS (U2):** `HY2120-LB` (SOT-23-6):
  - **Overcharge Detection:** $4.280\,\text{V} \pm 0.025\,\text{V}$ per cell.
  - **Overdischarge Detection:** $2.900\,\text{V} \pm 0.050\,\text{V}$ per cell ($5.80\,\text{V}$ pack total).
  - **Overcurrent / Short Circuit:** Trips at $10\,\text{A}$ discharge.
- **Dual Low-Side N-MOSFETs (Q2, Q3):** `AO8822` (TSSOP-8) dual N-channel ($V_{DS} = 20\,\text{V}$, $R_{DS(on)} < 12\,\text{m}\Omega$).
  - Handles bidirectional battery current (charge cutoff via Q2, discharge cutoff via Q3).
- **Passive Cell Balancers (U3, U4):** `HY2212-BB3A` (SOT-23-6):
  - Threshold: $4.200\,\text{V}$.
  - Shunt Resistors ($R_{B1}, R_{B2}$): $2 \times 68\,\Omega$ 1206 ($0.5\,\text{W}$ rated) providing $I_{bleed} \approx \frac{4.20\,\text{V}}{68\,\Omega} \approx 62\,\text{mA}$.
- **Reverse Cell Insertion Protection (Q4, Q5):** Low-$R_{DS(on)}$ P-channel MOSFETs in series with each battery terminal, with gate tied to the opposing terminal. If a cell is inserted backwards, $V_{GS} > 0\,\text{V}$ and the MOSFET remains completely open, preventing short-circuit fires.
- **Cell Temperature Sensing (RT1):** $10\,\text{k}\Omega$ NTC thermistor ($B = 3950\,\text{K}$) placed in mechanical contact with cell clips, wired to RP2040 ADC3 and MP2762A NTC pin.

---

## Subsystem 3: 2S Switching Charger & Seamless Power Path (MP2762A)

### Purpose
Implements high-efficiency switching battery charging from `V_BUS_IN` (9V–20V) and generates the intermediate system power rail `V_SYS`. Provides seamless 0ms switchover from wall power to battery.

```text
  V_BUS_IN (9-20V) ─────────────────┐
                                    ▼
                          ┌───────────────────┐
                          │   MP2762A (MPS)   │
                          │ - Buck Charger    │──────▶ L1 (2.2µH) ───▶ BATT+ (Charge)
                          │ - Power Path FETs │
                          │ - I2C Telemetry   │
                          └─────────┬─────────┘
                                    │
                                    ▼
                         V_SYS Rail (6.0V - 20V)
                   (Zero Millisecond Diode-OR Power Rail)
```

### Component Details
- **Charger IC (U5):** `MP2762A` (QFN-30, 4mm × 5mm):
  - Integrated input blocking FET, high-side buck FET, and low-side synchronous rectifier.
  - Programmable input current limit ($100\,\text{mA}$ to $3.0\,\text{A}$) and charge current ($0.5\,\text{A}$ to $2.5\,\text{A}$).
  - Fully autonomous hardware power path: When `V_BUS_IN` is connected, `V_SYS` is powered directly from input power while concurrently charging the battery. When `V_BUS_IN` is unplugged, the internal battery power switch closes in $< 5\,\mu\text{s}$, seamlessly transferring `V_SYS` to the battery.
- **Power Inductor (L1):** $2.2\,\mu\text{H}$, $I_{sat} > 6.0\,\text{A}$, $DCR < 20\,\text{m}\Omega$ (e.g. Coilcraft XGL4020-222MEC or Sunlord MWSA0503-2R2MT).
- **Current Shunt (R_SNS):** $10\,\text{m}\Omega$, 1%, 1206 package ($0.5\,\text{W}$).
- **Capacitors:**
  - Input: $2 \times 22\,\mu\text{F}$ 25V X7R ceramic (1206).
  - System ($V_{SYS}$): $3 \times 22\,\mu\text{F}$ 25V X7R ceramic + $1 \times 100\,\mu\text{F}$ 25V conductive polymer aluminum solid cap (e.g. Panasonic EEH-ZA1E101XP).

---

## Subsystem 4: High-Efficiency Synchronous Buck-Boost 5.1V @ 5A (TPS55289)

### Purpose
Steps down (when $V_{SYS} > 5.1\text{V}$, e.g. 12V wall or 8.4V full battery) or steps up (when $V_{SYS}$ dips to 6.0V near empty) to maintain an unwavering **5.10V @ 5.0A continuous (25.5W)** to the Raspberry Pi 5.

```text
  V_SYS (6.0V - 20V) ───────────────┐
                                    ▼
                          ┌───────────────────┐
                          │     TPS55289      │
   RP2040 GPIO15 ────────▶│ EN (Power-Cut Pin)│──────▶ L2 (1.5µH) ───▶ V_OUT (5.10V @ 5A)
   (Zombie Halt Control)  │ 4-Switch Synchron.│
                          │ Buck-Boost        │
                          └───────────────────┘
```

### Component Details & Mathematical Design
- **Buck-Boost Controller/Converter (U6):** `TPS55289` (QFN-21, 3.0mm × 3.5mm, integrated 4-MOSFET H-bridge):
  - Peak switch current limit: $8.5\,\text{A}$.
  - Switching frequency: $600\,\text{kHz}$ (selectable via $R_{FS}$ resistor).
  - Feedback voltage: $0.800\,\text{V}$ reference.
- **Feedback Voltage Divider ($R_{FB1}, R_{FB2}$):**
  $$V_{OUT} = V_{REF} \times \left(1 + \frac{R_{FB1}}{R_{FB2}}\right)$$
  - Choosing $R_{FB2} = 10.0\,\text{k}\Omega$ (0.1%):
    $$5.10\,\text{V} = 0.80\,\text{V} \times \left(1 + \frac{R_{FB1}}{10.0\,\text{k}\Omega}\right) \implies \frac{R_{FB1}}{10.0\,\text{k}\Omega} = 5.375 \implies R_{FB1} = 53.75\,\text{k}\Omega$$
  - Standard value: $53.6\,\text{k}\Omega$ (0.1%) yielding $V_{OUT} = 0.80 \times (1 + 5.36) = 5.088\,\text{V}$, or $53.6\,\text{k}\Omega + 150\,\Omega$ series combination for exact $5.100\,\text{V}$.
- **Power Inductor (L2):** $1.5\,\mu\text{H}$, $I_{sat} > 10.5\,\text{A}$, $DCR < 12\,\text{m}\Omega$ (e.g. Coilcraft XGL6030-152MEC or Wurth 744314150).
- **Output Capacitor Network (Critical for Pi 5 transient load step):**
  - Raspberry Pi 5 executes aggressive quad-core DVFS load steps (0.5A to 4.5A in $< 1\,\mu\text{s}$). To guarantee $V_{OUT} > 4.75\,\text{V}$ at all times:
    - $1 \times 330\,\mu\text{F}$ 10V ultra-low ESR ($< 15\,\text{m}\Omega$) conductive polymer aluminum solid electrolytic capacitor (Panasonic SEPF series).
    - $4 \times 47\,\mu\text{F}$ 10V X7R ceramic (1206) placed directly adjacent to output pins.
    - Total output capacitance: $\approx 518\,\mu\text{F}$, holding transient sag to $< 120\,\text{mV}$.
- **Enable Control (`EN` Pin):**
  - Connected to RP2040 `GPIO15` with a $100\,\text{k}\Omega$ pull-up to $V_{SYS}$ (default ON).
  - When `ShutdownImminent` 45-second timer expires, RP2040 drives GPIO15 LOW to cut output power completely, clearing the Raspberry Pi 5 PMIC "zombie halt" state.

---

## Subsystem 5: Raspberry Pi 5 Host Interface & USB-C Power/Data

### Purpose
Carries both 5.1V/5A power and native USB 2.0 D+/D- bidirectional communication to the Raspberry Pi 5 over a single USB-C cable.

```text
[Pi-cemaker Output: J2 (USB-C Receptacle)]                 [Raspberry Pi 5 USB-C Port]
  VBUS (A4, A9, B4, B9) ───[ V_OUT: 5.10V @ 5A ]────────▶ VBUS (Powers Pi 5)
  GND  (A1, A12, B1, B12) ──[ System GND ]──────────────▶ GND
  D+   (A6, B6) ───────────[ RP2040 USB_DP via 27Ω ]─────▶ D+ (USB HID UPS + CDC)
  D-   (A7, B7) ───────────[ RP2040 USB_DM via 27Ω ]─────▶ D- (USB HID UPS + CDC)
  CC1  (A5) ───────────────[ 10kΩ Pull-Up to 5V ]────────▶ CC1 (10k Rp = 3A source; 5A needs PD)
  CC2  (B5) ───────────────[ 10kΩ Pull-Up to 5V ]────────▶ CC2 (10k Rp = 3A source; 5A needs PD)

[Auxiliary Header: J3 (JST-SH 4-pin)]
  Pin 1: GND | Pin 2: USB_DP | Pin 3: USB_DM | Pin 4: VBUS_SENSE

[Hardware Power Button Header: J4 (JST-SH 2-pin)]
  Pin 1: GND | Pin 2: Open-Drain 2N7002 FET (driven by RP2040 GPIO14)
  ───▶ Connected to Raspberry Pi 5 JST "PWR_BTN" Header
```

### Component Details
- **USB-C Output Receptacle (J2):** High-current 16-pin / 24-pin USB-C connector rated for 5A continuous.
- **CC Pin Configuration:**
  - $10\,\text{k}\Omega$ 1% pull-up resistors to 5.0V on CC1 and CC2. A 10 kΩ Rp to 5 V advertises a **3 A** source (verify against the USB Type-C spec Rp table); 5 A is only signalled through USB PD and needs an e-marked cable. Without a 5 V/5 A PDO the Raspberry Pi 5 limits its USB ports to 600 mA unless `usb_max_current_enable=1` (config.txt) or `PSU_MAX_CURRENT=5000` (EEPROM) is set. See issue #7.
- **USB Data Line Protection (ESD1, ESD2):**
  - Ultra-low capacitance ($< 0.5\,\text{pF}$) TVS array (e.g. `USBLC6-2SC6`) on D+ and D- lines.
  - $27\,\Omega$ series termination resistors between RP2040 USB pins and J2.
- **Auxiliary USB Link (J3):** 4-pin JST-SH connector allowing connection to one of the Pi 5's USB-A host ports for users who do not want to configure `otg_mode=1` on the Pi's power port.
- **Power Button Header (J4):** 2-pin JST-SH connector driven by an N-channel FET (`2N7002`), allowing the RP2040 to toggle the Pi 5 hardware power button.

---

## Subsystem 6: RP2040 Microcontroller & 3.3V Logic Rail

### Purpose
Executes the USB HID Power Device stack, reads all analog voltages and temperatures, executes the power-cut state machine, and provides the serial CLI.

```text
  V_SYS (6.0V - 20V) ──▶ [ U7: TPS70933 LDO ] ──▶ V_MCU_3V3 (3.3V, Iq < 2µA)
                                                        │
                                                        ▼
                                             ┌─────────────────────┐
                                             │     RP2040 MCU      │
  V_BAT ───[100k / 33k Divider] ────────────▶│ ADC0 (GPIO26)       │
  V_BUS_IN─[100k / 15k Divider] ────────────▶│ ADC1 (GPIO27)       │
  V_OUT ───[100k / 100k Divider]────────────▶│ ADC2 (GPIO28)       │
  RT1 ─────[10k NTC Divider]    ────────────▶│ ADC3 (GPIO29)       │
                                             │                     │
                                             │ GPIO15 ─────────────┼──▶ EN_5V_REG (TPS55289)
                                             │ GPIO14 ─────────────┼──▶ PWR_BTN_GATE (2N7002)
                                             │ GPIO16/17/18 ───────┼──▶ Status LEDs
                                             │ GPIO19 ─────────────┼──▶ User Button
                                             │ I2C0 (GPIO4/5) ─────┼──▶ MP2762A + Stemma QT
                                             │ USB_DP / USB_DM ────┼──▶ Host USB Link
                                             │ QSPI ───────────────┼──▶ W25Q128JVS (16MB Flash)
                                             │ XIN / XOUT ─────────┼──▶ 12.000 MHz Crystal
                                             └─────────────────────┘
```

### Component Details
- **Low-Quiescent 3.3V LDO (U7):** `TPS70933DBVR` (SOT-23-5):
  - Ultra-low quiescent current: $I_Q = 1.4\,\mu\text{A}$ typical.
  - Wide input voltage: $2.7\,\text{V}$ to $30.0\,\text{V}$ (operates directly from `V_SYS`).
  - Guarantees that the MCU remains powered in both wall and battery states without draining the cells during sleep.
- **Microcontroller (U8):** `RP2040` (QFN-56, 7mm × 7mm).
- **QSPI Flash (U9):** `W25Q128JVS` (16MB SPI NOR Flash, SOIC-8).
- **Crystal Oscillator (Y1):** $12.000\,\text{MHz}$ ($\pm 10\,\text{ppm}$, 18pF load capacitance) with $2 \times 22\,\text{pF}$ ceramic caps.
- **Analog Dividers (0.1% Metal Film Precision Resistors):**
  - **ADC0 (`V_BAT`):** $R_{top} = 100\,\text{k}\Omega$, $R_{bot} = 33.0\,\text{k}\Omega$.
    $$V_{ADC0} = V_{BAT} \times \frac{33.0}{133.0} \implies \text{At } 8.40\,\text{V}, V_{ADC0} = 2.084\,\text{V}$$
  - **ADC1 (`V_BUS_IN`):** $R_{top} = 100\,\text{k}\Omega$, $R_{bot} = 15.0\,\text{k}\Omega$.
    $$V_{ADC1} = V_{BUS\_IN} \times \frac{15.0}{115.0} \implies \text{At } 20.0\,\text{V}, V_{ADC1} = 2.609\,\text{V}$$
  - **ADC2 (`V_OUT`):** $R_{top} = 100\,\text{k}\Omega$, $R_{bot} = 100.0\,\text{k}\Omega$.
    $$V_{ADC2} = V_{OUT} \times \frac{100.0}{200.0} \implies \text{At } 5.10\,\text{V}, V_{ADC2} = 2.550\,\text{V}$$
- **Status LEDs:**
  - `D3` (Green): Power Rail Active (GPIO16).
  - `D4` (Amber): Battery State / Charging / Discharging (GPIO17).
  - `D5` (Red): Fault / Low Battery Warning (GPIO18).
- **Expansion Connector (J5):** 4-pin JST-SH (Stemma QT / Qwiic compatible) wired to RP2040 I2C0 (`GPIO4` SDA, `GPIO5` SCL, 3.3V, GND) for optional external OLED displays or environmental sensors.
