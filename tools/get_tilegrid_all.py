#!/usr/bin/env python3
"""
For each architecture and part, obtain a tilegrid and save it in the database
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
	for architecture in devices["architectures"].keys():
		print("Architecture: " + architecture)
		for part in devices["architectures"][architecture]["parts"].keys():
			print("Part: " + part)
			selected_part = devices["architectures"][architecture]["parts"][part]
			package = selected_part["packages"][0]

			json_file = path.join(database.get_db_subdir(architecture, part), "tilegrid.json")
			chipdb = path.join(tang_root, "arch", part + ".db") 
			unlogic.decode_chipdb(["get_tilegrid_all", chipdb, "--tilegrid", json_file, "--datadir", path.join("work_decrypt",part)])

if __name__ == "__main__":
	main()