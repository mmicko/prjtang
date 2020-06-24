#!/usr/bin/env python3
"""
For each architecture and part, decrypt all data and save it to raw file
"""

import os
from os import path
import shutil

import database
import json
import unlogic

def main():
	devices = database.get_devices()
	tang_root = database.get_tangdinasty_root()
	shutil.rmtree("work_decrypt", ignore_errors=True)
	os.mkdir("work_decrypt")
	print("Decrypting all chipdb files...")
	for architecture in devices["architectures"].keys():
		print("Architecture: " + architecture)
		for part in devices["architectures"][architecture]["parts"].keys():
			print("Part: " + part)
			selected_part = devices["architectures"][architecture]["parts"][part]
			package = selected_part["packages"][0]
			os.mkdir(path.join("work_decrypt",part))
			unc_file = path.join("work_decrypt", part + ".unc")
			chipdb = path.join(tang_root, "arch", part + ".db") 
			unlogic.decode_chipdb(["decrypt_all", chipdb, "--decrypt", unc_file, "--datadir", path.join("work_decrypt",part)])

if __name__ == "__main__":
	main()