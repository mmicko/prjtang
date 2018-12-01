#!/usr/bin/env python3
"""
Convert the tile grid for a given family and device to HTML format
"""
import sys, re
import argparse
import database

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('architecture', type=str,
					help="FPGA architecture (e.g. ECP5)")
parser.add_argument('part', type=str,
					help="FPGA part (e.g. LFE5U-85F)")
parser.add_argument('outfile', type=argparse.FileType('w'),
					help="output HTML file")


def get_colour(ttype):
    colour = "#FFFFFF"
    if ttype.startswith("iol_"):
        colour = "#88FF88"
    elif ttype.startswith("miscs_mic_io"):
        colour = "#88FFFF"
    elif ttype.startswith("gclk"):
        colour = "#FF8888"
    elif ttype.startswith("plb"):
        colour = "#8888FF"
    elif ttype.startswith("DUMMY"):
        colour = "#FFFFFF"
    elif ttype.startswith("gclk") or ttype.startswith("gclk_iospine"):
        colour = "#FF88FF"
    elif ttype.find("DSP") != -1:
        colour = "#FFFF88"
    elif ttype.find("pib") != -1:
        colour = "#DDDDDD"
    else:
        colour = "#888888"
    return colour


def main(argv):
	args = parser.parse_args(argv[1:])
	tilegrid = database.get_tilegrid(args.architecture, args.part)
	device_info = database.get_devices()["architectures"][args.architecture]["parts"][args.part]

	max_row = device_info["max_row"]
	max_col = device_info["max_col"]
	tiles = []
	
	for i in range(max_col ):
		row = []
		for j in range(max_row):
			row.append([])
		tiles.append(row)

	for row in tilegrid:
		for item in row:
			x = int(item["x"])
			y = int(item["y"])
			for data in item["val"]:
				colour = get_colour(data["type"])
				tiles[y][x].append((data["inst"], data["type"], colour))

	f = args.outfile
	print(
		"""<html>
			<head><title>{} Tiles</title></head>
			<body>
			<h1>{} Tilegrid</h1>
			<table style='font-size: 8pt; border: 2px solid black; text-align: center'>
		""".format(args.part.upper(), args.part.upper()), file=f)
	for trow in tiles:
		print("<tr>", file=f)
		row_max_height = 0
		for tloc in trow:
			row_max_height = max(row_max_height, len(tloc))
		row_height = max(75, 30 * row_max_height)
		for tloc in trow:
			print("<td style='border: 2px solid black; height: {}px'>".format(row_height), file=f)
			for tile in tloc:
				print(
					"<div style='height: {}%; background-color: {}'><em>{}</em><br/><strong>{}</strong></div>".format(
						100 / len(tloc), tile[2], tile[0], tile[1], tile[1]), file=f)
			print("</td>", file=f)
		print("</tr>", file=f)
	print("</table></body></html>", file=f)


if __name__ == "__main__":
	main(sys.argv)
