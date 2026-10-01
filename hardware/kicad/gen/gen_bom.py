#!/usr/bin/env python3
"""Draft BOM (grouped by value+footprint) from netlist.json. MPN column is blank/`verify` for most lines on purpose."""
import json, csv, os, collections
HERE = os.path.dirname(os.path.abspath(__file__))
nl = json.load(open(os.path.join(HERE, "netlist.json")))
g = collections.defaultdict(list)
for ref, p in nl.items(): g[(p["value"], p["fp"], p["mpn"], p["dnp"])].append(ref)
with open(os.path.join(HERE, "..", "..", "BOM_DRAFT.csv"), "w", newline="") as f:
    w = csv.writer(f); w.writerow(["Refs", "Qty", "Value", "Footprint", "MPN", "DNP", "Status"])
    for (v, fp, mpn, dnp), refs in sorted(g.items(), key=lambda kv: kv[1][0]):
        w.writerow([" ".join(sorted(refs)), len(refs), v, fp, mpn, "DNP" if dnp else "", "MPN verified" if mpn and "verify" not in mpn else "UNVERIFIED"])
print(len(g), "lines")
