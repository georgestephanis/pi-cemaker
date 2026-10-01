#!/bin/sh
# Regenerate everything from design.py. Needs KiCad 10 installed; Freerouting 2.4.1 + Java 25 for the routing step (optional).
set -e
cd "$(dirname "$0")"
KP=/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3
python3 gen_sch.py && python3 gen_fp.py && python3 gen_bom.py
$KP gen_pcb.py
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch erc --severity-all --exit-code-violations -o /tmp/pcm-erc.rpt ../pi-cemaker.kicad_sch
echo "Schematic regenerated + ERC clean. Routing: export DSN with pcbnew.ExportSpecctraDSN, run freerouting, ImportSpecctraSES (see README)."
