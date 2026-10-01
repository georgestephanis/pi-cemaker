#!/usr/bin/env python3
"""Generate pi-cemaker.kicad_sch, pi-cemaker.kicad_sym and netlist.json from design.py.
Schematic style: every pin gets a short stub with a net label (or GND symbol). Readable per-group, not hand-drawn."""
import json, os, re, sys, uuid
sys.path.insert(0, os.path.dirname(__file__))
from kilib import load_symbol, pins
import design

OUT = os.path.join(os.path.dirname(__file__), "..")
ROOT = str(uuid.uuid5(uuid.NAMESPACE_URL, "pi-cemaker-root"))
def U(*a): return str(uuid.uuid5(uuid.NAMESPACE_URL, "pcm/" + "/".join(map(str, a))))
def f(v): return ("%.4f" % v).rstrip("0").rstrip(".")

def custom_symbol(name, spec):
    """Box symbol with pins on left/right. Returns (text, pinlist) in library coordinates (y up)."""
    L, R = spec["left"], spec["right"]
    n = max(len(L), len(R)); h = (n + 1) * 2.54; w = 20.32
    top = h / 2; out = []
    pl = []
    def pin(p, x, ang, y):
        pl.append(dict(etype=p[2], x=x, y=y, ang=ang, length=2.54, name=p[1], number=p[0]))
        return (f'(pin {p[2]} line (at {f(x)} {f(y)} {ang}) (length 2.54) (name "{p[1]}" (effects (font (size 1.27 1.27)))) '
                f'(number "{p[0]}" (effects (font (size 1.27 1.27)))))')
    pins_txt = []
    for i, p in enumerate(L): pins_txt.append(pin(p, -(w / 2 + 2.54), 0, top - 2.54 * (i + 1)))
    for i, p in enumerate(R): pins_txt.append(pin(p, (w / 2 + 2.54), 180, top - 2.54 * (i + 1)))
    txt = (f'(symbol "{name}" (pin_names (offset 1.016)) (exclude_from_sim no) (in_bom yes) (on_board yes)\n'
           f'  (property "Reference" "U" (at 0 {f(top+2.54)} 0) (effects (font (size 1.27 1.27))))\n'
           f'  (property "Value" "{name}" (at 0 {f(-top-2.54)} 0) (effects (font (size 1.27 1.27))))\n'
           f'  (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))\n'
           f'  (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))\n'
           f'  (symbol "{name}_0_1" (rectangle (start {f(-w/2)} {f(top)}) (end {f(w/2)} {f(-top)}) '
           f'(stroke (width 0.254) (type default)) (fill (type background))))\n'
           f'  (symbol "{name}_1_1" {" ".join(pins_txt)}))')
    return txt, pl

# ---- library symbols
libsyms = {}   # "Lib:Name" -> (text, pins)
custom_txt = {}
def get_sym(libid):
    if libid in libsyms: return libsyms[libid]
    lib, name = libid.split(":")
    if lib == "pi-cemaker":
        txt, pl = custom_symbol(name, design.CUSTOM[name]); custom_txt[name] = txt
    else:
        txt = load_symbol(lib, name); pl = pins(txt)
    if libid == "MCU_Module:RaspberryPi_Pico":   # stock symbol marks GND/AGND pins power_out; make them passive (see DECISIONS.md D21)
        txt = re.sub(r'\(pin power_out (\w+)(\s+\(at [^)]*\)\s+\(length [-\d.]+\)(?:\s*\(hide yes\))?\s+\(name "A?GND")', r'(pin passive \1\2', txt)
        pl = pins(txt)
    embedded = txt.replace(f'(symbol "{name}"', f'(symbol "{libid}"', 1)
    libsyms[libid] = (embedded, pl)
    return libsyms[libid]

# ---- netlist + validation
netlist = {}; errors = []
def resolve(part):
    _, pl = get_sym(part["sym"])
    m = {}
    for p in pl:
        net = None; 
        for key in (f"#{p['number']}", p["name"]):
            if key in part["pins"]: net = part["pins"][key]; break
        if net is None:
            if p["name"] in part["nc"] or f"#{p['number']}" in part["nc"]: net = None; m[p["number"]] = ("NC", p)
            else: errors.append(f"{part['ref']}: pin {p['number']} ({p['name']}) unassigned"); m[p["number"]] = ("NC", p)
        else: m[p["number"]] = (net, p)
    unknown = [k for k in part["pins"] if not any(k in (f"#{p['number']}", p["name"]) for p in pl)]
    if unknown: errors.append(f"{part['ref']}: unknown pin keys {unknown}")
    return m

# ---- placement
cells = {}; groups = {}
for p in design.PARTS: groups.setdefault(p["group"], []).append(p)
out = []; x0, y_cursor = 25.4, 50.8
placed = []
for gname in sorted(groups):
    parts = groups[gname]; x = x0; row_h = 0; y = y_cursor + 12.7
    out.append(("text", gname, x0, y_cursor))
    col = 0
    for part in parts:
        pm = resolve(part)
        ps = [e[1] for e in pm.values()]
        if ps:
            minx = min(q["x"] for q in ps); maxx = max(q["x"] for q in ps); miny = min(q["y"] for q in ps); maxy = max(q["y"] for q in ps)
        else: minx = maxx = miny = maxy = 0
        w = (maxx - minx) + 50.8; h = (maxy - miny) + 15.24
        if col >= 5:
            x = x0; y += row_h; row_h = 0; col = 0
        ox = round((x - minx + 25.4) / 2.54) * 2.54; oy = round((y + maxy + 7.62) / 2.54) * 2.54
        placed.append((part, pm, ox, oy)); x += w; row_h = max(row_h, h); col += 1
    y_cursor = y + row_h + 12.7

body = []; pwr_n = [0]
def wire(x1, y1, x2, y2, tag):
    body.append(f'(wire (pts (xy {f(x1)} {f(y1)}) (xy {f(x2)} {f(y2)})) (stroke (width 0) (type default)) (uuid "{U("w", tag)}"))')
def label(net, x, y, ang, tag):
    just = "left bottom" if ang in (0, 90) else "right bottom"
    body.append(f'(label "{net}" (at {f(x)} {f(y)} {ang}) (effects (font (size 1.27 1.27)) (justify {just})) (uuid "{U("l", tag)}"))')
def inst(libid, ref, val, fp, x, y, ang, tag, extra="", hide_ref=False, mpn=""):
    pl = get_sym(libid)[1]
    pins_txt = " ".join(f'(pin "{p["number"]}" (uuid "{U("pin", tag, p["number"])}"))' for p in pl)
    hide = " (hide yes)" if hide_ref else ""
    mp = f'(property "MPN" "{mpn}" (at {f(x)} {f(y)} 0) (effects (font (size 1.27 1.27)) (hide yes)))' if mpn else ""
    body.append(
        f'(symbol (lib_id "{libid}") (at {f(x)} {f(y)} {ang}) (unit 1) (exclude_from_sim no) (in_bom yes) (on_board yes) '
        f'(dnp {"yes" if "DNP" in extra else "no"}) (uuid "{U("s", tag)}")\n'
        f'  (property "Reference" "{ref}" (at {f(x)} {f(y-2.54)} 0) (effects (font (size 1.27 1.27)){hide}))\n'
        f'  (property "Value" "{val}" (at {f(x)} {f(y+2.54)} 0) (effects (font (size 1.27 1.27)){hide}))\n'
        f'  (property "Footprint" "{fp}" (at {f(x)} {f(y)} 0) (effects (font (size 1.27 1.27)) (hide yes)))\n'
        f'  (property "Datasheet" "" (at {f(x)} {f(y)} 0) (effects (font (size 1.27 1.27)) (hide yes)))\n  {mp}\n'
        f'  {pins_txt}\n'
        f'  (instances (project "pi-cemaker" (path "/{ROOT}" (reference "{ref}") (unit 1)))))')

pwr_ct = 0
for part, pm, ox, oy in placed:
    nl = {}
    inst(part["sym"], part["ref"], part["value"], part["fp"], ox, oy, 0, part["ref"], "DNP" if part["dnp"] else "", mpn=part["mpn"])
    for num, (net, p) in pm.items():
        px, py = ox + p["x"], oy - p["y"]
        out_ang = (p["ang"] + 180) % 360     # direction pointing away from body (lib angle y-up)
        dx = {0: 1, 90: 0, 180: -1, 270: 0}[out_ang]; dy = {0: 0, 90: -1, 180: 0, 270: 1}[out_ang]
        ex, ey = px + dx * 2.54, py + dy * 2.54
        if net == "NC":
            body.append(f'(no_connect (at {f(px)} {f(py)}) (uuid "{U("nc", part["ref"], num)}"))'); continue
        nl.setdefault(net, []).append(num)
        wire(px, py, ex, ey, f'{part["ref"]}.{num}')
        if net == "GND":
            pwr_ct += 1
            inst("power:GND", f"#PWR{pwr_ct:03d}", "GND", "", ex, ey, 0, f'gnd{pwr_ct}', hide_ref=True)
        else:
            lang = {0: 0, 90: 90, 180: 180, 270: 270}[out_ang]
            label(net, ex, ey, lang if lang != 270 else 90, f'{part["ref"]}.{num}')
    netlist[part["ref"]] = dict(value=part["value"], fp=part["fp"], dnp=part["dnp"], mpn=part["mpn"],
                                pads={n: e[0] for n, e in pm.items() if e[0] != "NC"})

# power flags (one per listed net, placed in a row under everything)
fy = y_cursor + 20.32; fx = x0
get_sym("power:PWR_FLAG")
for i, net in enumerate(design.POWER_FLAG_NETS):
    x = fx + i * 25.4
    pl = get_sym("power:PWR_FLAG")[1][0]
    # PWR_FLAG pin at (0,0): connect to net via wire stub to the right
    inst("power:PWR_FLAG", f"#FLG{i+1:02d}", "PWR_FLAG", "", x, fy, 0, f"flag{i}", hide_ref=True)
    if net == "GND":
        pwr_ct += 1
        inst("power:GND", f"#PWR{pwr_ct:03d}", "GND", "", x, fy, 0, f"gndflag", hide_ref=True)
    else:
        wire(x, fy, x + 5.08, fy, f"flag{i}")
        label(net, x + 5.08, fy, 0, f"flag{i}")
body.append(f'(text "Generated by hardware/kicad/gen/gen_sch.py from design.py - edit design.py, not this file. See hardware/DECISIONS.md." (at {f(x0)} 25.4 0) (effects (font (size 2 2)) (justify left)) (uuid "{U("hdr")}"))')
for kind, txt, tx, ty in [o for o in out if o[0] == "text"]:
    body.append(f'(text "{txt}" (at {f(tx)} {f(ty)} 0) (effects (font (size 3 3) bold) (justify left)) (uuid "{U("grp", txt)}"))')

if errors:
    print("\n".join(errors)); sys.exit(1)

# power:GND embedded too
get_sym("power:GND")
lib_txt = "\n".join(t for t, _ in libsyms.values())
sch = (f'(kicad_sch (version 20250114) (generator "pi-cemaker-gen") (generator_version "10.0") (uuid "{ROOT}") (paper "A0")\n'
       f'(title_block (title "Pi-cemaker Lite 2S UPS") (date "2026-10-01") (rev "0.2-draft") (company "Pi-cemaker") '
       f'(comment 1 "DRAFT - NOT REVIEWED - SEE hardware/DECISIONS.md BLOCKERS"))\n'
       f'(lib_symbols\n{lib_txt}\n)\n' + "\n".join(body) +
       f'\n(sheet_instances (path "/" (page "1")))\n(embedded_fonts no)\n)\n')
open(os.path.join(OUT, "pi-cemaker.kicad_sch"), "w").write(sch)
open(os.path.join(OUT, "pi-cemaker.kicad_sym"), "w").write(
    '(kicad_symbol_lib (version 20241209) (generator "pi-cemaker-gen")\n' + "\n".join(custom_txt.values()) + "\n)\n")
json.dump(netlist, open(os.path.join(os.path.dirname(__file__), "netlist.json"), "w"), indent=1, sort_keys=True)
print(f"{len(design.PARTS)} parts, {len(set(n for p in netlist.values() for n in p['pads'].values()))} nets")
