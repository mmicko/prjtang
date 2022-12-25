#!/usr/bin/env python3

"""Generate a HTML representation of the tile bit database"""

import database
import argparse
import sys
import re
import unlogic
import os

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('family', type=str,
                    help="FPGA family (e.g. eg4)")
parser.add_argument('device', type=str,
                    help="FPGA device (e.g. eagle_s20)")
parser.add_argument('outfile', type=argparse.FileType('w'),
                    help="output HTML file")


colours = {"A": "#88FFFF", "B": "#FF88FF", "C": "#8888FF", "D": "#FFFF88", "M": "#FFBBBB", "H": "#BBBBFF",
           "V": "#FFFFBB"}

def bit_grid_html(device_info, f):
    w = int(device_info["bits_per_frame"])
    h = int(device_info["frames"])
    max_row = int(device_info["max_row"])
    max_col = int(device_info["max_col"])
    
    fuses = [[0 for x in range(w)] for y in range(h)] 
    for x in range(max_col):
        for y in range(max_row):
            for k,v in unlogic.info[y][x].items():
                if (v["type"] in unlogic.bcc_info.keys()):
                    b = unlogic.bcc_info[v["type"]]
                    for bit in b:
                        start_frame = int(v["start_frame"])
                        start_bit = int(v["start_bit"])

                        if int(bit["remap"])==1:
                            new_v = unlogic.info[v["y"]+bit["yoff"]][v["x"]+bit["xoff"]][bit["type"].lower()]
                            start_frame = int(new_v["start_frame"])
                            start_bit = int(new_v["start_bit"])

                        if int(bit["pll_info"])==0:
                            row = start_frame + bit["frame"]
                            col = start_bit + bit["bit"]
                            #if (col >=974):
                            #    col += 6
                            #if (col >=2920+6):
                            #    col += 6

                            fuses[row][col]+=1

    for bit in range(h):
        for frame in range(w):
            print(fuses[bit][frame], end = '', file=f)
        print("", file=f)

def main(argv):
    args = parser.parse_args(argv[1:])
    tilegrid = database.get_tilegrid(args.family, args.device)
    device_info = database.get_devices()["families"][args.family]["devices"][args.device]

    tang_root = database.get_tangdinasty_root()
    chipdb = os.path.join(tang_root, "arch", args.device + ".db") 
    unlogic.decode_chipdb(["decrypt_all", chipdb])
    f = args.outfile
    bit_grid_html(device_info, f)


if __name__ == "__main__":
    main(sys.argv)
