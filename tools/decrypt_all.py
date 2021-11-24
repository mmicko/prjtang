#!/usr/bin/env python3
"""
For each family and device, decrypt all data and save it to raw file
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
	for family in devices["families"].keys():
		print("Family: " + family)
		for device in devices["families"][family]["devices"].keys():
			print("Device: " + device)
			selected_device = devices["families"][family]["devices"][device]
			package = selected_device["packages"][0]
			os.mkdir(path.join("work_decrypt",device))
			unc_file = path.join("work_decrypt", device + ".unc")
			chipdb = path.join(tang_root, "arch", device + ".db") 
			unlogic.decode_chipdb(["decrypt_all", chipdb, "--decrypt", unc_file, "--datadir", path.join("work_decrypt",device)])

if __name__ == "__main__":
	main()