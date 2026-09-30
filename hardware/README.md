# 🔌 Pi-cemaker Hardware Design & Architecture

This directory contains the electrical circuit designs, schematics, Bill of Materials (BOM), and PCB layout guidelines for the **Pi-cemaker** 2× 18650 USB-C PD DC UPS for Raspberry Pi 5.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This hardware design is in active development and is **not complete yet**. It is **certainly NOT for sale** or production deployment. Schematics, components, and board layouts are evolving prototypes shared for open-source engineering development.

---

## ⚡ Key Electrical Specifications

| Parameter | Specification | Circuit Implementation |
|---|---|---|
| **Input Voltage (`V_BUS_IN`)** | 9.0V to 20.0V DC | Negotiated via HUSB238 USB-PD Sink controller |
| **Input Inrush / Surge** | TVS protected + P-FET reverse blocking | SMAJ24A TVS + DMP3010LK3 P-MOSFET |
| **Battery Configuration** | 2S1P (2× 18650 Li-ion cells) | Keystone 1048 (SMD) or 1049 (THM) holders |
| **Pack Voltage (`V_BAT`)** | 6.0V (cutoff) – 7.4V (nom) – 8.4V (max) | 2S cell protection + active cell balancing |
| **Battery Protection** | Hardware BMS (independent of MCU) | HY2120-LB + Dual AO8822 N-channel power FETs |
| **Passive Balancing** | 4.20V threshold, ~60mA bleed current | Dual HY2212 balance ICs + 68Ω power resistors |
| **Reverse Cell Protection** | Individual cell polarity protection | Low-RDS(on) P-FET per battery bay |
| **2S Charger Subsystem** | Switching buck charger with power path | Monolithic MP2762A (up to 2.5A charge rate) |
| **System Rail (`V_SYS`)** | 6.0V to 20.0V DC | Seamless ideal diode-OR switchover (0ms delay) |
| **Regulated Output (`V_OUT`)** | **5.10V ± 1.5% @ 5.0A continuous (25.5W)** | TPS55289 synchronous 4-switch buck-boost |
| **Output Transient Sag** | < 150mV sag under 0A ➔ 5A step | Low-ESR solid polymer + multi-layer ceramic array |
| **5V Rail Enable Control** | Logic controlled by RP2040 GPIO15 | Used for 45s post-shutdown zombie halt power-cut |
| **Logic Supply (`V_MCU_3V3`)** | 3.3V @ 150mA | TPS70933 ultra-low Iq LDO (< 2µA quiescent) |
| **Microcontroller** | Raspberry Pi RP2040 | Dual ARM Cortex-M0+ @ 133MHz, 16MB QSPI Flash |
| **Host Link** | Single USB-C cable (5V/5A + USB 2.0 D+/D-) | Emulates USB HID Power Device + CDC serial |
| **Auxiliary Link** | 4-pin JST-SH (GND, D+, D-, 5V) | Allows connection to Pi 5 USB 3.0/2.0 host ports |
| **Hardware Shutdown Header** | 2-pin JST-SH (GND, PWR_BTN) | Open-drain pulse to Pi 5 dedicated power header |

---

## 📐 System Block Diagram

```text
[ USB-C PD Input: 9-20V ]
         │
         ▼
  [ TVS + P-FET Inrush / Rev Protection ]
         │
         ▼
  [ HUSB238 PD Sink ] 
         │
         ├─────────────────────────────────────────────┐
         ▼                                             ▼
  [ MP2762A 2S Charger ]                      [ Ideal Diode-OR Power Path ]
         │                                             │
         ├──────────────────────┐                      │
         ▼                      ▼                      │
  [ HY2120 2S Protection ]  [ HY2212 2S Balancer ]    │
         │                      │                      │
         ▼                      ▼                      ▼
  [ 2x 18650 Cells (2S) ]───────┴───────────────▶ [ V_SYS Rail: 6.0V - 20V ]
  (with Reverse Polarity P-FETs)                       │
                                                       ├──────────────────────────┐
                                                       ▼                          ▼
                                             [ TPS55289 Buck-Boost ]    [ TPS70933 LDO ]
                                             (Controlled by GPIO15)               │
                                                       │                          ▼
                                                       ▼                 [ V_MCU_3V3 Logic ]
                                              [ V_OUT: 5.1V @ 5A ]                │
                                                       │                          ▼
                                                       ├────────────────▶ [ RP2040 Microcontroller ]
                                                       │                  - ADC0: V_BAT
                                                       │                  - ADC1: V_BUS_IN
                                                       │                  - ADC2: V_OUT
                                                       │                  - ADC3: NTC Temp
                                                       ▼                  - USB D+/D- (HID + CDC)
                                            [ USB-C Out to Pi 5 ]
                                            (5.1V 5A Power + Data)
```

---

## 📚 Documentation Index

1. [**Circuit Schematics & Subsystems (`SCHEMATICS.md`)**](SCHEMATICS.md) — Detailed schematic design, component ratings, formulas, and pin connections.
2. [**Bill of Materials (`BOM.md`)**](BOM.md) / [**BOM (CSV)**](BOM.csv) — Complete BOM with MPNs, manufacturers, footprints, and distributor part numbers.
3. [**PCB Layout Guidelines (`PCB_LAYOUT_GUIDELINES.md`)**](PCB_LAYOUT_GUIDELINES.md) — 4-layer stackup, 5A trace sizing, thermal vias, and decoupling layout.
4. [**KiCad Project Files (`kicad/`)**](kicad/) — KiCad 7/8 project files, schematic sheets, and board layout.
