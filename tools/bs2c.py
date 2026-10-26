#!/usr/bin/env python3
"""
bs2c — translate GBA beatscript assembly (.bs) into C for the PC build.

The .bs files are only assembled by the GBA build (Makefile: $(AS) on *.bs).
The PC build has no assembler step, which is why every scene had to be
hand-written as a stub in platform/game_globals.c.

Every .bs macro — however high-level — expands to exactly one primitive:

    beatscript_cmd <op>, <param1>, <param2>, <param3>
        .word (op & 0xff) | (param1 << 8)
        .word param2
        .word param3

which is field-for-field the PC `struct Beatscript`
{command:8, param1:24, const void *param2, uintptr_t param3}.  So the
translation is 1:1; this tool just expands the macros and prints C.

Macro definitions are read from the .inc files at run time rather than being
hardcoded, so the tool stays correct if the macro set changes.

Usage:  tools/bs2c.py <file.bs> [-o out.c]
        tools/bs2c.py --all            (translate every .bs in the tree)
"""

import argparse
import glob
import os
import re
import sys

INC_FILES = ["include/beatscript_main.inc", "include/beatscript.inc"]

# Macros we interpret ourselves instead of expanding.
STRUCTURAL = {"script", "struct", "endstruct", "text", "endtext", "beatscript_cmd"}


def strip_comments(line):
    line = re.sub(r"/\*.*?\*/", " ", line)      # C-style, used inside macro bodies
    # asm line comment -- but not an '@' inside a string: Shift-JIS puts 0x40
    # in the trail byte of many characters (the full-width space is 81 40),
    # and cutting there silently dropped whole text blocks.
    out, quoted = [], False
    for i, ch in enumerate(line):
        if ch == '"' and (i == 0 or line[i - 1] != "\\"):
            quoted = not quoted
        elif ch == "@" and not quoted:
            break
        out.append(ch)
    return "".join(out).strip()


def split_args(s):
    """Split on commas that are not inside parentheses."""
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


OPERATOR_CHARS = "+-*/%|&^<>~"


def split_macro_args(s):
    """Macro invocation args: separated by commas *or* whitespace (GNU as),
    but never inside parentheses, and not by whitespace next to an operator:
    GNU as keeps `table + (n * 0xC)` together as one argument. Splitting it
    there silently dropped the offset (drum_intro's dmi_event0B)."""
    out, depth, cur = [], 0, ""
    for i, ch in enumerate(s):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if depth == 0 and ch.isspace():
            prev = cur.rstrip()[-1:]
            nxt = s[i:].lstrip()[:1]
            if (prev and prev in OPERATOR_CHARS) or (nxt and nxt in OPERATOR_CHARS):
                cur += ch
                continue
        if depth == 0 and (ch == "," or ch.isspace()):
            if cur.strip():
                out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


index_objects = set()


def build_symbol_index(root):
    """identifier -> header that declares it.

    The generated files reference game-specific symbols (engine functions,
    animations, sound banks).  Rather than hardcoding a path mapping, scan the
    headers once and include exactly what each file needs.
    """
    index = {}
    # Symbols whose declaration is a plain object — not an array, not a
    # function, not already a pointer.  Using one as an address needs an
    # explicit &, where an array or function name decays by itself.  Deciding
    # this from the header beats guessing from the name, which is what the
    # earlier *_seqData / scene_* special cases were doing.
    objects = index_objects
    pats = [
        # extern <type> name(...)   /  extern <type> *name;  /  extern <type> name[];
        re.compile(r"^\s*extern\s+[^;()]*?\b(\w+)\s*\("),
        re.compile(r"^\s*extern\s+[^;()]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*;"),
        # struct/array definitions in headers
        re.compile(r"^\s*(?:const\s+)?struct\s+\w+\s+\*?(\w+)\s*(?:\[[^\]]*\])?\s*;"),
    ]
    for base in ("include", "src", "data", "games"):
        for dirpath, _, files in os.walk(os.path.join(root, base)):
            for fn in files:
                if not fn.endswith(".h"):
                    continue
                p = os.path.join(dirpath, fn)
                rel = os.path.relpath(p, root)
                try:
                    txt = open(p, errors="replace").read()
                except OSError:
                    continue
                for line in txt.split("\n"):
                    for i, pat in enumerate(pats):
                        m = pat.match(line)
                        if m:
                            index.setdefault(m.group(1), rel)
                            decays = (i == 0                      # function
                                      or "[" in line              # array
                                      or re.search(r"\*\s*" + re.escape(m.group(1)), line))
                            if not decays:
                                objects.add(m.group(1))
                            break
    return index


def collect_includes(path, root, seen=None):
    """Follow the .include chain of a source file, exactly as the assembler
    would, so game-specific macro files are picked up without a hardcoded map."""
    if seen is None:
        seen = []
    for raw in open(path, errors="replace"):
        m = re.match(r'\s*\.include\s+"([^"]+)"', raw)
        if not m:
            continue
        inc = m.group(1)
        if inc in seen:
            continue
        seen.append(inc)
        full = os.path.join(root, inc)
        if os.path.exists(full):
            collect_includes(full, root, seen)
    return seen


def load_macros(root, includes=None):
    """name -> (param_names, defaults, body_lines)"""
    macros = {}
    for inc in (includes if includes is not None else INC_FILES):
        path = os.path.join(root, inc)
        if not os.path.exists(path):
            continue
        txt = open(path, errors="replace").read()
        for m in re.finditer(r"^\.macro\s+(\S+)([^\n]*)\n(.*?)^\.endm", txt, re.S | re.M):
            name = m.group(1)
            params, defaults = [], {}
            for p in split_args(m.group(2).strip()):
                if "=" in p:
                    k, v = p.split("=", 1)
                    params.append(k.strip())
                    defaults[k.strip()] = v.strip()
                elif p:
                    params.append(p.strip())
            body = [strip_comments(l) for l in m.group(3).split("\n")]
            macros[name] = (params, defaults, [l for l in body if l])
    return macros


def expand(line, macros, depth=0):
    """Expand a source line into a list of terminal lines."""
    if depth > 24:
        raise RuntimeError(f"macro recursion too deep: {line}")
    line = strip_comments(line)
    if not line:
        return []

    head = line.split(None, 1)
    name = head[0]
    rest = head[1] if len(head) > 1 else ""

    if name in STRUCTURAL or name not in macros:
        return [line]

    params, defaults, body = macros[name]
    args = split_macro_args(rest)
    binding = {}
    for i, p in enumerate(params):
        binding[p] = args[i] if i < len(args) and args[i] != "" else defaults.get(p, "0")

    out = []
    for bl in body:
        sub = bl
        # longest names first so \arg1 isn't clobbered by \arg
        for p in sorted(binding, key=len, reverse=True):
            sub = sub.replace("\\" + p, binding[p])
        for piece in sub.split(";"):
            out.extend(expand(piece, macros, depth + 1))
    return out


class Emitter:
    """Collects labelled blocks and renders them as C definitions."""

    def __init__(self, path=""):
        self.path = path
        self.scene_var_headers = set()
        self.blocks = []      # (kind, name, items)
        self.kind = None
        self.name = None
        self.items = []
        self.warnings = []

    def flush(self):
        if self.kind and self.items:
            self.blocks.append((self.kind, self.name, self.items))
        self.kind, self.name, self.items = None, None, []

    def start(self, kind, name):
        self.flush()
        self.kind, self.name, self.items = kind, name, []

    def add(self, item):
        if self.kind:
            self.items.append(item)

    # ---- rendering -------------------------------------------------------

    @staticmethod
    def c_bytes(lit):
        """An assembler string body (read as latin-1) as a C literal body.
        Escapes pass through; bytes >= 0x80 (Shift-JIS) become octal escapes,
        which cannot swallow a following character the way \\x can."""
        out = []
        for ch in lit:
            o = ord(ch)
            out.append(ch if o < 0x80 else "\\%03o" % o)
        return "".join(out)

    @staticmethod
    def is_data(items):
        return bool(items) and all(isinstance(it, tuple) and len(it) == 2
                                   and it[0] in ("b", "h") for it in items)

    def render_data(self, name, items):
        """Raw .byte/.hword data, little-endian, as the GBA laid it out."""
        out = []
        for kind, v in items:
            if kind == "b":
                out.append(f"(u8)({v})")
            else:
                out.append(f"(u8)(({v}) & 0xff), (u8)((({v}) >> 8) & 0xff)")
        return (f"const u8 {name}[] __attribute__((aligned(4))) = {{\n    "
                + ",\n    ".join(out) + "\n};")

    def render_text(self, name, items):
        parts = []
        for kind, terminated, lit in items:
            parts.append(self.c_bytes(lit) + ("\\0" if terminated else ""))
        body = "".join(parts)
        # C adds the final terminator itself.
        if body.endswith("\\0"):
            body = body[:-2]
        return f'const char {name}[] = "{body}";'

    OBJECT_SYMBOL = re.compile(r"^[A-Za-z_]\w*$")

    def obj_ref(self, tok):
        """A struct field that stores the address of another object
        (e.g. Scene.initParam -> the SubScene).  Arrays decay by themselves;
        plain structs need an explicit &."""
        t = tok.strip()
        if t in ("0", "NULL") or t in getattr(self, "consts", ()):
            return "NULL"
        if re.match(r"^[A-Za-z_]\w*$", t):
            return f"&{t}"
        return t

    def int_or_zero(self, tok):
        """SubScene's callback parameters are s32, so NULL is a type error."""
        t = tok.strip()
        return "0" if t in ("NULL", "0") else f"(s32)({self.ref(t)})"

    def classify_local(self):
        """Register the blocks this file defines itself.

        index_objects only knows what the headers declare, so a Scene or
        SubScene defined in the same .bs was missing its & — 25 of the 49
        remaining failures.  Classify them the same way render_struct does."""
        self.local_objects = set()
        self.local_ptr_tables = set()   # rendered as arrays of pointers
        for kind, name, items in self.blocks:
            if kind != "struct":
                continue          # scripts and text render as arrays: they decay
            words  = [v for k, v in items if k == "w"]
            hwords = [v for k, v in items if k == "h"]
            single = (len(words) == 9 and not hwords) or \
                     ((len(words) == 6 and len(hwords) == 1) or
                      (len(words) == 7 and not hwords)) or \
                     (len(words) == 2 and len(hwords) == 4)
            if single:
                self.local_objects.add(name)
            elif words and not hwords:
                self.local_ptr_tables.add(name)

    def ref(self, tok):
        """Render an operand that the assembler would have stored as an address."""
        t = tok.strip()
        # `table + byteOffset` into a table of .words: the offset counts
        # 4-byte GBA pointers. Here the elements are host pointers, so turn
        # it into an element index.
        m = re.match(r"^([A-Za-z_]\w*)\s*\+\s*(.+)$", t)
        if m and m.group(1) in getattr(self, "local_ptr_tables", ()):
            return f"({m.group(1)} + ({m.group(2)}) / 4)"
        if not re.match(r"^[A-Za-z_]\w*$", t):
            return t
        # A plain object needs its address taken; arrays and functions decay.
        # Header declarations feed index_objects; blocks defined in this same
        # file are classified by classify_local().
        if t in getattr(self, "local_objects", ()) or t in index_objects:
            return f"&{t}"
        return t

    def render_script(self, name, cmds):
        out = [f"const struct Beatscript {name}[] = {{"]
        for op, p1, p2, p3 in cmds:
            opcode = op.strip()
            if opcode == "0x09":
                sym = scene_var_offset(self.path, p2)
            elif opcode in ("0x21", "0x22"):
                sym = scene_var_offset(self.path, p2, SCENE_COND_FIELDS)
            else:
                sym = None
            if sym is not None:
                expr, header = sym
                self.scene_var_headers.add(header)
                p2 = f"(const void *)({expr})"
            else:
                p2 = "NULL" if p2.strip() in ("0", "NULL") else f"(const void *)({self.ref(p2)})"
            out.append(f"    {{ {op}, {p1}, {p2}, (uintptr_t)({self.ref(p3)}) }},")
        out.append("};")
        return "\n".join(out)

    def render_struct(self, name, items):
        words  = [v for k, v in items if k == "w"]
        hwords = [v for k, v in items if k == "h"]

        # SubScene: 4 (func, param) pairs + script pointer
        if len(words) == 9 and not hwords:
            f = words
            return (f"const struct SubScene {name} = {{\n"
                    f"    /* start  */ (void (*)()){f[0]}, {self.int_or_zero(f[1])},\n"
                    f"    /* paused */ (void (*)()){f[2]}, {self.int_or_zero(f[3])},\n"
                    f"    /* update */ (void (*)()){f[4]}, {self.int_or_zero(f[5])},\n"
                    f"    /* stop   */ (void (*)()){f[6]}, {self.int_or_zero(f[7])},\n"
                    f"    /* script */ {f[8]},\n"
                    f"}};")

        # Scene: 3 (func, param) pairs + required memory.
        # The memory field is written as .hword by define_gameplay_scene and as
        # .word by the hand-written scene structs.
        if (len(words) == 6 and len(hwords) == 1) or (len(words) == 7 and not hwords):
            f = words
            mem = hwords[0] if hwords else words[6]
            return (f"struct Scene {name} = {{\n"
                    f"    /* init   */ (void (*)()){f[0]}, (void *)({self.obj_ref(f[1])}),\n"
                    f"    /* loop   */ (u32 (*)()){f[2]}, (void *)({f[3]}),\n"
                    f"    /* end    */ (void (*)()){f[4]}, (void *)({f[5]}),\n"
                    f"    /* memory */ {mem},\n"
                    f"}};")

        # MarkingCriteria: 2 remark pointers + a packed flags hword + 3 thresholds.
        # struct MarkingCriteria splits that first hword into flags:u8 and
        # checkAverageMisses:u8.
        if len(words) == 2 and len(hwords) == 4:
            return (f"const struct MarkingCriteria {name} = {{\n"
                    f"    /* positive */ {words[0]},\n"
                    f"    /* negative */ {words[1]},\n"
                    f"    /* flags    */ ({hwords[0]}) & 0xFF, (({hwords[0]}) >> 8) & 0xFF,\n"
                    f"    /* minHitsForSuccess   */ {hwords[1]},\n"
                    f"    /* minHitsBeforeFail   */ {hwords[2]},\n"
                    f"    /* maxMissesBeforeFail */ {hwords[3]},\n"
                    f"}};")

        # A NULL/END_OF_CRITERIA-terminated table of MarkingCriteria pointers —
        # this is what import_marking_criteria receives.
        if words and not hwords and words[-1].strip() in ("END_OF_CRITERIA", "NULL", "0"):
            body = "".join(f"    (const struct MarkingCriteria *)({self.obj_ref(w)}),\n" for w in words)
            return f"const struct MarkingCriteria *{name}[] = {{\n{body}}};"

        # The same table without its terminator. Only rat_race's has this: the
        # ROM genuinely omits it (the .bs marks it "@! No criteria terminator"),
        # and on the GBA whatever follows in ROM ends the walk harmlessly. Here
        # the neighbouring object is arbitrary, results_get_negative_comments
        # reads past the end, dereferences it as a MarkingCriteria and crashes
        # in strcat. The .bs.c is PC-only -- the GBA build assembles the .bs --
        # so terminating it here cannot disturb the matching ROM.
        if (words and not hwords
                and all("marking_criteria" in w for w in words)):
            body = "".join(f"    (const struct MarkingCriteria *)({self.obj_ref(w)}),\n" for w in words)
            return (f"const struct MarkingCriteria *{name}[] = {{\n{body}"
                    f"    END_OF_CRITERIA, // missing in the ROM; see bs2c.py\n}};")

        # Anything else is a plain data table local to this file — grade
        # thresholds, comment lists, text tables.  No header declares them, so
        # the only consumer is this file taking their address, and emitting
        # them with matching element widths is enough.  Skipping them instead
        # left dangling references that broke the link.
        if words and not hwords:
            body = "".join(f"    (const void *)({self.ref(w)}),\n" for w in words)
            return f"const void *{name}[] = {{\n{body}}};"
        if hwords and not words:
            body = ", ".join(hwords)
            return f"const u16 {name}[] = {{ {body} }};"

        self.warnings.append(f"{name}: unrecognised struct shape "
                             f"({len(words)} words, {len(hwords)} hwords) — skipped")
        return None

    def referenced(self):
        """Every identifier appearing in the rendered operands."""
        names = set()
        for kind, name, items in self.blocks:
            if kind == "text":
                continue    # string contents, not symbol references
            for it in items:
                # A struct item is (type tag, value); the tag is not a symbol.
                toks = it[1:] if isinstance(it, tuple) and it and it[0] in ("w", "h", "b") else \
                       (it if isinstance(it, tuple) else (it,))
                for tok in toks:
                    # The lookbehind keeps the tail of a number (the xC in 0xC,
                    # also after a macro pasted it next to other digits) out.
                    for w in re.findall(r"(?<![\w.])[A-Za-z_]\w*", str(tok)):
                        names.add(w)
        return names

    def render(self, src, index=None):
        import re as _re
        self.classify_local()
        extra = []
        if index:
            hdrs = {index[n] for n in self.referenced() if n in index}
            base = {"include/global.h", "src/main.h", "src/audio.h",
                    "src/scenes/gameplay.h", "src/scenes/results.h",
                    "src/code_0800b778.h"}
            extra = sorted(h for h in hdrs if h not in base)
        # Symbols that no header declares (engine tables, remark strings, …).
        # They are only ever used as addresses here, so an opaque array
        # declaration is enough; anything genuinely missing shows up as a link
        # error, which is exactly the list worth working through.
        defined = {nm for _, nm, _ in self.blocks}
        consts = getattr(self, "consts", set())
        unknown = sorted(n for n in self.referenced()
                         if index is not None and n not in index and n not in defined
                         and n not in consts and not n[0].isdigit()
                         and n not in ("NULL", "TRUE", "FALSE", "uintptr_t", "void",
                                       "const", "struct", "u32", "u16", "s32"))
        fwd = [f"extern char {n}[];   /* undeclared in any header */" for n in unknown]

        # Scripts may `call`/`goto` labels defined later in the same file.
        for kind, nm, items in self.blocks:
            if kind == "text":
                fwd.append(f"extern const char {nm}[];")
            if kind == "script" and self.is_data(items):
                fwd.append(f"extern const u8 {nm}[];")
            elif kind == "script":
                fwd.append(f"extern const struct Beatscript {nm}[];")
            elif kind == "struct":
                w = [v for k, v in items if k == "w"]
                h = [v for k, v in items if k == "h"]
                if len(w) == 9 and not h:
                    fwd.append(f"extern const struct SubScene {nm};")
                elif (len(w) == 6 and len(h) == 1) or (len(w) == 7 and not h):
                    fwd.append(f"extern struct Scene {nm};")
                elif len(w) == 2 and len(h) == 4:
                    fwd.append(f"extern const struct MarkingCriteria {nm};")
                elif w and not h and w[-1].strip() in ("END_OF_CRITERIA", "NULL", "0"):
                    fwd.append(f"extern const struct MarkingCriteria *{nm}[];")
                elif w and not h and all("marking_criteria" in x for x in w):
                    fwd.append(f"extern const struct MarkingCriteria *{nm}[];")
                elif w and not h:
                    fwd.append(f"extern const void *{nm}[];")
                elif h and not w:
                    fwd.append(f"extern const u16 {nm}[];")
        # Render the bodies first: render_script() is what discovers which
        # scene headers the offsetof() expressions need.
        body = []
        for kind, name, items in self.blocks:
            if kind == "script" and self.is_data(items):
                body.append(self.render_data(name, items))
            elif kind == "script":
                body.append(self.render_script(name, items))
            elif kind == "struct":
                s = self.render_struct(name, items)
                if s:
                    body.append(s)
            elif kind == "text":
                body.append(self.render_text(name, items))
            body.append("")

        parts = [
            "/* Generated by tools/bs2c.py from %s — do not edit. */" % src,
            '#include "global.h"',
            '#include "src/main.h"',
            '#include "src/audio.h"',
            '#include "src/scenes/gameplay.h"',
            '#include "src/scenes/results.h"',
            '#include "src/code_0800b778.h"',
            '#include "include/beatscript_consts.h"',
        ] + [f'#include "{h}"' for h in extra] + (
            ["#include <stddef.h>   // offsetof, for the scene-variable offsets below"]
            + [f'#include "{h}"' for h in sorted(self.scene_var_headers)]
            if self.scene_var_headers else []
        ) + [""] + fwd + [""] + body
        return "\n".join(parts)


KNOWN_DIRECTIVES = (".include", ".section", ".end", ".balign", ".align",
                    ".set", ".equ", ".global", ".globl", ".text", ".data",
                    ".byte", ".2byte", ".4byte", ".ltorg", ".syntax", ".arm",
                    ".thumb", ".pool", "glabel")


def translate(path, macros, root, index=None, consts=None):
    em = Emitter(path)
    em.consts = consts or set()
    # Macros come from this file's own .include chain (plus the always-present
    # core set), so a game's macros.inc is honoured without any hardcoding.
    incs = collect_includes(path, root)
    if incs:
        macros = load_macros(root, INC_FILES + [i for i in incs if i not in INC_FILES])
    # A .bs may define its own macros (quiz_show_endless.bs does).
    own = load_macros(root, [os.path.relpath(path, root)])
    if own:
        macros = dict(macros)
        macros.update(own)
    in_macro_def = False
    # Labels the game's load_* macros .set (EVENT_05, PRINT_TEXT, CUE_*...).
    # They are per game and reuse the same names with different values, so
    # they cannot be global C #defines: beatscript_consts.h keeps the first
    # one it saw, and staff_credit's PRINT_TEXT (0) became another game's 5,
    # an engine event past the end of its table. Resolve them here, in source
    # order, exactly as the assembler does.
    labels = {}

    def resolve(v):
        if not labels:
            return v
        return re.sub(r"\b[A-Za-z_]\w*\b",
                      lambda m: f"({labels[m.group(0)]})" if m.group(0) in labels else m.group(0), v)

    # latin-1 keeps every byte: the text blocks hold Shift-JIS strings that
    # must reach the C output unchanged.
    # The .bs sources are run through the C preprocessor by the GBA build;
    # the only conditionals in them are on REV (the ROM revision). The base
    # ROM this port is checked against is REV 0 (BASEROM_SHA1 in Makefile).
    rev = int(os.environ.get("REV", "0"))
    cond = []   # stack of "this branch is live"
    for raw in open(path, encoding="latin-1"):
        pp = raw.strip()
        if pp.startswith("#if "):
            expr = pp[4:].strip()
            if not re.fullmatch(r"REV\s*(<|<=|>|>=|==|!=)\s*\d+", expr):
                raise SystemExit(f"{path}: unsupported conditional '{pp}'")
            live = eval(expr, {}, {"REV": rev})
            cond.append(live and all(cond))
            continue
        if pp == "#else":
            outer = all(cond[:-1])
            cond[-1] = outer and not cond[-1]
            continue
        if pp == "#endif":
            cond.pop()
            continue
        if cond and not cond[-1]:
            continue
        # Skip the bodies of macros defined in this file; they were loaded above.
        word = strip_comments(raw).split(None, 1)
        if word and word[0] == ".macro":
            in_macro_def = True
        if in_macro_def:
            if word and word[0] == ".endm":
                in_macro_def = False
            continue
        for line in expand(raw, macros):
            tok = line.split(None, 1)
            head = tok[0]
            rest = tok[1].strip() if len(tok) > 1 else ""

            if head in (".include", ".section", ".end", ".balign", ".align", "glabel"):
                continue
            if head in (".set", ".equ"):
                a = split_args(rest)
                if len(a) == 2 and re.match(r"^[A-Za-z_]\w*$", a[0]):
                    labels[a[0]] = resolve(a[1])
                continue
            rest = resolve(rest)
            if head == "script":
                em.start("script", rest)
            elif head == "struct":
                em.start("struct", rest)
            elif head == "text":
                em.start("text", rest)
            elif head in ("endstruct", "endtext"):
                em.flush()
            elif head == "beatscript_cmd":
                a = split_args(rest)
                if len(a) == 4 and em.kind == "script":
                    em.add(tuple(a))
            elif head == ".word" and em.kind == "struct":
                for v in split_args(rest):
                    em.add(("w", v))
            elif head == ".hword" and em.kind == "struct":
                for v in split_args(rest):
                    em.add(("h", v))
            elif head in (".ascii", ".asciz", ".string") and em.kind == "text":
                # Text blocks define strings that exist nowhere else (the
                # games/*/*_text.c tables hold other, D_-named strings), so
                # emit them. .asciz/.string add a terminator, .ascii does not.
                for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', rest):
                    em.add(("s", head != ".ascii", lit))
            elif head in (".byte", ".hword") and em.kind == "script":
                # Raw data under a script label: Sick Beats' virus paths are
                # 4-byte {u8 command, u8 arg, u16 rest} records written this way.
                for v in split_args(rest):
                    em.add(("b" if head == ".byte" else "h", v))
            elif head.endswith(":") or head in KNOWN_DIRECTIVES:
                pass
            elif head in (".word", ".hword"):
                pass  # outside a struct: nothing to emit
            else:
                # An unexpanded mnemonic means a macro we never loaded.  Left
                # silent this drops a whole command from the script (this is
                # how load_karate_man once vanished), so make it loud.
                em.warnings.append(f"unexpanded line dropped: {line!r}")
    em.flush()
    return em.render(os.path.relpath(path, root), index), em



# ─── Scene-variable offsets ──────────────────────────────────────────────────
#
# The scene-relative opcodes -- 0x09 scene_set_var, 0x21 scene_if_eq and
# 0x22 scene_if_neq -- address the current scene's data struct by a raw byte
# offset baked into the .bs source. Those offsets were computed for the
# GBA, where a pointer is 4 bytes. On a 64-bit host every pointer ahead of the
# target field shifts it, so e.g. ResultsSceneData::inputsEnabled moves from 8
# to 16 and the write lands in the middle of a pointer instead — the results
# screen then never enables its input and the game stops there.
#
# Emitting offsetof() instead is correct on both: it is still 8 on the GBA.
# Each entry below was checked against the struct in src/scenes/.
SCENE_VAR_FIELDS = {
    "scenes/results/score_scene.bs":   {8: ("ResultsSceneData",         "inputsEnabled", "src/scenes/results.h")},
    "scenes/results/rank_scene.bs":    {8: ("ResultsSceneData",         "inputsEnabled", "src/scenes/results.h")},
    "scenes/results/debug_scene.bs":   {8: ("ResultsSceneData",         "inputsEnabled", "src/scenes/results.h")},
    "scenes/main_menu/scene.bs":       {8: ("MainMenuSceneData",        "inputsEnabled", "src/scenes/main_menu.h")},
    "scenes/epilogue/scene.bs":        {8: ("EpilogueSceneData",        "inputsEnabled", "src/scenes/epilogue.h")},
    "scenes/debug/template_scene.bs":  {8: ("TemplateSceneData",        "inputsEnabled", "src/scenes/debug_template_scene.h")},
    "scenes/debug/flash_mem_test.bs":  {8: ("FlashMemoryTestSceneData", "inputsEnabled", "src/scenes/debug_flash_mem.h")},
    "scenes/debug/data_check.bs":      {8: ("DataCheckSceneData",       "inputsEnabled", "src/scenes/data_check.h")},
    "scenes/debug/menu.bs":            {8: ("DebugMenuSceneData",       "inputsEnabled", "src/scenes/debug_menu.h")},
    "scenes/debug/asset_test.bs":      {8: ("AssetTestSceneData",       "inputsEnabled", "src/scenes/debug_asset_test.h")},
    "scenes/perfect/scene.bs":         {4: ("PerfectSceneData",         "inputsEnabled", "src/scenes/perfect.h")},
    "scenes/data_room/reading.bs":     {4: ("ReadingSceneData",         "inputsEnabled", "src/scenes/reading.h")},
    # 0x358 on the GBA: StudioSceneData embeds a DrumTechController (with
    # pointers) ahead of inputsEnabled.
    # inputsEnabled follows a u8, at 4 on both; written symbolically anyway.
    "scenes/game_select/scene.bs":     {4: ("GameSelectSceneData",  "inputsEnabled", "src/scenes/game_select.h")},
    "scenes/studio/scene.bs":          {0x358: ("StudioSceneData",      "inputsEnabled", "src/scenes/studio.h")},
    # No pointer precedes inputsEnabled here, so 0x10 is already right on both;
    # written symbolically anyway so it cannot rot.
    "scenes/options/scene.bs":         {0x10: ("OptionsSceneData",      "inputsEnabled", "src/scenes/options.h")},
    # Offset 0 is the first field, which needs no translation:
    # warning, read_error, data_room, medal_corner, cafe, arrival.
}

# Same story for the scene_if_eq / scene_if_neq comparisons (0x21 / 0x22).
# main_menu reads these to decide whether to start its BGM: with the raw GBA
# offsets the host read a byte of objFont instead of the flag, the comparison
# went the wrong way, and the whole music block was skipped -- the main menu
# played silently.
SCENE_COND_FIELDS = {
    "scenes/main_menu/scene.bs": {
        0x1A: ("MainMenuSceneData", "enteredFromOptionsMenu", "src/scenes/main_menu.h"),
        0x1B: ("MainMenuSceneData", "exitingToOptionsMenu",   "src/scenes/main_menu.h"),
    },
    "scenes/options/scene.bs": {
        0x28: ("OptionsSceneData",  "canceledDataClear",      "src/scenes/options.h"),
    },
}


def scene_var_offset(src_path, raw, table=None):
    """Symbolic replacement for a raw scene-relative offset, or None."""
    if table is None:
        table = SCENE_VAR_FIELDS
    try:
        n = int(raw.strip(), 0)
    except ValueError:
        return None
    for suffix, offsets in table.items():
        if src_path.replace(os.sep, "/").endswith(suffix) and n in offsets:
            struct, field, header = offsets[n]
            return "offsetof(struct %s, %s)" % (struct, field), header
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("-o", "--output")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--root", default=".")
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    macros = load_macros(root)   # baseline; translate() re-derives per file
    index = build_symbol_index(root)
    consts = set(re.findall(r"^#define (\w+)", open(os.path.join(root,"include/beatscript_consts.h"),errors="replace").read(), re.M)) \
             if os.path.exists(os.path.join(root,"include/beatscript_consts.h")) else set()
    print(f"// {len(index)} symbols indexed", file=sys.stderr)
    print(f"// {len(macros)} macros loaded", file=sys.stderr)

    files = args.files
    if args.all:
        files = sorted(glob.glob(os.path.join(root, "**", "*.bs"), recursive=True))

    total_warn = 0
    for f in files:
        c, em = translate(f, macros, root, index, consts)
        out = args.output or (os.path.splitext(f)[0] + ".bs.c")
        if args.output == "-":
            print(c)
        else:
            open(out, "w").write(c)
        for w in em.warnings:
            print(f"  warn {os.path.basename(f)}: {w}", file=sys.stderr)
        total_warn += len(em.warnings)
        print(f"{os.path.relpath(f, root)} -> {len(em.blocks)} blocks", file=sys.stderr)
    print(f"// {len(files)} files, {total_warn} warnings", file=sys.stderr)


if __name__ == "__main__":
    main()
