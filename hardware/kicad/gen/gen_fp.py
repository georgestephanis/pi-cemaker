#!/usr/bin/env python3
"""Hand-built footprint for TI BQ25792 (RQM0029A, VQFN-HR 4x4). BEST GUESS from a rendered image of the TI drawing
(see hardware/DECISIONS.md D18). Replace with TI's CAD model before any order."""
import os
pads = []
def pad(n, x, y, w, h): pads.append(f'  (pad "{n}" smd roundrect (at {x:.3f} {y:.3f}) (size {w:.3f} {h:.3f}) (layers "F.Cu" "F.Mask" "F.Paste") (roundrect_rratio 0.25))')
for i in range(9):  pad(i + 1, -1.85, -1.6 + 0.4 * i, 0.5, 0.2)            # left side, pin 1 at top (top view)
for i in range(6):  pad(10 + i, -1.0 + 0.4 * i, 1.85, 0.2, 0.5)            # bottom row, left to right
for i in range(9):  pad(16 + i, 1.85, 1.6 - 0.4 * i, 0.5, 0.2)             # right side, bottom to top
for i in range(5):  pad(25 + i, 1.35 - 0.675 * i, -1.85, 0.45, 0.6)        # top row, right to left
body = ('  (fp_rect (start -2 -2) (end 2 2) (stroke (width 0.1) (type default)) (fill none) (layer "F.Fab"))\n'
        '  (fp_rect (start -2.4 -2.4) (end 2.4 2.4) (stroke (width 0.05) (type default)) (fill none) (layer "F.CrtYd"))\n'
        '  (fp_circle (center -2.4 -1.9) (end -2.3 -1.9) (stroke (width 0.2) (type default)) (fill none) (layer "F.SilkS"))')
txt = ('(footprint "VQFN-HR-29_RQM0029A_4x4mm" (version 20240108) (generator "pi-cemaker-gen") (layer "F.Cu")\n'
       '  (descr "BQ25792 RQM0029A - BEST GUESS, UNVERIFIED (DECISIONS.md D18)")\n'
       '  (property "Reference" "REF**" (at 0 -3.2 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))\n'
       '  (property "Value" "BQ25792" (at 0 3.2 0) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))\n'
       '  (attr smd)\n' + body + "\n" + "\n".join(pads) + "\n)\n")
open(os.path.join(os.path.dirname(__file__), "..", "pi-cemaker.pretty", "VQFN-HR-29_RQM0029A_4x4mm.kicad_mod"), "w").write(txt)
print(len(pads), "pads")
