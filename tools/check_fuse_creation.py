#!/usr/bin/env python3
"""
For each architecture and part, decrypt all data and save it to raw file
"""

import argparse, sys, os
import database
import unlogic

def extract_elements(bitfile, infile):
	bits = dict()
	with open(bitfile, "rt") as fin:		
		for line in fin:
			line = line.rstrip()			
			row = line.split(':')[1].split(' ')[0]
			col = line.split(':')[2]
			if (not row in bits):
				bits[row] = dict()
			bits[row][col] = 1


	prefix = ""
	with open(infile, "rt") as fin:		
		for line in fin:
			line = line.rstrip()
			if (line.startswith('//')):
				inst = line.split(':')[1].split(',')[0]
				prefix = inst
			elif (line.startswith('###')):
				prefix = ""
			elif (not line==""):
				t = unlogic.inst[prefix]
				b = unlogic.bcc_info[t["type"]]
				bit = b[line]
				if t["flag"]==0:
					t = unlogic.info[t["y"]+bit["yoff"]][t["x"]+bit["xoff"]][bit["type"].lower()]
				row = t["wl_beg"]+bit["y"]
				col = t["bl_beg"]+bit["x"]
				if (col >=974):
					col += 6				
				if (col >=2920+6):
					col += 6
				status = "FAILED"
				if (str(row) in bits and str(col) in bits[str(row)]):
					bits[str(row)].pop(str(col))
					status = "OK"
					
				print("{}/{},{} flag:{} row:{} col:{} {}".format(prefix,line,t["type"],t["flag"],row, col, status))

	
def main(argv):
	parser = argparse.ArgumentParser(prog="check_fuse_creation",
		usage="%(prog)s [options] <log_file>.log")
	parser.add_argument("bits_file", metavar="<log_file>.bits", nargs="?",
		help="Anlogic bitstream generated .bits file")
	parser.add_argument("log_file", metavar="<log_file>.log", nargs="?",
		help="Anlogic bitstream generated .log file")
	parser.add_argument('part', type=str,
		help="FPGA part (e.g. eagle_s20)")
	args = parser.parse_args(argv[1:])	
	log_file = args.log_file
	bits_file = args.bits_file

	if log_file is None or bits_file is None :
		parser.print_help()
		sys.exit()

	if not os.path.isfile(log_file):
		print("File path {} does not exist...".format(log_file))
		sys.exit()
	if not os.path.isfile(bits_file):
		print("File path {} does not exist...".format(bits_file))
		sys.exit()

	tang_root = database.get_tangdinasty_root()
	chipdb = os.path.join(tang_root, "arch", args.part + ".db") 
	unlogic.decode_chipdb(["decrypt_all", chipdb, "--datadir", os.path.join("work_decrypt",args.part)])
	print()
	extract_elements(bits_file,log_file)

if __name__ == '__main__':
	main(sys.argv)