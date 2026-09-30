# 📋 Pi-cemaker Bill of Materials (BOM)

This document lists the components, footprints, manufacturer part numbers (MPNs), and sourcing information for **Pi-cemaker**.

> [!CAUTION]
> **UNDER ACTIVE CONSTRUCTION / PROTOTYPE PHASE**
> 
> This BOM represents an evolving prototype and is **not complete yet**. It is **certainly NOT for sale**.

---

## Component Summary Table

| Designator | Qty | Value / Description | Package | Manufacturer | MPN | Sourcing (LCSC / DigiKey) |
|---|---|---|---|---|---|---|
| **U1** | 1 | USB-PD Sink Controller (9-20V) | QFN-16 | Hynetek | HUSB238 | LCSC C2838520 |
| **U2** | 1 | 2S Li-ion Battery Protection IC | SOT-23-6 | Hycon | HY2120-LB | LCSC C2682619 |
| **U3, U4** | 2 | 2S Passive Cell Balancer IC (4.20V) | SOT-23-6 | Hycon | HY2212-BB3A | LCSC C347391 |
| **U5** | 1 | 2S Buck Switching Charger with Power Path | QFN-30 (4x5) | MPS | MP2762AGV-Z | DigiKey 1589-MP2762AGV-ZCT-ND |
| **U6** | 1 | 5.1V @ 5A Synchronous Buck-Boost IC | QFN-21 (3x3.5) | Texas Instruments | TPS55289RYHR | DigiKey 296-TPS55289RYHRCT-ND |
| **U7** | 1 | Ultra-low Iq 3.3V LDO (1.4µA Iq) | SOT-23-5 | Texas Instruments | TPS70933DBVR | DigiKey 296-35688-1-ND |
| **U8** | 1 | RP2040 Dual ARM Cortex-M0+ MCU | QFN-56 (7x7) | Raspberry Pi | RP2040 | DigiKey 2648-SC0914CT-ND |
| **U9** | 1 | 16MB SPI NOR Flash Memory | SOIC-8 (208mil) | Winbond | W25Q128JVSIQ | LCSC C97521 |
| **Q1** | 1 | P-Channel MOSFET (-30V, 10mΩ) | PowerDI5060 | Diodes Inc | DMP3010LK3-13 | DigiKey DMP3010LK3-13DICT-ND |
| **Q2, Q3** | 2 | Dual N-Channel Power MOSFET (20V, 12mΩ) | TSSOP-8 | Alpha & Omega | AO8822 | LCSC C28373 |
| **Q4, Q5** | 2 | P-Channel Reverse Polarity FET (-20V) | SOT-23 | Alpha & Omega | AO3401A | LCSC C15127 |
| **Q6** | 1 | N-Channel FET for Pi 5 PWR_BTN Header | SOT-23 | onsemi | 2N7002 | LCSC C8500 |
| **L1** | 1 | 2.2µH 6.5A Power Inductor (Charger) | 5x5mm SMD | Sunlord | MWSA0503-2R2MT | LCSC C408332 |
| **L2** | 1 | 1.5µH 11A High-Current Inductor (5V/5A) | 6.5x6.5mm SMD | Coilcraft / Wurth | 744314150 | DigiKey 732-2253-1-ND |
| **D1** | 1 | 24V Standoff TVS Surge Diode | SMA (DO-214AC) | Littelfuse | SMAJ24A | DigiKey SMAJ24ALFCT-ND |
| **D2** | 1 | 15V Zener Diode (Gate protection) | SOT-23 | Diodes Inc | BZX84C15-7-F | LCSC C2199 |
| **D3** | 1 | Green LED (Power Rail Active) | 0603 | Everlight | 19-217/GHC-YR1S2/3T | LCSC C72043 |
| **D4** | 1 | Amber LED (Battery State) | 0603 | Everlight | 19-217/BHC-AP1Q2/3T | LCSC C72041 |
| **D5** | 1 | Red LED (Fault / Low Battery) | 0603 | Everlight | 19-217/R6C-AL1M2VY/3T| LCSC C72038 |
| **ESD1, ESD2**| 2 | Ultra-Low Cap USB ESD Protection Array | SOT-23-6 | STMicroelectronics | USBLC6-2SC6 | DigiKey 497-5235-1-ND |
| **Y1** | 1 | 12.000 MHz Crystal (18pF, ±10ppm) | 3.2x2.5mm SMD | Yangxing Tech | X322512MOB4SI | LCSC C9002 |
| **BH1, BH2**| 2 | 18650 Battery Holder Clips (SMD) | 18650 SMD bay | Keystone | 1048 | DigiKey 36-1048-ND |
| **J1, J2** | 2 | USB-C 16-pin / 24-pin Receptacle (5A rated) | SMD / Hybrid | Korean Hroparts | TYPE-C-31-M-12 | LCSC C165948 |
| **J3** | 1 | 4-Pin JST-SH Connector (Aux USB Link) | 1.0mm Pitch SMD | JST | SM04B-SRSS-TB | DigiKey 455-1804-1-ND |
| **J4** | 1 | 2-Pin JST-SH Connector (Pi 5 PWR_BTN) | 1.0mm Pitch SMD | JST | SM02B-SRSS-TB | DigiKey 455-1802-1-ND |
| **J5** | 1 | 4-Pin JST-SH Stemma QT / Qwiic I2C | 1.0mm Pitch SMD | JST | SM04B-SRSS-TB | DigiKey 455-1804-1-ND |
| **SW1, SW2**| 2 | Tactile Push Buttons (BOOTSEL, USER) | 3x4x2.5mm SMD | C&K / Omron | PTS636 SM43 LFS | DigiKey CKN10892CT-ND |
| **R_SNS** | 1 | 10mΩ 1% 0.5W Current Sense Shunt | 1206 | Vishay / Walsin | WSL1206R0100FEA | DigiKey WSL-.010CT-ND |
| **R_B1, R_B2**| 2 | 68Ω 0.5W Passive Bleed Resistors | 1206 | Yageo | RC1206FR-0768RL | LCSC C17937 |
| **R_FB1** | 1 | 53.6kΩ 0.1% 25ppm Feedback Resistor | 0603 | Susumu | RR0816P-5362-D-71C | DigiKey RR08P53.6KDCT-ND |
| **R_FB2** | 1 | 10.0kΩ 0.1% 25ppm Feedback Resistor | 0603 | Susumu | RR0816P-1002-D-M | DigiKey RR08P10.0KDCT-ND |
| **R_ADC1..4** | 4 | 100kΩ 0.1% ADC Top Dividers | 0603 | Susumu | RR0816P-1003-D-M | DigiKey RR08P100KDCT-ND |
| **R_ADC_VBAT**| 1 | 33.0kΩ 0.1% ADC Bottom Divider | 0603 | Susumu | RR0816P-3302-D-M | DigiKey RR08P33.0KDCT-ND |
| **R_ADC_VBUS**| 1 | 15.0kΩ 0.1% ADC Bottom Divider | 0603 | Susumu | RR0816P-1502-D-M | DigiKey RR08P15.0KDCT-ND |
| **C_OUT_POLY**| 1 | 330µF 10V Solid Conductive Polymer Cap | Radial SMD 6.3x8| Panasonic | 10SEPF330M | DigiKey P16399CT-ND |
| **C_SYS_POLY**| 1 | 100µF 25V Hybrid Conductive Polymer Cap | Radial SMD 6.3x8| Panasonic | EEH-ZA1E101XP | DigiKey PCE5019CT-ND |
| **C_CER_47U** | 6 | 47µF 10V X7R Ceramic Capacitors | 1206 | Murata | GRM31CR71A476KE15L | DigiKey 490-17932-1-ND |
| **C_CER_22U** | 6 | 22µF 25V X7R Ceramic Capacitors | 1206 | Murata | GRM31CR71E226KE15L | DigiKey 490-13233-1-ND |
| **RT1** | 1 | 10kΩ NTC Thermistor (B=3950K) | 0603 | Murata | NCP18XH103F03RB | DigiKey 490-2434-1-ND |
