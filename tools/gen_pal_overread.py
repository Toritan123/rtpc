#!/usr/bin/env python3
# Append, for PC builds, the ROM bytes the game reads past each palette.
#
# Most graphics tables copy a fixed 0x140 or 0x200 bytes of palette, while the
# palette itself defines fewer banks. On the GBA the rest comes from whatever
# follows in ROM (often cel data), and some games show it: Wizard's Waltz's
# title text uses OBJ bank 4 colour 15 of its prologue palette, which defines
# only 3 banks, so the ROM draws it brown (0x81d4 from the following bytes)
# where the PC read a host pointer past the C array and drew it black. Code
# reads past palettes too: Bon Odori fades 7 banks from bon_odori_bg_pal (6
# defined), and its lyric highlight takes its colour from the 7th.
#
# For every palette that some GraphicsTable entry copies past its end, this
# checks that the defined colours match the base ROM at the address in the
# palette's comment, and appends the ROM banks up to the largest copy inside
# an #ifdef PLATFORM_PC block at the end of the palette's initialiser, so every
# read of the palette on PC sees what the GBA sees. Re-running it replaces the
# blocks it wrote before.
#
#   tools/gen_pal_overread.py [BASEROM]
import glob, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROM_PATH = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser(
    '~/Downloads/2462 - Rhythm Tengoku (J)(WRG).gba')
BEGIN = '#ifdef PLATFORM_PC // ROM bytes past this palette (tools/gen_pal_overread.py)\n'
END = '#endif // PLATFORM_PC\n'

rom = open(ROM_PATH, 'rb').read()
files = sorted(f for f in glob.glob(ROOT + '/**/*.c', recursive=True)
               if not os.path.relpath(f, ROOT).startswith(('build', 'dist')))
src = {f: open(f, encoding='latin-1').read() for f in files}

# Drop the blocks a previous run wrote, so the defined part is what is parsed.
block = re.compile(re.escape(BEGIN) + r'.*?' + re.escape(END), re.S)
for f in files:
    src[f] = block.sub('', src[f])


def rgb555(v):
    return ((v >> 16 & 0xFF) >> 3) | ((v >> 8 & 0xFF) >> 3) << 5 | ((v & 0xFF) >> 3) << 10


def initialiser_end(s, start):
    """Index of the '}' closing the initialiser whose '{' ends at start."""
    depth = 1; i = start
    while depth:
        depth += {'{': 1, '}': -1}.get(s[i], 0)
        i += 1
    return i - 1


# name -> (rom address, [u16 values], file)
pals = {}
for f in files:
    s = src[f]
    for m in re.finditer(r'(//[^\n]*\n)?[ \t]*(?:const\s+)?Palette\s+(\w+)\[\]\s*=\s*\{', s):
        a = re.search(r'\[(?:D_)?(?:0x)?(0?8[0-9a-fA-F]{6,7})\]', m.group(1) or '')
        body = re.sub(r'/\*.*?\*/', '', s[m.end():initialiser_end(s, m.end())], flags=re.S)
        vals = [rgb555(int(x, 16)) for x in re.findall(r'TO_RGB555\((0x[0-9A-Fa-f]+)\)', body)]
        if not vals:
            vals = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', body)]
        pals[m.group(2)] = (int(a.group(1), 16) if a else None, vals, f)

# name -> largest copy size
sizes = {}
for f in files:
    for m in re.finditer(r'/\*\s*Src\.\s*\*/\s*&?(\w+),\s*/\*\s*Dest\.\s*\*/\s*[^,]*PALETTE[^,]*,'
                         r'\s*/\*\s*Size\s*\*/\s*(0x[0-9A-Fa-f]+|\d+)', src[f]):
        name, size = m.group(1), int(m.group(2), 0)
        if name in pals and size > 2 * len(pals[name][1]):
            sizes[name] = max(size, sizes.get(name, 0))

total = 0
for name in sorted(sizes):
    addr, vals, f = pals[name]
    if addr is None:
        sys.exit('%s (%s): no ROM address comment' % (name, f))
    if len(vals) % 16 or sizes[name] % 32:
        sys.exit('%s: not whole banks' % name)
    off = addr - 0x08000000
    words = struct.unpack('<%dH' % (sizes[name] // 2), rom[off:off + sizes[name]])
    if list(words[:len(vals)]) != vals:
        sys.exit('%s: defined colours do not match the ROM at %08x' % (name, addr))
    tail = words[len(vals):]
    total += len(tail) * 2

    s = src[f]
    m = re.search(r'(?:const\s+)?Palette\s+' + name + r'\[\]\s*=\s*\{', s)
    close = initialiser_end(s, m.end())
    head = s[:close].rstrip()
    if not head.endswith(','):
        head += ','
    lines = [BEGIN]
    for b in range(0, len(tail), 16):
        lines.append('    /* PALETTE %02d */ { %s },\n' % ((len(vals) + b) // 16, ', '.join('0x%04x' % w for w in tail[b:b + 16])))
    lines.append(END)
    src[f] = head + '\n' + ''.join(lines) + s[close:]

for f in files:
    if src[f] != open(f, encoding='latin-1').read():
        open(f, 'w', encoding='latin-1').write(src[f])
print('%d palettes, %d bytes past their ends' % (len(sizes), total))
