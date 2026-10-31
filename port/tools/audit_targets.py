"""Static audit: every constant jump/call target in the ported code must have
a hook (a native routine) at that address, or the game would fall into the
interpreter there. Usage: python -I audit_targets.py <repo root> <cc>"""
import glob, os, re, subprocess, sys

root, cc = sys.argv[1], sys.argv[2]
port = os.path.join(root, 'port')

# label -> (bank, addr)
syms = {}
for line in open(os.path.join(root, 'warioland3.sym')):
    m = re.match(r'([0-9a-f]+):([0-9a-f]+) (\S+)', line)
    if m:
        syms[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))

files = sorted(glob.glob(os.path.join(port, 'src', 'ported', '**', '*.c'), recursive=True))
hooks = set()  # (bank, addr)
file_banks = {}
port_re = re.compile(r'^PORT(_[A-Z_]+)?\(\s*([A-Za-z0-9_]+)\s*(?:,\s*([A-Za-z0-9_]+))?')
for f in files:
    banks = set()
    for line in open(f, encoding='utf-8'):
        m = port_re.match(line)
        if not m or (m.group(1) or '').startswith('_RAM'):
            continue
        name = m.group(2)
        if (m.group(1) or '').startswith('_LOCAL'):
            name += '.' + m.group(3)
        if name in syms:
            b, a = syms[name]
            b = 0 if a < 0x4000 else b
            hooks.add((b, a))
            banks.add(b)
    file_banks[f] = banks


def args_of(text, start):
    """The arguments of a call whose '(' is at start, split at top-level commas."""
    depth, i, cur, out = 0, start, '', []
    while i < len(text):
        c = text[i]
        if c == '(':
            depth += 1
            if depth == 1:
                i += 1
                continue
        elif c == ')':
            depth -= 1
            if depth == 0:
                out.append(cur)
                return out
        elif c == ',' and depth == 1:
            out.append(cur)
            cur = ''
            i += 1
            continue
        cur += c
        i += 1
    return None


src_cache = {}
marker_re = re.compile(r'^# (\d+) "([^"]+)"')


def line_index(text):
    """For each line of cc -E output, the (file, line) it came from."""
    out = []
    cur_file, cur_line = None, 0
    for ln in text.split('\n'):
        m = marker_re.match(ln)
        if m:
            cur_line = int(m.group(1))
            cur_file = m.group(2).replace('\\\\', '/')
            out.append((None, 0))
            continue
        out.append((cur_file, cur_line))
        cur_line += 1
    return out


def source_line(path, n):
    if path not in src_cache:
        try:
            src_cache[path] = open(path, encoding='utf-8').read().split('\n')
        except OSError:
            src_cache[path] = []
    lines = src_cache[path]
    return lines[n - 1] if 0 < n <= len(lines) else ''


def const_value(expr):
    e = re.sub(r'\((?:uint8_t|uint16_t|int|unsigned|size_t)\)', '', expr)
    e = re.sub(r'(0x[0-9a-fA-F]+|\d+)[uUlL]+', r'\1', e)
    if re.search(r'[A-Za-z_]\w*', re.sub(r'0x[0-9a-fA-F]+', '', e)):
        return None  # uses a variable: not a constant target
    try:
        return int(eval(e)) & 0xffff
    except Exception:
        return None


inc = ['-I', os.path.join(port, 'src'), '-I', os.path.join(port, 'generated'),
       '-I', os.path.join(port, 'build', 'generated')]
missing = {}
patterns = [
    (r'gb->cpu\.pc = \(uint16_t\)', 'JUMP_TO', 0, None),
    (r'\bgb_call\s*', 'gb_call', 1, None),
    (r'\bgb_call_ret\s*', 'gb_call_ret', 1, None),
    (r'\bgb_call_ret_unwind\s*', 'gb_call_ret_unwind', 1, None),
    (r'\bport_call\s*', 'CALL', 2, 1),
    (r'\bport_jump\s*', 'JUMP', 2, 1),
    (r'\bport_farcall\s*', 'FARCALL', 2, 1),
    (r'\bport_farcall_ret\s*', 'FARCALL_RET', 2, 1),
]
for f in files:
    out = subprocess.run([cc, '-E'] + inc + [f], capture_output=True, text=True)
    if out.returncode:
        print('preprocess failed', f, out.stderr[:300])
        continue
    text = out.stdout
    index = line_index(text)
    for pat, kind, ai, bi in patterns:
        for m in re.finditer(pat, text):
            p = m.end()
            if p >= len(text) or text[p] != '(':
                continue
            args = args_of(text, p)
            if not args or len(args) <= ai:
                continue
            addr = const_value(args[ai])
            if addr is None or addr >= 0x8000:
                continue
            src_file, src_n = index[text.count('\n', 0, m.start())]
            src = source_line(src_file, src_n) if src_file else ''
            if bi is not None:
                b = const_value(args[bi])
                cands = {0 if addr < 0x4000 else (b if b is not None else -1)}
            elif addr < 0x4000:
                cands = {0}
            else:
                # the bank of the ADDR_X label on the source line, else the
                # banks of the file's own routines
                labs = [x for x in re.findall(r'ADDR_(\w+)', src) if x in syms]
                cands = {syms[x][0] for x in labs} or file_banks[f]
            if any((b, addr) in hooks for b in cands):
                continue
            key = (os.path.relpath(f, port), kind, addr, tuple(sorted(cands)), src.strip()[:90])
            missing[key] = missing.get(key, 0) + 1

names = {}
for n, (b, a) in syms.items():
    names.setdefault((b, a), []).append(n)
for (f, kind, addr, cands, src), n in sorted(missing.items()):
    lab = [x for b in cands for x in names.get((0 if addr < 0x4000 else b, addr), [])]
    print(f'{f}: {kind} {"/".join("%02x" % b for b in cands)}:{addr:04x} {",".join(lab) or "(no label)"}')
    print('    ' + src)
print(len(missing), 'targets without a hook')
