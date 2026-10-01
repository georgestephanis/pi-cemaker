"""Minimal KiCad library helpers: extract a symbol from a .kicad_sym and list its pins."""
import re, os
SYMDIR = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols"
FPDIR = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"

def _block(text, start):
    """Return the balanced (...) block starting at index start (string-aware)."""
    depth = 0; i = start; instr = False
    while i < len(text):
        c = text[i]
        if instr:
            if c == '\\': i += 1
            elif c == '"': instr = False
        else:
            if c == '"': instr = True
            elif c == '(': depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0: return text[start:i+1]
        i += 1
    raise ValueError("unbalanced")

_cache = {}
def load_symbol(lib, name):
    """Return the symbol s-expression text (with its own name, un-prefixed)."""
    if lib not in _cache:
        _cache[lib] = open(os.path.join(SYMDIR, lib + ".kicad_sym")).read()
    t = _cache[lib]
    m = re.search(r'\(symbol "%s"' % re.escape(name), t)
    if not m: raise KeyError(f"{lib}:{name}")
    blk = _block(t, m.start())
    ext = re.search(r'\(extends "([^"]+)"', blk)
    if ext:  # resolve inheritance by merging base symbol graphics under the new name
        base = load_symbol(lib, ext.group(1))
        # rename base to derived name; keep derived properties
        base = base.replace('(symbol "%s"' % ext.group(1), '(symbol "%s"' % name, 1)
        base = re.sub(r'\(symbol "%s_(\d+_\d+)"' % re.escape(ext.group(1)), r'(symbol "%s_\1"' % name, base)
        return base
    return blk

PIN_RE = re.compile(r'\(pin (\w+) (\w+)\s+\(at ([-\d.]+) ([-\d.]+) (\d+)\)\s+\(length ([-\d.]+)\)(.*?)\(name "([^"]*)".*?\(number "([^"]*)"', re.S)
def pins(symtext):
    out = []
    for m in PIN_RE.finditer(symtext):
        out.append(dict(etype=m.group(1), x=float(m.group(3)), y=float(m.group(4)), ang=int(m.group(5)),
                        length=float(m.group(6)), name=m.group(8), number=m.group(9)))
    return out
