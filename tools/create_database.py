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
	shutil.rmtree(database.get_db_root(), ignore_errors=True)
	os.mkdir(database.get_db_root())

	shutil.copy(path.join(database.get_tang_root(), "devices.json"), path.join(database.get_db_root(), "devices.json"))

	for family in devices["families"].keys():
		print("Family: " + family)
		for device in devices["families"][family]["devices"].keys():
			print("Device: " + device)
			selected_device = devices["families"][family]["devices"][device]

			database_dir = database.get_db_subdir(family, device)
			if not path.exists(path.join(database_dir,"bits")):
				os.mkdir(path.join(database_dir,"bits"))
			chipdb = path.join(tang_root, "arch", device + ".db")
			unlogic.decode_chipdb(["create_database", chipdb, "--db_dir", database_dir])

if __name__ == "__main__":
	main()