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
	for family in devices["families"].keys():
		print("Family: " + family)
		for device in devices["families"][family]["devices"].keys():
			print("Device: " + device)
			selected_device = devices["families"][family]["devices"][device]
			package = selected_device["packages"][0]

			json_file = path.join(database.get_db_subdir(family, device), "tilegrid.json")
			chipdb = path.join(tang_root, "arch", device + ".db") 
			unlogic.decode_chipdb(["get_tilegrid_all", chipdb, "--tilegrid", json_file, "--datadir", path.join("work_decrypt",device)])

if __name__ == "__main__":
	main()