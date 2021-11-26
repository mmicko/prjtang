#!/usr/bin/env python3
"""
For each family and device, obtain a tilegrid and save it in the database
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
	if not path.exists(database.get_db_root()):
		os.mkdir(database.get_db_root())
	shutil.rmtree("work_decrypt", ignore_errors=True)
	os.mkdir("work_decrypt")

	shutil.copy(path.join(database.get_tang_root(), "devices.json"), path.join(database.get_db_root(), "devices.json"))

	for family in devices["families"].keys():
		print("Family: " + family)
		for device in devices["families"][family]["devices"].keys():
			print("Device: " + device)
			selected_device = devices["families"][family]["devices"][device]
			package = selected_device["packages"][0]
			os.mkdir(path.join("work_decrypt",device))

			json_file = path.join(database.get_db_subdir(family, device), "tilegrid.json")
			chipdb = path.join(tang_root, "arch", device + ".db") 
			unlogic.decode_chipdb(["create_database", chipdb, "--tilegrid", json_file, "--datadir", path.join("work_decrypt",device)])

if __name__ == "__main__":
	main()