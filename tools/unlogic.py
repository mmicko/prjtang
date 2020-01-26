import argparse
import sys
import os
import json

decrypt = None

def print_decrypt(val):
	global decrypt
	if (decrypt):
		print(val, file=decrypt)

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
	parser.add_argument("--tilegrid", metavar="<path_to_tilegrid_json>",
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

	tiles = []

	if "decrypt" in file_paths.keys():
		decrypt = open(file_paths["decrypt"], "wt")

	with open(dbfile) as fp:
		# First line contain encryption key
		items, out  = decode(fp, [])
		print_decrypt(out)

		key = items[1]
		arch = int(items[2])

		for i in range(arch):
			# arch names
			items,out  = decode(fp, [0])
			print_decrypt(out)
		for i in range(arch):
			# eagle_20 NONE NONE 8 8 41 72 2450 64 29 16 0 2
			items,out  = decode(fp, [0,1,2])
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
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			for j in range(3,42):
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
			print_decrypt(out)
			num = int(unk[4])
			for j in range(num):
				unk,out  = decode_skip(fp, [1])
				print_decrypt(out)
		for i in range(9):
			unk,out  = decode(fp, [])
			print_decrypt(out)
			num = int(unk[4])
			for j in range(num):
				unk,out  = decode_skip(fp, [1])
				print_decrypt(out)
		for i in range(65):
			blocks,out  = decode(fp, [])
			print_decrypt(out)
			for j in range(int(blocks[0])):
				n = int(blocks[j+1])
				for j in range(n):
					unk,out  = decode(fp, [0])
					print_decrypt(out)
					if (int(unk[2])==1):
						unk,out  = decode(fp, [1])
					else:
						unk,out  = decode(fp, [1,5])
					print_decrypt(out)
					
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		for i in range(int(blocks[0])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)

		blocks,out  = decode(fp, [])
		print_decrypt(out)
		for i in range(int(blocks[0])):
			unk,out  = decode_skip(fp, [0])
			print_decrypt(out)
		
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		assert blocks[0]=="pack"
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
		blocks,out  = decode(fp, [])
		print_decrypt(out)
		assert blocks[0]=="kcap"

		blocks,out  = decode(fp, [0])
		print_decrypt(out)
		assert blocks[0]=="TimingLib"
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [0,1])
			print_decrypt(out)
		for i in range(int(blocks[2])):
			unk,out  = decode(fp, [0,1])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
			unk,out  = decode(fp, [])
			print_decrypt(out)
		for i in range(int(blocks[3])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
			k = int(unk[2])
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
			
		blocks,out  = decode(fp, [0])
		print_decrypt(out)
		assert blocks[0]=="bcc_info"
		for i in range(int(blocks[1])):
			unk,out  = decode(fp, [0])
			print_decrypt(out)
			k = int(unk[3])
			for j in range(k):
				unk,out  = decode(fp, [0,1])
				print_decrypt(out)
				n1 = int(unk[9])
				unk,out  = decode(fp, [])
				print_decrypt(out)
				unk,out  = decode(fp, [])
				print_decrypt(out)
				for l in range(n1):
					unk,out  = decode(fp, [1])
					print_decrypt(out)
				empty,out  = decode(fp, [])
				print_decrypt(out)
				assert len(empty)==0
			empty,out  = decode(fp, [])
			print_decrypt(out)
			assert len(empty)==0
		empty,out  = decode(fp, [])
		print_decrypt(out)
		assert len(empty)==0
		blocks,out  = decode(fp, [0])
		print_decrypt(out)
		assert blocks[0]=="bil_info"
		
		max_col = int(blocks[1])
		max_row = int(blocks[2])
		for i in range(max_row ):
			row = []
			for j in range(max_col):
				row.append([])
			tiles.append(row)		
		bl = int(blocks[8])
		bl2 = int(blocks[7])		
		for i in range(max_row*max_col):
			unk,out  = decode(fp, [])
			print_decrypt(out)
			x = int(unk[0])
			y = int(unk[1])
			num = int(unk[2])
			
			tile_val = []
			for j in range(num):
				unk,out  = decode(fp, [0,1])
				print_decrypt(out)
				assert x==int(unk[2])
				assert y==int(unk[3])
				current_item = {
					"inst": unk[0],
					"type": unk[1],
					"unk1": int(unk[4]),
					"unk2": int(unk[5]),
					"wl_beg": int(unk[6]),
					"bl_beg": int(unk[7]),
					"unk5": int(unk[8])
				}
				tile_val.append(current_item)

				empty,out  = decode(fp, [])
				print_decrypt(out)
				assert len(empty)==0

			current_tile = {
				"x": x,
				"y": y,
				"loc": "x{}y{}".format(x,y),
				"val": tile_val
			}
			tiles[y][x] = current_tile

		empty,out  = decode(fp, [])
		print_decrypt(out)
		assert len(empty)==0
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

	if "tilegrid" in file_paths.keys():
		with open(file_paths["tilegrid"], "wt") as fout:
			json.dump(tiles, fout, indent=4)

if __name__ == '__main__':
	decode_chipdb(sys.argv)