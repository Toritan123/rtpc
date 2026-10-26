#!/usr/bin/env python3
# Recreate the game's graphics, samples and sequences from the player's own ROM.
#
#   extract_assets.py [BASEROM]            write every file in asset_manifest.json
#   extract_assets.py --check              only verify the files already there
#   extract_assets.py --make-manifest      (maintainers) rebuild the manifest
#                                          from the files in the tree
#
# The repository carries no ROM data: tools/asset_manifest.json lists, for
# each file, where its bytes live in the ROM, how to decode them and the
# SHA-1 of the result. Every file is checked against that hash, so a wrong
# or modified ROM fails loudly instead of building something that looks
# almost right.
#
# Kinds:
#   raw   bytes stored as-is (sequences, .bin, .raw.4bpp)
#   wav   a signed 8-bit PCM sample, written as an unsigned 8-bit mono WAV
#         (its RIFF header is stored in the manifest)
#   rle   the game's run-length format: a halfword literal stream followed
#         by run lengths (tilemaps, .rle.4bpp)
#   gfx2  rle whose literal stream is itself packed by the nibble-dictionary
#         coder (struct CompressedGFX; the .4bpp tile graphics)
# Both coders are the inverse of tools/compression.py, which builds the ROM.
import hashlib, json, os, struct, sys, wave

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MANIFEST = os.path.join(ROOT, 'tools', 'asset_manifest.json')
DEFAULT_ROM = 'baserom.gba'
EXTS = ('.4bpp', '.bin', '.mid', '.wav', '.tilemap')


def u16(rom, off): return struct.unpack_from('<H', rom, off)[0]
def u32(rom, off): return struct.unpack_from('<I', rom, off)[0]


def rle_decode(lit, rom, seg, total):
    """lit: halfword literals; seg: offset of the run-length bytes."""
    out = []
    li = 0
    literal = True
    while len(out) < total:
        n = 0
        b = rom[seg]; seg += 1
        while b == 0:
            n += 255
            b = rom[seg]; seg += 1
        n += b
        if literal:
            out.extend(lit[li:li + n]); li += n
        else:
            out.extend([out[-1]] * n)
        literal = not literal
    return out[:total]


def gfx2_decode(rom, p):
    """Decode a struct CompressedGFX at ROM offset p into its halfwords."""
    data = u32(rom, p) - 0x08000000
    size, count = u16(rom, p + 4), u16(rom, p + 6)
    w1, w2 = u32(rom, p + 8) - 0x08000000, u32(rom, p + 12) - 0x08000000
    bit = lambda base, i: (u32(rom, base + (i >> 5) * 4) >> (i & 31)) & 1
    out = []
    di = data
    b2 = 0
    def hw():
        nonlocal di
        v = u16(rom, di); di += 2
        return v
    for s in range(count):
        if not bit(w1, s):
            out.append(hw())
            continue
        mode1 = bit(w2, b2); b2 += 1
        if not mode1:
            h = hw()
            n, nib = (h >> 8) + 1, (h & 0xF, (h >> 4) & 0xF)
            for _ in range(n):
                v = hw()
                for k in range(4):
                    o = 0
                    for j in range(4):
                        o |= nib[(v >> (k * 4 + j)) & 1] << (j * 4)
                    out.append(o)
        else:
            d = hw(); h = hw()
            nib = [(d >> (j * 4)) & 0xF for j in range(4)]
            dec = lambda byte: sum(nib[(byte >> (j * 2)) & 3] << (j * 4) for j in range(4))
            out.append(dec(h >> 8))
            for _ in range((h & 0xFF) + 1):
                v = hw()
                out.append(dec(v & 0xFF)); out.append(dec(v >> 8))
    return out[:size]


def build(e, rom):
    k = e['kind']
    if k == 'raw':
        return rom[e['off']:e['off'] + e['len']]
    if k == 'wav':
        pcm = bytes(b ^ 0x80 for b in rom[e['off']:e['off'] + e['len']])
        return bytes.fromhex(e['head']) + pcm + bytes.fromhex(e['tail'])
    if k == 'rle':
        lit = struct.unpack_from('<%dH' % e['nlit'], rom, e['off'])
        return struct.pack('<%dH' % e['total'], *rle_decode(lit, rom, e['seg'], e['total']))
    if k == 'gfx2':
        lit = gfx2_decode(rom, e['off'])
        return struct.pack('<%dH' % e['total'], *rle_decode(lit, rom, e['off'] + 16, e['total']))
    raise ValueError(k)


def make_manifest(rom):
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import subprocess, array
    files = subprocess.run(['git', 'ls-files'], cwd=ROOT, capture_output=True, text=True).stdout.split()
    offsets = json.load(open(os.path.join(ROOT, 'tools', 'compression_offsets.json')))
    import compression  # the encoder the GBA build uses
    man, bad = {}, []
    for f in sorted(files):
        if not f.endswith(EXTS) or '_rev1.' in f:
            continue  # _rev1 files belong to the other ROM revision (REV >= 1)
        data = open(os.path.join(ROOT, f), 'rb').read()
        e = None
        if f.endswith('.wav'):
            w = wave.open(os.path.join(ROOT, f))
            assert w.getnchannels() == 1 and w.getsampwidth() == 1, f
            pcm = bytes(b ^ 0x80 for b in w.readframes(w.getnframes()))
            off = rom.find(pcm)
            at = data.find(bytes(b ^ 0x80 for b in pcm))
            if off >= 0 and at >= 0:
                # The header (with its encoder tag) and any pad byte are kept
                # verbatim so the file is byte-identical.
                e = {'kind': 'wav', 'off': off, 'len': len(pcm),
                     'head': data[:at].hex(), 'tail': data[at + len(pcm):].hex()}
        if e is None and not f.endswith('.wav'):
            off = rom.find(data) if len(data) >= 8 else -1
            if off >= 0:
                e = {'kind': 'raw', 'off': off, 'len': len(data)}
        if e is None and f.endswith(('.4bpp', '.tilemap')):
            hw = array.array('H', data)
            rle = compression.RleCompressedData(hw)
            sym = os.path.basename(f).split('.')[0]
            double = not (f.endswith('.tilemap') or f.endswith('.rle.4bpp'))
            if double and sym in offsets:
                a = offsets[sym][0] + 0x08000000
                p = rom.find(struct.pack('<I', a))
                while p >= 0:
                    if p % 4 == 0 and rom[p + 16:p + 16 + len(rle.counts)] == bytes(rle.counts):
                        e = {'kind': 'gfx2', 'off': p, 'total': len(hw)}
                        break
                    p = rom.find(struct.pack('<I', a), p + 1)
            elif not double:
                blob = struct.pack('<%dH' % len(rle.compressed), *rle.compressed) + bytes(rle.counts)
                off = rom.find(blob)
                if off >= 0:
                    e = {'kind': 'rle', 'off': off, 'nlit': len(rle.compressed),
                         'seg': off + 2 * len(rle.compressed), 'total': len(hw)}
        if e is None:
            bad.append(f); continue
        e['sha1'] = hashlib.sha1(data).hexdigest()
        if hashlib.sha1(build(e, rom)).hexdigest() != e['sha1']:
            bad.append(f + ' (decodes differently)'); continue
        man[f] = e
    return man, bad


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    rom_path = args[0] if args else os.path.join(ROOT, DEFAULT_ROM)
    if '--make-manifest' in sys.argv:
        rom = open(rom_path, 'rb').read()
        man, bad = make_manifest(rom)
        json.dump({'rom_sha1': hashlib.sha1(rom).hexdigest(), 'files': man}, open(MANIFEST, 'w'),
                  indent=0, sort_keys=True)
        print('%d files in the manifest, %d not found' % (len(man), len(bad)))
        for b in bad: print('  not found:', b)
        sys.exit(1 if bad else 0)
    m = json.load(open(MANIFEST))
    sha = lambda b: hashlib.sha1(b).hexdigest()
    todo = [f for f, e in m['files'].items()
            if not os.path.exists(os.path.join(ROOT, f)) or sha(open(os.path.join(ROOT, f), 'rb').read()) != e['sha1']]
    if not todo:
        sys.exit(0)  # everything already extracted; the ROM is not needed
    if '--check' in sys.argv:
        for f in todo: print('differs or missing:', f)
        sys.exit(1)
    if not os.path.exists(rom_path):
        sys.exit('%d game files are missing. Put your own copy of the ROM at %s\n'
                 '(or pass BASEROM=<path> to make) and build again. sha1 must be %s'
                 % (len(todo), rom_path, m['rom_sha1']))
    rom = open(rom_path, 'rb').read()
    if sha(rom) != m['rom_sha1']:
        sys.exit('%s is not the expected ROM (sha1 %s, want %s)' % (rom_path, sha(rom), m['rom_sha1']))
    bad = 0
    for f in todo:
        data = build(m['files'][f], rom)
        if sha(data) != m['files'][f]['sha1']:
            print('decodes wrong:', f); bad += 1; continue
        path = os.path.join(ROOT, f)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        open(path, 'wb').write(data)
    print('extracted %d files from %s' % (len(todo) - bad, rom_path))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
