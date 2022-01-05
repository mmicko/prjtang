import argparse
import sys
import os
import json

decrypt = None
inst = dict() 
bcc_info = dict()
info = []
def print_decrypt(val):
	global decrypt
	if (decrypt):
		print(val, file=decrypt)
def print_decrypt_tofile(f, val):
	print(val, file=f)

def decode_string(input):
	global keypos
	global key
	o = ""
	i = 0
	while i < len(input):
		inp = ord(input[i])
		mod = ord(key[keypos])
		z = inp - mod
		if (inp < mod):
			z = z + 0x5d
		if (z < 0x20):
			z = z + 0x5d
		if (inp == 0x20):
			z = 0x20
		keypos += 1
		if (keypos == len(key)):
			keypos = 0
		if (inp != 0x21):
			o += chr(z)
		else:
			i += 1
			o += input[i]
		i += 1
	return o

def decode(fp, ids):
	items = fp.readline().split()
	for i in range(len(ids)):
		items[ids[i]] = decode_string(items[ids[i]])
	out = " ".join(items)
	return items,out

def decode_items(items, ids):
	for i in range(len(ids)):
		items[ids[i]] = decode_string(items[ids[i]])
	out = " ".join(items)
	return items,out

def decode_skip(fp, ids):
	items = fp.readline().split()
	for i in range(len(items)):
		if i not in ids:
			items[i] = decode_string(items[i])
	out = " ".join(items)
	return items,out

class DictAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        assert isinstance(getattr(namespace, self.dest), dict), "Use ArgumentParser.set_defaults() to initialize {} to dict()".format(self.dest)
        name = option_string.lstrip(parser.prefix_chars).replace("-", "_")
        getattr(namespace, self.dest)[name] = values

def decode_chipdb(argv):
	parser = argparse.ArgumentParser(prog="unlogic",
		usage="%(prog)s [options] <db_name>.db")
	parser.set_defaults(file_paths=dict())
	parser.add_argument("dbfile", metavar="<db_file>.db>", nargs="?",
        help="Anlogic architecture .db file")
	parser.add_argument("--db_dir", metavar="<path_to_database_directory>",
        action=DictAction, dest="file_paths")
	parser.add_argument("--decrypt", metavar="<path_to_decrypted_file>",
        action=DictAction, dest="file_paths")
	args = parser.parse_args(argv[1:])	
	dbfile = args.dbfile
	global decrypt
	decrypt = None
	file_paths = args.file_paths
		
	if dbfile is None:
		parser.print_help()
		sys.exit()

	if not os.path.isfile(dbfile):
		print("File path {} does not exist...".format(dbfile))
		sys.exit()

	global key
	global keypos
	keypos = 0
	key = ""
	is_ph1 = False
	tiles = dict()

	if "decrypt" in file_paths.keys():
		decrypt = open(file_paths["decrypt"], "wt")

	with open(dbfile) as fp:
		global info
		info = []
		# First line contain encryption key
		items, out  = decode(fp, [])
		print_decrypt(out)

		key = items[1]
		arch = int(items[2])
		if (key=="ph1_100"):
			is_ph1 = True

		for i in range(arch):
			# arch names
			items,out  = decode(fp, [0])			
			print_decrypt(out)
		for i in range(arch):
			# eagle_20 NONE NONE 8 8 41 72 2450 64 29 16 0 2
			items,out  = decode(fp, [0,1,2,3,4,5])
			print_decrypt(out)
			items,out  = decode(fp, [])
			print_decrypt(out)
			# 1 1 0 0 0
			items,out  = decode(fp, [])
			print_decrypt(out)
			for j in range(5):
				for k in range(int(items[j])):
					dummy,out  = decode(fp, [0])
					print_decrypt(out)
			# 17 23
			num,out  = decode(fp, [])
			print_decrypt(out)
			n = int(num[0]) + int(num[1])
			# n element array
			unk,out  = decode(fp, [])
			print_decrypt(out)
			num,out  = decode(fp, [])
			print_decrypt(out)
			for j in range(4):
				l = []
				for k in range(int(num[j])):
					l.append(1+k*2)
				if (len(l)>0):
					unk,out  = decode(fp, l)
					print_decrypt(out)
			# usually 0 
			num_el,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			assert int(num_el[0])==len(unk)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			
			blk,out  = decode(fp, [])
			print_decrypt(out)
			for k in range(int(blk[1])):
				blocks,out  = decode(fp, [])
				print_decrypt(out)
				for j in range(3,49):
					unk,out  = decode(fp, [])
					print_decrypt(out)
					assert int(unk[0])==j
					unk,out  = decode(fp, [])
					print_decrypt(out)
					unk,out  = decode(fp, [])
					print_decrypt(out)
					items,out  = decode(fp, [])
					print_decrypt(out)
					for l in range(5):
						for k in range(int(items[l])):
							dummy,out  = decode(fp, [0])
							print_decrypt(out)
				for k in range(int(blocks[8])):
					items,out  = decode(fp, [])
					print_decrypt(out)

		# wires and pips
		blocks, out = decode(fp, [])
		print_decrypt(out)
		for i in range(int(blocks[0])):
			unk,out  = decode(fp, [])
			print_decrypt(out)
			num = int(unk[4])
			for j in range(num):
				unk,out  = decode_skip(fp, [1])
				print_decrypt(out)
		for i in range(16):
			unk,out  = decode(fp, [])
			assert(0 == int(unk[0]))
			assert(0 == int(unk[1]))
			assert(0 == int(unk[2]))
			assert(0 == int(unk[3]))
			print_decrypt(out)
			num = int(unk[4])
			for j in range(num):
				unk,out  = decode_skip(fp, [1])
				print_decrypt(out)
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [])
			# ef3_4 and ef3_9 need 2nd
			assert int(unk[0])==i+1 or int(unk[0])==i
			print_decrypt(out)
			unk,out  = decode(fp, [])
			assert(0 == int(unk[0]))
			assert(0 == int(unk[1]))
			assert(0 == int(unk[2]))
			assert(0 == int(unk[3]))
			print_decrypt(out)
			num = int(unk[4])
			for j in range(num):
				unk,out  = decode_skip(fp, [1])
				print_decrypt(out)
		for i in range(19 if is_ph1 else 9):
			unk,out  = decode(fp, [])
			assert(0 == int(unk[0]))
			assert(0 == int(unk[1]))
			assert(0 == int(unk[2]))
			assert(0 == int(unk[3]))
			print_decrypt(out)
			num = int(unk[4])
			for j in range(num):
				unk,out  = decode_skip(fp, [1])
				print_decrypt(out)

		if is_ph1:
			unk,out  = decode(fp, [])
			print_decrypt(out)
			for i in range(2):
				unk,out  = decode(fp, [])
				print_decrypt(out)
				unk,out  = decode(fp, [])
				assert(0 == int(unk[0]))
				assert(0 == int(unk[1]))
				assert(0 == int(unk[2]))
				assert(0 == int(unk[3]))
				print_decrypt(out)
				num = int(unk[4])
				for j in range(num):
					unk,out  = decode_skip(fp, [1])
					print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			for i in range(2):
				unk,out  = decode(fp, [])
				print_decrypt(out)
				unk,out  = decode(fp, [])
				assert(0 == int(unk[0]))
				assert(0 == int(unk[1]))
				assert(0 == int(unk[2]))
				assert(0 == int(unk[3]))
				print_decrypt(out)
				num = int(unk[4])
				for j in range(num):
					unk,out  = decode_skip(fp, [1])
					print_decrypt(out)
			for i in range(5):
				unk,out  = decode(fp, [])
				assert(0 == int(unk[0]))
				assert(0 == int(unk[1]))
				assert(0 == int(unk[2]))
				assert(0 == int(unk[3]))
				print_decrypt(out)
				num = int(unk[4])
				for j in range(num):
					unk,out  = decode_skip(fp, [1])
					print_decrypt(out)
			blocks,out  = decode(fp, [])
			print_decrypt(out)
			for i in range(int(blocks[0])):
				unk,out  = decode(fp, [0])
				print_decrypt(out)

		# models
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		for i in range(int(blocks[0])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)

		for k in range(int(blocks[0])):
			blocks,out  = decode(fp, [])
			print_decrypt(out)
			for i in range(int(blocks[1])):
				blocks,out  = decode(fp, [])
				print_decrypt(out)
				for j in range(int(blocks[1])):
					unk,out  = decode(fp, [0])
					print_decrypt(out)
					if (int(unk[2])==1):
						unk,out  = decode(fp, [1])
					elif (int(unk[2])==2):
						unk,out  = decode(fp, [1,5])
					elif (int(unk[2])==3):
						unk,out  = decode(fp, [1,5,9])
					print_decrypt(out)

		# architecture
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		for i in range(int(blocks[0])):
			unk,out  = decode_skip(fp, [0])
			print_decrypt(out)

		# xml
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		assert blocks[0]=="pack"
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		assert blocks[0]=="kcap"

		# timing lib
		blocks,out  = decode(fp, [0])
		print_decrypt(out)
		assert blocks[0]=="TimingLib"
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [0,1])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
		for i in range(int(blocks[2])):
			unk,out  = decode(fp, [0,1])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
		for i in range(int(blocks[3])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
			k = int(unk[4])
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			for j in range(k):
				unk,out  = decode(fp, [0])
				print_decrypt(out)
				unk,out  = decode(fp, [])
				print_decrypt(out)
			empty,out  = decode(fp, [])
			print_decrypt(out)
			assert len(empty)==0

		items = fp.readline().split()
		if len(items)==0:
			empty,out  = decode_items(items, [])
			assert len(empty)==0
			print_decrypt(out)
		else:
			#pcie
			blocks,out  = decode_items(items, [0])
			print_decrypt(out)
			for i in range(int(blocks[1])):
				unk,out  = decode(fp, [0])
				print_decrypt(out)
			#serdes
			blocks,out  = decode(fp, [0])
			print_decrypt(out)
			for i in range(int(blocks[1])):
				unk,out  = decode(fp, [0])
				print_decrypt(out)
			
			empty,out  = decode(fp, [])
			assert len(empty)==0
			print_decrypt(out)

		# bitstream cell type info
		blocks,out  = decode(fp, [0])
		print_decrypt(out)
		mapping = { 0 : 'FALSE', 1:'TRUE', 2:'?', 3:'(', 4 : ')', 5 : '+', 6:'?', 7:'*', 8:'~', 9:'?'}
		assert blocks[0]=="bcc_info"
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
			global bcc_info
			bcc_name = unk[0]
			k = int(unk[3])
			bits = []
			tiles_addressed = dict()
			for j in range(k):
				unk,out  = decode(fp, [0,1])
				print_decrypt(out)
				current_item = {
					"name": unk[0],
					"type": unk[1],
					"frame": int(unk[2]),
					"bit": int(unk[3]),
					"xoff": int(unk[4]),
					"yoff": int(unk[5]),
					"map_wire_arc": int(unk[6]), # maps wire or arc in expression
					"remap": int(unk[7]),
					"pll_info": int(unk[8]),
					"cnt": int(unk[11]),
					"expr": [],
					"rpn": [],
					"data": [],
				}
				tiles_addressed[unk[4]+","+unk[5]] = 1
				assert(int(unk[6])==0 or int(unk[6])==1)
				assert(int(unk[7])==0 or int(unk[7])==1)
				assert(int(unk[8])==0 or int(unk[8])==1)
				if (unk[1]=="NONE"):
					assert(int(unk[7])==0)
				else:
					assert(int(unk[7])==1)
				n1 = int(unk[9])
				n2 = int(unk[10])
				n3 = int(unk[11])
				
				expr = []
				for l in range(n1):
					unk,out  = decode(fp, [])
					if(int(unk[0])>=10):
						print_decrypt(chr(55+int(unk[0])))
						expr.append(chr(55+int(unk[0])))
					else:
						print_decrypt(mapping[int(unk[0])])
						expr.append(mapping[int(unk[0])])
				empty,out  = decode(fp, [])
				print_decrypt(out)
				assert len(empty)==0
				
				rpn = []
				for l in range(n2):
					unk,out  = decode(fp, [])
					if(int(unk[0])>=10):
						print_decrypt(chr(55+int(unk[0])))
						rpn.append(chr(55+int(unk[0])))
					else:
						print_decrypt(mapping[int(unk[0])])
						rpn.append(mapping[int(unk[0])])
				empty,out  = decode(fp, [])
				print_decrypt(out)
				assert len(empty)==0

				data = dict()
				for l in range(n3):
					unk,out  = decode(fp, [1])
					print_decrypt(chr(55+int(unk[0]))+" "+ unk[1])
					data[chr(55+int(unk[0]))] = unk[1]
					operation = ""
					if ("(" in unk[1]):
						sub = unk[1].split("(")
						operation = sub[0]
					else:
						operation = unk[1]
					#if (operation=="SELECTOR"):
					#	print(unk[1].replace("SELECTOR(","").replace(")",""))
					#if (operation=="PROPERTY"):
					#	print(unk[1].replace("PROPERTY(","").replace(")",""))
					#if (operation=="MEMORY"):
					#	print(unk[1].replace("MEMORY(","").replace(")",""))
					#if (operation=="ARCVAL"):
					#	print(unk[1].replace("ARCVAL(","").replace(")",""))
					#if (operation=="WIRE"):
					#	print(unk[1].replace("WIRE(","").replace(")",""))
					#if (operation=="PORTS"):
					#	print(unk[1].replace("PORTS(","").replace(")",""))
					#if (operation=="COMP"):
					#	print(unk[1].replace("COMP(","").replace(")",""))
					# 'PROPERTY' 'ARCVAL' 'FALSE' 'COMP' 'MEMORY' 'SELECTOR' 'WIRE' 'PORTS' 'TRUE'
					# always separate : FALSE, TRUE, MEMORY and SELECTOR
					# can be with other in group: PROPERTY, ARCVAL, COMP, WIRE and PORTS

				current_item["expr"] = expr
				current_item["rpn"] = rpn
				current_item["data"] = data

				bits.append(current_item)

				empty,out  = decode(fp, [])
				print_decrypt(out)
				assert len(empty)==0

			#if len(tiles_addressed)>1:
			#	print(bcc_name, len(tiles_addressed))
				
			if "db_dir" in file_paths.keys():
				json_file = os.path.join(file_paths["db_dir"], "bits", bcc_name + ".json")
				with open(json_file, "wt") as f:
					json.dump(bits, f, indent=4)


			bcc_info[bcc_name] = bits
			empty,out  = decode(fp, [])
			print_decrypt(out)
			assert len(empty)==0
		empty,out  = decode(fp, [])
		print_decrypt(out)
		assert len(empty)==0

		# bitstream top info
		blocks,out  = decode(fp, [0])
		print_decrypt(out)
		assert blocks[0]=="bil_info"
		
		max_col = int(blocks[1])
		max_row = int(blocks[2])
		for i in range(max_row ):
			row = []
			for j in range(max_col):
				row.append([])
			row2 = []
			for j in range(max_col):
				row2.append([])
			info.append(row2)
		bl = int(blocks[8])
		bl2 = int(blocks[7])
		total_num = 0
		global inst		
		for i in range(max_row*max_col):
			unk,out  = decode(fp, [])
			print_decrypt(out)
			x = int(unk[0])
			y = int(unk[1])
			num = int(unk[2])
			total_num += num
			info[y][x] = dict()
			for j in range(num):
				unk,out  = decode(fp, [0,1])
				print_decrypt(out)
				assert x==int(unk[2])
				assert y==int(unk[3])
				current_item = {
					#"inst": unk[0],
					"type": unk[1],
					"x": int(unk[2]),
					"y": int(unk[3]),
					"rows": int(unk[4]),
					"cols": int(unk[5]),
					"start_frame": int(unk[6]),
					"start_bit": int(unk[7]),
					"flag": int(unk[8]),
					"sites": []
				}
				sites = []
				if (unk[1]=="plb"):
					sites.append({"name": "slice0", "type": "mslice"})
					sites.append({"name": "slice1", "type": "mslice"})
					sites.append({"name": "slice2", "type": "lslice"})
					sites.append({"name": "slice3", "type": "lslice"})
				if (unk[1].startswith("iol_pair")):
					sites.append({"name": "pad0", "type": "iol"})
					sites.append({"name": "pad1", "type": "iol"})
				if (unk[1].startswith("iol_quad")):
					sites.append({"name": "pad0", "type": "iol"})
					sites.append({"name": "pad1", "type": "iol"})
					sites.append({"name": "pad2", "type": "iol"})
					sites.append({"name": "pad3", "type": "iol"})
				if (unk[1] == "emb_slice"):
					sites.append({"name": "emb0", "type": "emb"})
					sites.append({"name": "emb1", "type": "emb"})
					sites.append({"name": "emb2", "type": "emb"})
					sites.append({"name": "emb3", "type": "emb"})
					sites.append({"name": "fifo0", "type": "fifo"})
					sites.append({"name": "fifo1", "type": "fifo"})
					sites.append({"name": "fifo2", "type": "fifo"})
					sites.append({"name": "fifo3", "type": "fifo"})
				if (unk[1] == "emb32k"):
					sites.append({"name": "emb32k0", "type": "emb32k"})
					sites.append({"name": "emb32k1", "type": "emb32k"})
				if (unk[1] == "dsp"):
					sites.append({"name": "mult0", "type": "mult18"})
					sites.append({"name": "mult1", "type": "mult18"})
					sites.append({"name": "mult2", "type": "mult18"})
					sites.append({"name": "mult3", "type": "mult18"})
				if (unk[1]=="config"):
					sites.append({"name": "config", "type": "config"})
				if (unk[1]=="osc"):
					sites.append({"name": "osc", "type": "osc"})
				if (unk[1]=="pwrmnt"):
					sites.append({"name": "pwrmnt", "type": "pwrmnt"})
				if (unk[1]=="pll_matrix"):
					sites.append({"name": "pll", "type": "pll"})
				current_item["sites"] = sites
				if (int(unk[8])==0):
					assert(int(unk[4])==0)
					assert(int(unk[5])==0)
					assert(int(unk[6])==0)
					assert(int(unk[7])==0)
				if (int(unk[8])==-1):
					info[y][x][unk[1]] = current_item
				inst[unk[0]] = current_item
				tiles[unk[0]] = current_item
				empty,out  = decode(fp, [])
				print_decrypt(out)
				assert len(empty)==0

		assert(total_num == bl2)
		empty,out  = decode(fp, [])
		print_decrypt(out)
		assert len(empty)==0

		# bel types
		for i in range(bl):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
		
		empty,out  = decode(fp, [])
		print_decrypt(out)
		assert len(empty)==0
		
		while 1:
			unk,out  = decode(fp, [0])
			print_decrypt(out)
			if unk[0]=="power%%20no":
				break
			if unk[0]=="FOOT_IOB":
				break

		unk,out  = decode(fp, [])
		print_decrypt(out)

	if "decrypt" in file_paths.keys():
		decrypt.close()

	if "db_dir" in file_paths.keys():
		json_file = os.path.join(file_paths["db_dir"], "tilegrid.json")
		with open(json_file, "wt") as fout:
			json.dump(tiles, fout, indent=4)

if __name__ == '__main__':
	decode_chipdb(sys.argv)