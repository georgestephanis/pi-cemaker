#!/usr/bin/env python3
"""Create pi-cemaker.kicad_pcb from netlist.json: footprints, nets, placement, outline. Run with KiCad's python:
  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 gen_pcb.py"""
import json, os, sys, math
import pcbnew
from pcbnew import FromMM as mm, VECTOR2I
HERE = os.path.dirname(os.path.abspath(__file__)); OUT = os.path.join(HERE, "..")
FPROOT = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"
nl = json.load(open(os.path.join(HERE, "netlist.json")))
W, H = 112.0, 78.0

bd = pcbnew.NewBoard(os.path.join(OUT, "pi-cemaker.kicad_pcb"))
bd.SetCopperLayerCount(2)
nets = {}
def net(name):
    if name not in nets:
        ni = pcbnew.NETINFO_ITEM(bd, name); bd.Add(ni); nets[name] = ni
    return nets[name]

# ---- placement table: ref -> (x, y, rot_deg, side)   (centers, mm; board origin top-left)
P = {
 # connectors
 "J1": (8.0, 14.0, 270), "J2": (8.0, 62.0, 270), "J3": (60.0, 73.5, 0), "J4": (47.0, 73.5, 0),
 "J5": (73.0, 73.0, 0), "J6": (34.0, 73.5, 0), "J7": (79.0, 73.0, 0),
 # ICs and magnetics
 "U1": (24.0, 10.0, 0), "U2": (40.0, 30.0, 0), "L1": (40.0, 19.0, 0),
 "U3": (62.0, 46.0, 0), "L2": (64.0, 59.0, 0), "U4": (84.0, 13.0, 0),
 "Q1": (50.0, 41.0, 0), "Q2": (75.0, 62.0, 0), "SW1": (88.0, 72.0, 0), "D2": (72.0, 4.5, 0),
 "JP1": (56.0, 38.0, 0), "JP2": (74.0, 52.0, 0), "JP3": (51.0, 24.0, 0),
 "H1": (3.5, 3.5, 0), "H2": (108.5, 3.5, 0), "H3": (3.5, 74.5, 0), "H4": (108.5, 74.5, 0),
}
# rows of passives: (ref list, x0, y0, pitch_x, pitch_y, per_row)
REG = [
 (["F1", "D1"], 14.0, 22.0, 8.0, 0, 2),
 (["C1", "R1", "R2", "TP1"], 14.0, 28.0, 4.0, 4.0, 4),
 (["C2", "C3", "C4", "C5", "C6", "C7", "C8"], 22.0, 36.0, 5.0, 5.0, 4),
 (["C9", "C10", "C11", "C20"], 26.0, 14.0, 4.5, 0, 4),
 (["C12", "C13", "C14", "C15", "C16", "C17"], 50.0, 26.0, 5.5, 5.0, 3),
 (["C18", "C19", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10"], 27.0, 44.0, 5.2, 4.0, 5),
 (["TP2", "TP3", "TP4", "TP5", "TP6", "TP7", "TP8", "TP9", "TP10", "TP11", "TP12"], 6.0, 44.0, 3.5, 3.5, 6),
 (["C21", "C22", "C23", "C24", "C25", "C26", "C27", "C28", "C29"], 66.0, 28.0, 6.0, 5.0, 4),
 (["R11", "R12", "R13", "R14", "R15", "R16"], 48.0, 48.0, 4.2, 4.0, 3),
 (["TP13", "TP14", "TP15", "TP16", "TP17", "TP18", "TP19"], 44.0, 58.0, 3.5, 3.5, 4),
 (["R17", "R18", "C30"], 16.0, 56.0, 4.0, 4.0, 3),
 (["R19", "R20", "R21", "R22", "R23", "D3", "D4", "D5", "R24"], 70.0, 12.0, 4.0, 4.0, 5),
 (["TP20", "TP21", "TP22", "TP23", "TP24", "TP25", "TP26", "TP27"], 72.0, 38.0, 3.5, 3.5, 4),
]
for refs, x0, y0, px, py, per in REG:
    for i, r in enumerate(refs):
        P[r] = (x0 + (i % per) * px, y0 + (i // per) * (py or 4.0), 0)

missing = [r for r in nl if r not in P]
if missing: print("UNPLACED:", missing); sys.exit(1)

for ref in sorted(nl):
    d = nl[ref]; lib, name = d["fp"].split(":")
    libdir = os.path.join(OUT, "pi-cemaker.pretty") if lib == "pi-cemaker" else os.path.join(FPROOT, lib + ".pretty")
    fp = pcbnew.FootprintLoad(libdir, name)
    if fp is None: print("cannot load", d["fp"]); sys.exit(1)
    fp.SetReference(ref); fp.SetValue(d["value"])
    x, y, rot = P[ref]
    fp.SetPosition(VECTOR2I(mm(x), mm(y))); fp.SetOrientationDegrees(rot)
    for pad in fp.Pads():
        n = pad.GetNumber()
        if n in d["pads"]: pad.SetNet(net(d["pads"][n]))
    bd.Add(fp)

# outline
pts = [(0, 0), (W, 0), (W, H), (0, H)]
for i in range(4):
    s = pcbnew.PCB_SHAPE(bd); s.SetShape(pcbnew.SHAPE_T_SEGMENT); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.1))
    s.SetStart(VECTOR2I(mm(pts[i][0]), mm(pts[i][1]))); s.SetEnd(VECTOR2I(mm(pts[(i + 1) % 4][0]), mm(pts[(i + 1) % 4][1]))); bd.Add(s)
# GND pour on both layers
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(bd); z.SetLayer(layer); z.SetNet(net("GND"))
    z.SetMinThickness(mm(0.2)); z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetLocalClearance(mm(0.25))
    o = z.Outline(); o.NewOutline()
    for px, py in [(0.5, 0.5), (W - 0.5, 0.5), (W - 0.5, H - 0.5), (0.5, H - 0.5)]: o.Append(mm(px), mm(py))
    bd.Add(z)
bd.Save(os.path.join(OUT, "pi-cemaker.kicad_pcb"))
print("saved", len(nl), "footprints")
