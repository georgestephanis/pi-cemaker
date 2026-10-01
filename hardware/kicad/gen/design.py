"""Lite board (3 A / 5.1 V, 2-layer) as data. See hardware/DECISIONS.md (D#) for the reasoning.

PARTS: list of dicts
  ref, sym ("Lib:Name"), value, fp (footprint "Lib:Name"), pins {key: net}, group, mpn (optional)
pin key: pin NAME applies to every pin with that name; "#12" selects the pin NUMBER 12.
Pins of a part not mentioned in `pins` and not in `nc` raise an error (so nothing is silently floating).
"""

R0603 = "Resistor_SMD:R_0603_1608Metric"
C0603 = "Capacitor_SMD:C_0603_1608Metric"
C0805 = "Capacitor_SMD:C_0805_2012Metric"
C1206 = "Capacitor_SMD:C_1206_3216Metric"
C1210 = "Capacitor_SMD:C_1210_3225Metric"
TP = "TestPoint:TestPoint_Pad_D1.0mm"
JP = "Jumper:SolderJumper-2_P1.3mm_Bridged_RoundedPad1.0x1.5mm"

PARTS = []
def add(ref, sym, value, fp, pins, group, mpn="", nc=(), dnp=False):
    PARTS.append(dict(ref=ref, sym=sym, value=value, fp=fp, pins=pins, group=group, mpn=mpn, nc=list(nc), dnp=dnp))

def res(ref, val, a, b, group, dnp=False, fp=R0603):
    add(ref, "Device:R", val, fp, {"#1": a, "#2": b}, group, dnp=dnp)
def cap(ref, val, a, b, group, fp=C0603, dnp=False, mpn=""):
    add(ref, "Device:C", val, fp, {"#1": a, "#2": b}, group, dnp=dnp, mpn=mpn)
def tp(ref, net, group):
    add(ref, "Connector:TestPoint", net, TP, {"#1": net}, group)

# ---------------------------------------------------------------- INPUT (J1, HUSB238)
g = "1 Input / PD sink"
add("J1", "Connector:USB_C_Receptacle_USB2.0_16P", "USB-C in (PD sink)",
    "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal",
    {"GND": "GND", "VBUS": "VBUS_RAW", "CC1": "CC1_IN", "CC2": "CC2_IN", "D+": "USB_DP", "D-": "USB_DM",
     "SHIELD": "GND"}, g, mpn="GCT USB4105-GF-A (verify)", nc=["SBU1", "SBU2"])
add("F1", "Device:Fuse", "3.15A or 0R (fit one)", "Fuse:Fuse_1206_3216Metric", {"#1": "VBUS_RAW", "#2": "VBUS"}, g)
add("D1", "Device:D_TVS", "SMAJ13A", "Diode_SMD:D_SMA", {"#1": "VBUS", "#2": "GND"}, g, mpn="SMAJ13A (verify clamp vs 30 V parts)")
add("U1", "Interface_USB:HUSB238_xxxDD", "HUSB238", "Package_DFN_QFN:DFN-10-1EP_3x3mm_P0.5mm_EP1.65x2.38mm",
    {"VIN": "VBUS", "D+": "USB_DP", "D-": "USB_DM", "CC1": "CC1_IN", "CC2": "CC2_IN", "SDA": "SDA", "SCL": "SCL",
     "VSET": "HVSET", "ISET": "HISET", "GND": "GND"}, g, mpn="Hynetek HUSB238 DFN-10 (verify package)", nc=["GATE"])
cap("C1", "1uF 25V", "VBUS", "GND", g)
res("R1", "10k (12 V)", "HVSET", "GND", g)
res("R2", "22.6k (3 A)", "HISET", "GND", g)
tp("TP1", "VBUS", g)

# ---------------------------------------------------------------- CHARGER (BQ25792)
g = "2 Charger BQ25792"
add("U2", "pi-cemaker:BQ25792", "BQ25792", "pi-cemaker:VQFN-HR-29_RQM0029A_4x4mm",
    {"STAT": "STAT", "#2": "VBUS", "#3": "VBUS", "BTST1": "BTST1", "REGN": "REGN", "VAC2": "VBUS", "VAC1": "VBUS",
     "ACDRV2": "GND", "ACDRV1": "GND", "QON": "QON", "CE": "CE", "SCL": "SCL", "SDA": "SDA", "TS": "TS",
     "ILIM_HIZ": "REGN", "BATP": "BATP", "BTST2": "BTST2", "PROG": "PROG", "INT": "INT", "#22": "BAT", "#23": "BAT",
     "SDRV": "SDRV", "SYS": "VSYS", "SW2": "SW2", "GND": "GND", "SW1": "SW1", "PMID": "PMID"},
    g, mpn="TI BQ25792RQMR", nc=["D+", "D-"])
add("L1", "Device:L", "1uH >=8A sat", "Inductor_SMD:L_Bourns_SRP7028A_7.3x6.6mm", {"#1": "SW1", "#2": "SW2"}, g,
    mpn="1 uH, Isat>=8 A shielded (verify)")
cap("C2", "10uF 25V", "VBUS", "GND", g, C1206); cap("C3", "10uF 25V", "VBUS", "GND", g, C1206); cap("C4", "100nF 25V", "VBUS", "GND", g)
for i in (5, 6, 7): cap(f"C{i}", "10uF 25V", "PMID", "GND", g, C1206)
cap("C8", "100nF 25V", "PMID", "GND", g)
cap("C9", "47nF 25V", "BTST1", "SW1", g); cap("C10", "47nF 25V", "BTST2", "SW2", g)
cap("C11", "4.7uF 10V", "REGN", "GND", g, C0805)
for i in (12, 13, 14, 15, 16): cap(f"C{i}", "10uF 25V", "VSYS", "GND", g, C1206)
cap("C17", "100nF 25V", "VSYS", "GND", g)
cap("C18", "10uF 16V", "BAT", "GND", g, C1206); cap("C19", "10uF 16V", "BAT", "GND", g, C1206)
cap("C20", "1nF 50V", "SDRV", "GND", g)
res("R3", "6.04k 1% (2S,1.5MHz)", "PROG", "GND", g)
res("R4", "10k (CE low = charge on)", "CE", "GND", g)
res("R5", "100R", "PACK_P", "BATP", g)   # BATP sense lead
res("R6", "5.24k (RT1)", "REGN", "TS", g)
res("R7", "30.31k (RT2)", "TS", "GND", g)
res("R8", "10k fixed NTC stand-in", "TS", "GND", g)       # D10; DNP when real NTC fitted
res("R9", "10k", "STAT", "+3V3", g); res("R10", "10k", "INT", "+3V3", g)
tp("TP2", "PMID", g); tp("TP3", "SW1", g); tp("TP4", "SW2", g); tp("TP5", "REGN", g)
tp("TP6", "TS", g); tp("TP7", "QON", g); tp("TP8", "VSYS", g); tp("TP9", "BAT", g)
tp("TP10", "STAT", g); tp("TP11", "INT", g)
add("J6", "Connector_Generic:Conn_01x02", "NTC (optional)", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
    {"#1": "TS", "#2": "GND"}, g)

# ---------------------------------------------------------------- PACK + LINKS
g = "3 Pack and links"
add("J3", "Connector_Generic:Conn_01x02", "PACK+ / PACK- (off-board 2S BMS)",
    "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2_1x02_P5.00mm_Horizontal", {"#1": "PACK_P", "#2": "GND"}, g)
add("JP3", "Jumper:SolderJumper_2_Bridged", "PACK link", JP, {"#1": "PACK_P", "#2": "BAT"}, g)
add("JP1", "Jumper:SolderJumper_2_Bridged", "VSYS link", JP, {"#1": "VSYS", "#2": "VSYS_B"}, g)
add("JP2", "Jumper:SolderJumper_2_Bridged", "5V link", JP, {"#1": "V5_B", "#2": "V5"}, g)
tp("TP12", "PACK_P", g)

# ---------------------------------------------------------------- 5 V BUCK (LMR51450)
g = "4 5V buck LMR51450"
add("U3", "pi-cemaker:LMR51450", "LMR51450SDRRR", "Package_SON:WSON-12-1EP_3x3mm_P0.5mm_EP1.5x2.5mm",
    {"SW": "SW_B", "BOOT": "BOOT_B", "PG": "PG", "FB": "FB", "AGND": "GND", "EN": "EN_B", "VIN": "VSYS_B", "#13": "GND"},
    g, mpn="TI LMR51450SDRRR", nc=["RT"])
cap("C21", "100nF 16V", "BOOT_B", "SW_B", g)
add("L2", "Device:L", "4.7uH >=8A sat", "Inductor_SMD:L_Bourns_SRP7028A_7.3x6.6mm", {"#1": "SW_B", "#2": "V5_B"}, g,
    mpn="4.7 uH, Isat>=8 A (verify)")
cap("C22", "10uF 25V", "VSYS_B", "GND", g, C1206); cap("C23", "10uF 25V", "VSYS_B", "GND", g, C1206); cap("C24", "100nF 25V", "VSYS_B", "GND", g)
cap("C25", "33uF 16V", "V5_B", "GND", g, C1210); cap("C26", "33uF 16V", "V5_B", "GND", g, C1210)
cap("C27", "33uF 16V (spare)", "V5_B", "GND", g, C1210, dnp=True); cap("C28", "33uF 16V (spare)", "V5_B", "GND", g, C1210, dnp=True)
res("R11", "102k (FBT)", "V5_B", "FB", g); res("R12", "19.1k (FBB)", "FB", "GND", g)
cap("C29", "33pF (CFF)", "V5_B", "FB", g)
res("R13", "62k (ENT)", "VSYS_B", "EN_B", g); res("R14", "21.5k (ENB)", "EN_B", "GND", g)
res("R15", "10k", "PG", "+3V3", g)
add("Q1", "Transistor_FET:2N7002", "2N7002", "Package_TO_SOT_SMD:SOT-23", {"G": "EN_KILL", "S": "GND", "D": "EN_B"}, g)
res("R16", "100k (gate pull-down)", "EN_KILL", "GND", g)
tp("TP13", "SW_B", g); tp("TP14", "EN_B", g); tp("TP15", "V5_B", g); tp("TP16", "FB", g); tp("TP17", "PG", g)
tp("TP18", "VSYS_B", g)

# ---------------------------------------------------------------- OUTPUT
g = "5 Output"
add("J2", "Connector:USB_C_Receptacle_USB2.0_16P", "USB-C out to Pi 5 (power only)",
    "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal",
    {"GND": "GND", "VBUS": "V5", "CC1": "CC1_OUT", "CC2": "CC2_OUT", "SHIELD": "GND"}, g, mpn="GCT USB4105-GF-A (verify)",
    nc=["D+", "D-", "SBU1", "SBU2"])
res("R17", "10k (Rp, 3 A)", "V5", "CC1_OUT", g); res("R18", "10k (Rp, 3 A)", "V5", "CC2_OUT", g)
add("J4", "Connector_Generic:Conn_01x02", "5V OUT / GND",
    "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2_1x02_P5.00mm_Horizontal", {"#1": "V5", "#2": "GND"}, g)
cap("C30", "10uF 10V", "V5", "GND", g, C0805); tp("TP19", "V5", g)
add("D2", "Device:D_Schottky", "SS34", "Diode_SMD:D_SMA", {"K": "PICO_VSYS", "A": "V5"}, g, mpn="SS34 (verify)")

# ---------------------------------------------------------------- MCU
g = "6 MCU (Pico module)"
add("U4", "MCU_Module:RaspberryPi_Pico", "Raspberry Pi Pico (RP2040)", "Module:RaspberryPi_Pico_Common_THT",
    {"GND": "GND", "AGND": "GND", "3V3": "+3V3", "VSYS": "PICO_VSYS", "GPIO4": "SDA", "GPIO5": "SCL", "GPIO6": "INT",
     "GPIO7": "STAT", "GPIO8": "PG", "GPIO14": "PWR_BTN", "GPIO15": "EN_KILL", "GPIO16": "LED_PWR", "GPIO17": "LED_BAT",
     "GPIO18": "LED_FLT", "GPIO19": "USER_BTN", "RUN": "RUN"}, g, mpn="Pico (headers hand-fitted)",
    nc=["GPIO0", "GPIO1", "GPIO2", "GPIO3", "GPIO9", "GPIO10", "GPIO11", "GPIO12", "GPIO13", "GPIO20", "GPIO21",
        "GPIO22", "GPIO26_ADC0", "GPIO27_ADC1", "GPIO28_ADC2", "ADC_VREF", "3V3_EN", "VBUS"])
res("R19", "10k", "SDA", "+3V3", g); res("R20", "10k", "SCL", "+3V3", g)
for n, (net, col) in enumerate([("LED_PWR", "G"), ("LED_BAT", "A"), ("LED_FLT", "R")]):
    res(f"R{21+n}", "1k", net, f"LEDA{n}", g)
    add(f"D{3+n}", "Device:LED", {"G": "green", "A": "amber", "R": "red"}[col], "LED_SMD:LED_0603_1608Metric",
        {"A": f"LEDA{n}", "K": "GND"}, g)
add("SW1", "Switch:SW_Push", "USER", "Button_Switch_SMD:SW_SPST_PTS810", {"#1": "USER_BTN", "#2": "GND"}, g)
add("Q2", "Transistor_FET:2N7002", "2N7002", "Package_TO_SOT_SMD:SOT-23", {"G": "PWR_BTN", "S": "GND", "D": "PI_PWR_BTN"}, g)
res("R24", "100k (gate pull-down)", "PWR_BTN", "GND", g)
add("J5", "Connector_Generic:Conn_01x02", "To Pi 5 J2 power button", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
    {"#1": "PI_PWR_BTN", "#2": "GND"}, g)
add("J7", "Connector_Generic:Conn_01x02", "I2C expansion (3V3 SDA)", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
    {"#1": "SDA", "#2": "SCL"}, g)
tp("TP20", "+3V3", g); tp("TP21", "SDA", g); tp("TP22", "SCL", g); tp("TP23", "RUN", g)
for i in range(4): tp(f"TP{24+i}", "GND", g)
for i in range(4):
    add(f"H{i+1}", "Mechanical:MountingHole", "M2.5", "MountingHole:MountingHole_2.7mm_M2.5_Pad", {}, "7 Mechanical")

POWER_FLAG_NETS = ["VBUS_RAW", "VBUS", "PMID", "VSYS", "VSYS_B", "PACK_P", "BAT", "V5_B", "V5", "PICO_VSYS", "GND"]

# custom symbols: name -> (left pins, right pins); pin = (number, name, electrical type)
CUSTOM = {
    "BQ25792": dict(
        left=[("1", "STAT", "open_collector"), ("2", "VBUS", "power_in"), ("3", "VBUS", "power_in"),
              ("9", "VAC1", "power_in"), ("8", "VAC2", "power_in"), ("11", "ACDRV1", "passive"), ("10", "ACDRV2", "passive"),
              ("12", "QON", "passive"), ("13", "CE", "input"), ("14", "SCL", "input"), ("15", "SDA", "bidirectional"),
              ("6", "D+", "passive"), ("7", "D-", "passive"), ("4", "BTST1", "passive"), ("5", "REGN", "power_out"),
              ("29", "PMID", "power_in")],
        right=[("28", "SW1", "passive"), ("26", "SW2", "passive"), ("19", "BTST2", "passive"), ("25", "SYS", "power_in"),
               ("22", "BAT", "power_in"), ("23", "BAT", "power_in"), ("18", "BATP", "input"), ("24", "SDRV", "passive"),
               ("16", "TS", "passive"), ("17", "ILIM_HIZ", "passive"), ("20", "PROG", "passive"), ("21", "INT", "open_collector"),
               ("27", "GND", "power_in")]),
    "LMR51450": dict(
        left=[("10", "VIN", "power_in"), ("11", "VIN", "power_in"), ("12", "VIN", "power_in"), ("9", "EN", "input"),
              ("6", "RT", "passive"), ("7", "FB", "input")],
        right=[("1", "SW", "passive"), ("2", "SW", "passive"), ("3", "SW", "passive"), ("4", "BOOT", "passive"),
               ("5", "PG", "open_collector"), ("8", "AGND", "power_in"), ("13", "PGND", "power_in")]),
}
