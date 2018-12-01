"""
Database and Database Path Management
"""
import os
from os import path
import json
import subprocess


def get_tang_root():
    """Return the absolute path to the Project Tang repo root"""
    return path.abspath(path.join(__file__, "../../../"))


def get_db_root():
    """
    Return the path containing the Project Tang database
    This is database/ in the repo, unless the `PRJTANG_DB` environment
    variable is set to another value.
    """
    if "PRJTANG_DB" in os.environ and os.environ["PRJTANG_DB"] != "":
        return os.environ["PRJTANG_DB"]
    else:
        return path.join(get_tang_root(), "database")


def get_db_subdir(architecture = None, part = None, package = None):
    """
    Return the DB subdirectory corresponding to a architecture, part and
    package (all if applicable), creating it if it doesn't already
    exist.
    """
    subdir = get_db_root()
    if not path.exists(subdir):
        os.mkdir(subdir)
    dparts = [architecture, part, package]
    for dpart in dparts:
        if dpart is None:
            break
        subdir = path.join(subdir, dpart)
        if not path.exists(subdir):
            os.mkdir(subdir)
    return subdir


def get_tilegrid(architecture, part):
    """
    Return the deserialised tilegrid for a architecture, part
    """
    tgjson = path.join(get_db_subdir(architecture, part), "tilegrid.json")
    with open(tgjson, "r") as f:
        return json.load(f)


def get_devices():
    """
    Return the deserialised content of devices.json
    """
    djson = path.join(get_tang_root(), "devices.json")
    with open(djson, "r") as f:
        return json.load(f)

def get_db_commit():
    return subprocess.getoutput('git -C "{}" rev-parse HEAD'.format(get_db_root()))
