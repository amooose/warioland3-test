# c2cpp.py in.c Class out.cpp "header comment" [namespace]: a register-level
# ported C file -> a C++ class over AsmModule (src/native/asm_module.hpp), the
# same logic: static functions and PORT bodies become members, the C
# helpers (gb, alu_*, rd8/wr8...) the base class's, and each hook calls its
# member. Review the output; it must still be fuzzed.
import re, sys

src, cls, out, comment = sys.argv[1:5]
ns = sys.argv[5] if len(sys.argv) > 5 else 'wl3::level'
import os
inc = os.path.relpath(os.path.abspath('src/native/asm_module.hpp'), os.path.dirname(os.path.abspath(out))).replace(os.sep, '/')  # run from port/
c = open(src, encoding='utf-8').read()

# drop the includes and file-wide helper macros the base provides
c = re.sub(r'^#include .*\n', '', c, flags=re.M)
for name in ['ZF', 'CF', 'CPU', 'OAM1', 'OAM2', 'OAM3', 'LJUMP', 'LCALL']:
	c = re.sub(r'^#define ' + name + r'\b.*\n', '', c, flags=re.M)
c = re.sub(r'^// Port of .*\n', '', c, flags=re.M)

hooks = []
def port(m):
	kind, args = m.group(1), m.group(2)
	parts = [a.strip() for a in args.split(',')]
	if kind == 'PORT':
		name, flags, clob = parts[0], '0', '0'
		label = name
	elif kind == 'PORT_JUMP':
		name, flags, clob = parts[0], 'HOOK_MANUAL_RET', '0'
		label = name
	elif kind == 'PORT_EX':
		name, flags, clob = parts
		label = name
	elif kind == 'PORT_LOCAL':
		name, flags, clob = parts[0] + '__' + parts[1], '0', '0'
		label = (parts[0], parts[1])
	elif kind == 'PORT_LOCAL_JUMP':
		name, flags, clob = parts[0] + '__' + parts[1], 'HOOK_MANUAL_RET', '0'
		label = (parts[0], parts[1])
	elif kind == 'PORT_LOCAL_EX':
		name, flags, clob = parts[0] + '__' + parts[1], parts[2], parts[3]
		label = (parts[0], parts[1])
	hooks.append((label, flags, clob, name))
	return 'void port_' + name + '()'
c = re.sub(r'^(PORT_LOCAL_JUMP|PORT_LOCAL_EX|PORT_LOCAL|PORT_JUMP|PORT_EX|PORT)\(([^)]*)\)', port, c, flags=re.M)

# static functions -> members; forward declarations go (members needn't
# be declared before use), including those over more than a line
c = re.sub(r'^static (inline )?[^=;{}\n]*?\b\w+\([^;{}]*\);\n', '', c, flags=re.M)
c = re.sub(r'^static (inline )?([\w\s\*]+?\b\w+)\(GB \*gb\);\n', '', c, flags=re.M)
c = re.sub(r'^static (inline )?([^=;\n]*?\b\w+\()GB \*gb, ', r'\2', c, flags=re.M)
c = re.sub(r'^static (inline )?([^=;\n]*?\b\w+\()GB \*gb\)', r'\2)', c, flags=re.M)
c = re.sub(r'^static (inline )?([^=;\n]*?\b\w+\([^)]*\);)\n', '', c, flags=re.M)
# file-level tables: static const -> static constexpr members
c = re.sub(r'^static const\b', 'static constexpr', c, flags=re.M)

# the CPU, the ALU and memory
c = c.replace('gb->cpu.', 'cpu_.').replace('CPU->', 'cpu_.').replace('&gb->cpu', '&cpu_')
c = re.sub(r'gb->', 'gb_->', c)
c = c.replace('gb->rom_bank', 'gb_->rom_bank').replace('gb->test', 'gb_->test').replace('gb->no_tick', 'gb_->no_tick')
ren = {'and': 'and_', 'or': 'or_', 'xor': 'xor_'}
c = re.sub(r'\balu_(\w+)\(gb, ', lambda m: 'alu_.' + ren.get(m.group(1), m.group(1)) + '(', c)
for f in ['SET_HL', 'SET_DE', 'SET_BC', 'SET_AF', 'gb_push16', 'gb_call', 'gb_call_ret', 'gb_call_ret_unwind',
		'port_farcall', 'port_farcall_ret', 'port_hcall', 'port_call', 'port_jump', 'gb_tick', 'gb_read', 'gb_write']:
	c = re.sub(r'\b' + f + r'\(gb, ', f + '(gb_, ', c)
for f in ['REG_HL', 'REG_DE', 'REG_BC', 'REG_AF', 'gb_pop16']:
	c = re.sub(r'\b' + f + r'\(gb\)', f + '(gb_)', c)
# the remaining helpers: f(gb, ...) -> f(...), f(gb) -> f()
c = re.sub(r'\b(\w+)\(gb, ', r'\1(', c)
c = re.sub(r'\b(\w+)\(gb\)', r'\1()', c)
if re.search(r'\bgb\b', c):
	for i, line in enumerate(c.split('\n')):
		if re.search(r'\bgb\b', line):
			print('gb left:', i + 1, line.strip())

body = '\n'.join('\t' + l if l.strip() else '' for l in c.strip('\n').split('\n'))
lines = []
for label, flags, clob, name in hooks:
	if isinstance(label, tuple):
		lines.append(f'PORT_LOCAL_EX({label[0]}, {label[1]}, {flags}, {clob}) {{ {cls}(gb).port_{name}(); }}')
	else:
		lines.append(f'PORT_EX({label}, {flags}, {clob}) {{ {cls}(gb).port_{name}(); }}')

text = f'''{comment}
#include "{inc}"

namespace {ns} {{

namespace {{

class {cls} : public AsmModule {{
public:
	using AsmModule::AsmModule;

{body}
}};

}} // namespace

}} // namespace {ns}

using {ns}::{cls};

{chr(10).join(lines)}
'''
open(out, 'w', encoding='utf-8', newline='\n').write(text)
print(len(hooks), 'hooks')
