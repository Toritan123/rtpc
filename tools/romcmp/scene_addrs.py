#!/usr/bin/env python3
# ROM addresses of the game scenes, for tools/mgba_ref.c.
#
# level_data_table (data/scenes/game_select/levels.inc.c) sits at 0x089ce344
# in the ROM, 0x20 bytes per entry, and starts with the entry scene pointer.
# Pair each ROM entry with the C entry's &scene_xxx, in order.
#
#   scene_addrs.py [BASEROM]   -> prints "name 0xADDR" (name without scene_)
import os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROM_PATH = os.path.expanduser('~/Downloads/2462 - Rhythm Tengoku (J)(WRG).gba')
LEVEL_DATA_TABLE = 0x089ce344
ENTRY_SIZE = 0x20


def scene_addrs(rom_path=ROM_PATH):
    rom = open(rom_path, 'rb').read()
    src = open(os.path.join(ROOT, 'data/scenes/game_select/levels.inc.c'), encoding='latin-1').read()
    names = re.findall(r'/\*\s*Entry Scene\s*\*/\s*&?(\w+|NULL)', src)
    out = {}
    for i, name in enumerate(names):
        off = LEVEL_DATA_TABLE - 0x08000000 + i * ENTRY_SIZE
        addr = struct.unpack('<I', rom[off:off + 4])[0]
        if name.startswith('scene_') and addr:
            out[name[len('scene_'):]] = addr
    # The medal corner menus (toys, endless games, drum lessons) list their
    # scenes in struct MedalCornerLevel tables, 0x10 bytes per entry, at the
    # ROM address in the comment above each table.
    import glob
    for f in sorted(glob.glob(os.path.join(ROOT, 'data/scenes/medal_corner/*.inc.c'))):
        src = open(f, encoding='latin-1').read()
        for m in re.finditer(r'// \[D_(08[0-9a-fA-F]{6})\][^\n]*\nstruct MedalCornerLevel \w+\[\] = \{(.*?)\n\};', src, re.S):
            base = int(m.group(1), 16)
            for i, name in enumerate(re.findall(r'/\*\s*Scene\s*\*/\s*&?(\w+)', m.group(2))):
                off = base - 0x08000000 + i * 0x10
                addr = struct.unpack('<I', rom[off:off + 4])[0]
                if name.startswith('scene_') and addr:
                    out.setdefault(name[len('scene_'):], addr)
    return out


if __name__ == '__main__':
    for name, addr in scene_addrs(*sys.argv[1:]).items():
        print(name, hex(addr))
