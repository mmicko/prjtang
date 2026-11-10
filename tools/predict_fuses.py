#!/usr/bin/env python3
"""
Predict content of bitstream from physical netlist of design, using fuse
expressions from database, and compare it with bitstream made by TD.

Files are created in TD after place and route with:
    write_pnl design.pnl
    bitgen -bit design.bit -info -log_file design.log

Log file is used only to give names to bits that are different.
"""

import argparse, sys, os, re, json
from collections import Counter, defaultdict

from check_fuse_creation import read_bitstream

SITE_NAMES = {
	"mslice": "SLICE{}", "lslice": "SLICE{}", "mult18": "MULT18_{}", "emb": "EMB{}",
	"emb32k": "EMB32K{}", "iol": "IOL{}", "config": "CFG",
}

DIRS = {"n": (0, 1), "s": (0, -1), "e": (1, 0), "w": (-1, 0)}
SPAN_RE = re.compile(r"^([nsew])(\d)(beg|mid|end)(\d+)$")

OPPOSITE = {"n": "s", "s": "n", "e": "w", "w": "e"}

def reflect(pos, bounds):
	"""Wire that would leave the chip comes back in opposite direction"""
	x, y = pos
	rx = -1 - x if x < 0 else (2 * bounds[0] + 1 - x if x > bounds[0] else x)
	ry = -1 - y if y < 0 else (2 * bounds[1] + 1 - y if y > bounds[1] else y)
	return (rx, ry), (rx, ry) != (x, y)

def span_origin(x, y, wire, bounds):
	"""For wire going over more tiles return tile where it begins"""
	m = SPAN_RE.match(wire)
	if not m:
		return None
	d, length, tap, idx = m.group(1), int(m.group(2)), m.group(3), m.group(4)
	off = {"beg": 0, "mid": length // 2, "end": length}[tap]
	dx, dy = DIRS[d]
	pos, reflected = reflect((x - dx * off, y - dy * off), bounds)
	return pos, (OPPOSITE[d] if reflected else d, length, idx)

def span_names(origin, span, x, y, bounds):
	"""Names of wire going over more tiles as seen in given tile: {tap: name}"""
	d, length, idx = span
	dx, dy = DIRS[d]
	names = dict()
	for tap, o in (("beg", 0), ("mid", length // 2), ("end", length)):
		if tap == "mid" and length == 1:
			continue
		pos, reflected = reflect((origin[0] + dx * o, origin[1] + dy * o), bounds)
		if pos == (x, y):
			names[tap] = "{}{}{}{}".format(OPPOSITE[d] if reflected else d, length, tap, idx)
	return names

class Design:
	def __init__(self):
		self.sites = defaultdict(dict)   # (x,y) -> site name -> attributes
		self.models = defaultdict(dict)  # (x,y) -> site name -> model
		self.arcs = defaultdict(set)     # (x,y) -> (dst, src)
		self.wires = defaultdict(set)    # (x,y) -> wire
		self.inst_loc = dict()           # instance -> (x, y, site name)
		self.problems = Counter()
		self.through = 0
		self.routed = defaultdict(list)  # (x,y) -> (dst, [possible names of src])

	def resolve_arcs(self, known):
		"""Select name of source for each arc, from arcs known in database"""
		for loc, arcs in self.routed.items():
			for dst, sources in arcs:
				found = [w for w in sources if (dst, w) in known.get(loc, ())]
				if not found:
					self.problems["arc {} <- {} has no ARCVAL term in database".format(
						re.sub(r"\d+", "N", dst), re.sub(r"\d+", "N", sources[0]))] += 1
				src = found[0] if found else sources[0]
				self.wires[loc].add(src)
				self.wires[loc].add(dst)
				self.arcs[loc].add((dst, src))

	def used(self, loc):
		return loc in self.sites or loc in self.routed

def parse_loc(s):
	m = re.match(r"x(\d+)y(\d+)(?:_(\d+))?$", s)
	return int(m.group(1)), int(m.group(2)), (int(m.group(3)) if m.group(3) else 0)

# Content of LUT of adder is not written in netlist: (LUT0, LUT1)
ALU_LUT = {
	"ADD": (0x666A, 0x666A), "ADD_CARRY": (0x000A, 0x666A),
	"SUB": (0x999A, 0x999A), "SUB_CARRY": (0x0005, 0x999A),
	"A_LE_B": (0x999C, 0x999C), "A_LE_B_CARRY": (0x000A, 0x999C),
}

def parse_pnl(filename, bounds):
	d = Design()
	with open(filename) as f:
		text = f.read()
	for name, body in re.findall(r"\n\tinstance (\S+)\n\t\{(.*?)\n\t\}", text, re.S):
		model = re.search(r"model (\S+);", body).group(1)
		loc = re.search(r"location (\S+);", body)
		if not loc:
			continue
		x, y, z = parse_loc(loc.group(1))
		site = SITE_NAMES.get(model, model.upper()).format(z)
		attrs = dict()
		for line in re.search(r"attribute\s*\{(.*?)\n\t\t\}", body, re.S).group(1).split("\n"):
			m = re.match(r"([\w.\[\]]+)\s*([:=])\s*(.*)$", line.strip())
			if m:
				attrs[m.group(1)] = m.group(3)
		if model == "mslice" and attrs.get("MODE") == "RIPPLE" and "ALUTYPE" in attrs:
			if attrs["ALUTYPE"] in ALU_LUT:
				for k in (0, 1):
					attrs.setdefault("LUT{}".format(k), "16'h{:04X}".format(ALU_LUT[attrs["ALUTYPE"]][k]))
			else:
				d.problems["unknown ALUTYPE " + attrs["ALUTYPE"]] += 1
		if model == "pll":
			# dividers are kept as value - 1
			for k in list(attrs):
				if (k.endswith("_DIV") or k == "ICP_CURRENT") and attrs[k].isdigit():
					attrs[k] = str(int(attrs[k]) - 1)
				elif k.endswith("_FPHASE") and attrs[k].isdigit():
					attrs[k] = str((int(attrs[k]) + 5) % 8)
		d.sites[(x, y)][site] = attrs
		d.models[(x, y)][site] = model
		d.inst_loc[name] = (x, y, site)
	for name, body in re.findall(r"\n\tnet (\S+)\n\t\{(.*?)\n\t\}", text, re.S):
		route = re.search(r"route\s*\{(.*?)\}", body, re.S)
		if not route:
			continue
		for src, dst in re.findall(r"^\s*([^,\s]+),([^;\s]+);", route.group(1), re.M):
			stile, swire = src.split(".", 1)
			dtile, dwire = dst.split(".", 1)
			sx, sy, _ = parse_loc(stile)
			dx, dy, _ = parse_loc(dtile)
			if "." in swire or "." in dwire:
				continue # connection to pin of site
			# Switch is named with wire names of tile it is in, but route can
			# name wire going over more tiles in any tile it goes through
			tile = (dx, dy)
			dspan = span_origin(dx, dy, dwire, bounds)
			if dspan:
				tile = dspan[0]
				dwire = span_names(dspan[0], dspan[1], tile[0], tile[1], bounds)["beg"]
			sources = [swire] if (sx, sy) == tile else []
			sspan = span_origin(sx, sy, swire, bounds)
			if sspan:
				# wire turned back at edge of chip can be in tile twice, with
				# two names and two switches
				local = span_names(sspan[0], sspan[1], tile[0], tile[1], bounds)
				for tap in ("end", "mid"):
					if tap in local and local[tap] not in sources:
						sources.append(local[tap])
			if not sources:
				sources = [swire]
			m1 = re.match(r"^([abcd])(\d)$", swire)
			m2 = re.match(r"^(f|fx)(\d)$", dwire)
			if m1 and m2 and m1.group(2) == m2.group(2):
				# route goes through LUT that is not used by design
				n = int(m1.group(2))
				site = "SLICE{}".format(n // 2)
				attrs = d.sites[tile].setdefault(site, dict())
				if n < 4:
					attrs["LUT{}".format(n % 2)] = "(" + m1.group(1).upper() + ")"
				elif m2.group(1) == "f":
					attrs["LUTF{}".format(n % 2)] = "(" + m1.group(1).upper() + ")"
				else:
					attrs["LUTG{}".format(n % 2)] = "(" + m1.group(1).upper() + ")"
					attrs["LSFXMUX{}".format(n % 2)] = "LUTG"
				d.through += 1
				continue
			d.routed[tile].append((dwire.upper(), [w.upper() for w in sources]))
	return d

def lut_table(eq, size=16):
	"""Convert LUT equation to truth table"""
	if not re.match(r"^[A-F01~*+@() ]+$", eq):
		raise ValueError(eq)
	mask = (1 << size) - 1
	env = {"__builtins__": {}}
	for n, v in enumerate("ABCDEF"):
		env[v] = sum(1 << i for i in range(size) if (i >> n) & 1)
	expr = re.sub(r"\b1\b", "(-1)", eq).replace("*", "&").replace("+", "|").replace("@", "^")
	return eval(expr, env) & mask

def parse_number(val):
	m = re.match(r"(\d+)'([hbd])([0-9a-fA-F_]+)$", val)
	if m:
		return int(m.group(3).replace("_", ""), {"h": 16, "b": 2, "d": 10}[m.group(2)])
	if re.match(r"^\d+$", val):
		return int(val)
	return int(val, 0)

class Evaluator:
	def __init__(self, design):
		self.d = design
		self.unknown = Counter()
		self.ports_terms = 0

	def memory(self, loc, site, param, bit):
		attrs = self.d.sites[loc].get(site)
		if attrs is None:
			return False
		try:
			if "INIT_" + param in attrs:
				return bool((parse_number(attrs["INIT_" + param]) >> bit) & 1)
			if param not in attrs:
				return False
			if param.startswith("LUT") and "'" not in attrs[param]:
				return bool((lut_table(attrs[param], 16) >> bit) & 1)
			return bool((parse_number(attrs[param]) >> bit) & 1)
		except (ValueError, SyntaxError, NameError):
			self.unknown["value {}.{}={}".format(re.sub(r"\d+", "N", site), param, attrs.get(param))] += 1
			return False

	def term(self, loc, text):
		if text == "FALSE":
			return False
		if text == "TRUE":
			return True
		m = re.match(r"(\w+)\((.*)\)$", text)
		op, args = m.group(1), m.group(2).split(",")
		if op == "PROPERTY":
			site, prop = args[0].split(".", 1)
			attrs = self.d.sites[loc].get(site)
			if attrs is None or prop not in attrs:
				return False
			if attrs[prop] == args[1]:
				return True
			# number can be compared as 16 binary digits
			return bool(re.match(r"^[01]{16}$", args[1]) and attrs[prop].isdigit() and int(attrs[prop]) == int(args[1], 2))
		if op == "ARCVAL":
			return (args[0], args[1]) in self.d.arcs[loc]
		if op == "WIRE":
			return args[0] in self.d.wires[loc]
		if op == "COMP":
			return args[0] in self.d.sites[loc]
		if op in ("MEMORY", "SELECTOR"):
			return self.memory(loc, args[0], args[1], int(args[2]))
		if op == "PORTS":
			# Used only for inputs E and MI of LUT in slices 2 and 3. TD was
			# never seen to give it as reason for fuse, also when these
			# inputs of slice are connected, so meaning is not known.
			self.ports_terms += 1
			return False
		self.unknown["operation " + op] += 1
		return False

	def evaluate(self, loc, entry):
		stack = []
		for tok in entry["rpn"]:
			if tok == "+":
				b = stack.pop(); a = stack.pop(); stack.append(a or b)
			elif tok == "*":
				b = stack.pop(); a = stack.pop(); stack.append(a and b)
			elif tok == "~":
				stack.append(not stack.pop())
			elif tok in ("FALSE", "TRUE"):
				stack.append(tok == "TRUE")
			elif tok not in entry["data"]:
				self.unknown["token " + tok] += 1
				stack.append(False)
			else:
				stack.append(self.term(loc, entry["data"][tok]))
		if len(stack) != 1:
			self.unknown["expression " + "".join(entry["rpn"])] += 1
			return False
		return stack[0]

class Database:
	def __init__(self, db_dir, gaps):
		self.db_dir = db_dir
		self.gaps = gaps
		with open(os.path.join(db_dir, "tilegrid.json")) as f:
			self.tilegrid = json.load(f)
		self.by_loc = dict()
		self.pll_at = dict()
		for t in self.tilegrid.values():
			self.by_loc[(t["x"], t["y"], t["type"].lower())] = t
			m = re.match(r"^pll(\d+)$", t["type"])
			if m:
				self.pll_at[(t["x"], t["y"])] = int(m.group(1))
		self.bits = dict()

	def get_bits(self, tile_type):
		if tile_type not in self.bits:
			fn = os.path.join(self.db_dir, "bits", tile_type + ".json")
			self.bits[tile_type] = []
			if os.path.exists(fn):
				with open(fn) as f:
					self.bits[tile_type] = json.load(f)
		return self.bits[tile_type]

	def position(self, t, e):
		"""Return place of fuse in bitstream: ("pll", number, word, bit) or ("cram", frame, bit)"""
		if e["pll_info"]:
			if (t["x"], t["y"]) not in self.pll_at:
				return None
			return ("pll", self.pll_at[(t["x"], t["y"])], e["frame"], e["bit"])
		target = t
		if e["type"] != "NONE":
			target = self.by_loc.get((t["x"] + e["xoff"], t["y"] + e["yoff"], e["type"].lower()))
			if target is None:
				return None
		col = target["start_bit"] + e["bit"]
		pos = col
		for g in self.gaps:
			if col >= g:
				pos += 6
		return ("cram", target["start_frame"] + e["frame"], pos)

def main(argv):
	parser = argparse.ArgumentParser(prog="predict_fuses",
		usage="%(prog)s [options] <design>.pnl <design>.bit <db_dir>")
	parser.add_argument("pnl_file", help="physical netlist written by TD write_pnl")
	parser.add_argument("bit_file", help="bitstream generated by TD bitgen")
	parser.add_argument("db_dir", help="database directory of device (e.g. database/eg4/eagle_s20)")
	parser.add_argument("--log", dest="log_file", help="log file generated by TD bitgen -info, to name bits that differ")
	parser.add_argument("--gap", type=int, action="append", default=None,
		help="position in database where 6 bits not used by database are inserted in frame (default for eg4: 974 and 2920)")
	parser.add_argument("-n", type=int, default=3, help="examples to show for each group of differences")
	parser.add_argument("--groups", type=int, default=40, help="groups of differences to show")
	args = parser.parse_args(argv[1:])

	db = Database(args.db_dir, args.gap if args.gap is not None else [974, 2920])
	bounds = (max(t["x"] for t in db.tilegrid.values()), max(t["y"] for t in db.tilegrid.values()))
	design = parse_pnl(args.pnl_file, bounds)
	known = defaultdict(set)
	arcs_of_type = dict()
	for t in db.tilegrid.values():
		if t["type"] not in arcs_of_type:
			arcs_of_type[t["type"]] = set()
			for e in db.get_bits(t["type"]):
				for v in e["data"].values():
					m = re.match(r"ARCVAL\((\w+),(\w+)\)$", v)
					if m:
						arcs_of_type[t["type"]].add((m.group(1), m.group(2)))
		known[(t["x"], t["y"])] |= arcs_of_type[t["type"]]
	design.resolve_arcs(known)
	ev = Evaluator(design)

	# what TD did
	cram, pll_data = read_bitstream(args.bit_file)
	actual = set(("cram", f, b) for f, b in cram)
	for num, data in pll_data.items():
		for i in range(32 * 9):
			if data[i // 8] & (0x80 >> (i % 8)):
				actual.add(("pll", num, i // 9, 8 - i % 9))
	# names of fuses TD reported, to explain bits
	td_names = defaultdict(list)
	inst = None
	with open(args.log_file or os.devnull) as f:
		for line in f:
			line = line.strip()
			if line.startswith("//"):
				inst = line.split(":")[1].split(",")[0]
			elif line and not line.startswith("###") and inst in db.tilegrid:
				t = db.tilegrid[inst]
				name = line.split("\t")[0]
				for e in db.get_bits(t["type"]):
					if e["name"] == name:
						td_names[db.position(t, e)].append((inst, e))

	# what database says
	predicted = defaultdict(list)
	empty = dict()
	for inst, t in db.tilegrid.items():
		loc = (t["x"], t["y"])
		entries = db.get_bits(t["type"])
		if design.used(loc):
			on = [e for e in entries if ev.evaluate(loc, e)]
		else:
			if t["type"] not in empty:
				empty[t["type"]] = [e for e in entries if ev.evaluate(loc, e)]
			on = empty[t["type"]]
		for e in on:
			predicted[db.position(t, e)].append((inst, e))

	pset = set(predicted)
	missing = actual - pset   # set by TD, not predicted
	extra = pset - actual     # predicted, not set by TD
	print("bits set by TD: {}  predicted: {}  matching: {}  missing: {}  extra: {}".format(
		len(actual), len(pset), len(actual & pset), len(missing), len(extra)))
	for k, v in ev.unknown.most_common(10):
		print("  unknown: {} x{}".format(k, v))
	print("  arcs: {}  through LUT: {}  sites: {}".format(sum(len(a) for a in design.arcs.values()),
		design.through, sum(len(a) for a in design.sites.values())))
	for k, v in design.problems.most_common(args.groups):
		print("  note: {} x{}".format(k, v))

	groups = defaultdict(list)
	for kind, diff, names in (("missing", missing, td_names), ("extra", extra, predicted)):
		for pos in diff:
			cands = names.get(pos)
			if not cands:
				groups[(kind, "?", "bit without name")].append((str(pos), None))
				continue
			inst, e = cands[0]
			tt = db.tilegrid[inst]["type"]
			groups[(kind, tt, re.sub(r"\d+", "N", e["name"]))].append((inst, e))
	shown = 0
	for (kind, tt, pat), items in sorted(groups.items(), key=lambda kv: -len(kv[1])):
		shown += 1
		if shown > args.groups:
			break
		print("{:8} {:16} {:28} x{}".format(kind, tt, pat, len(items)))
		for inst, e in sorted(items, key=lambda i: i[0])[:args.n]:
			if e:
				print("      {}/{}: {} {} | {} {} {}".format(inst, e["name"], "".join(e["expr"])[:60],
					json.dumps(e["data"])[:220], e["type"], e["xoff"], e["yoff"]))
			else:
				print("      {}".format(inst))
	return 0 if not missing and not extra else 1

if __name__ == '__main__':
	sys.exit(main(sys.argv))
