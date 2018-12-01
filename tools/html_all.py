#!/usr/bin/env python3
"""
For each architecture and part, create tilegrid html
"""

import os
from os import path

import database
import json
import html_tilegrid
import shutil

def main():
	devices = database.get_devices()
	shutil.rmtree("work_html", ignore_errors=True)
	os.mkdir("work_html")
	for architecture in devices["architectures"].keys():
		for part in devices["architectures"][architecture]["parts"].keys():
			html_tilegrid.main(["html_tilegrid", architecture,  part, path.join("work_html",part + ".html")])
if __name__ == "__main__":
	main()