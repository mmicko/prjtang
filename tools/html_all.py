#!/usr/bin/env python3
"""
For each architecture and part, create tilegrid html
"""

import os, time
from os import path
from string import Template
import database
import json
import html_tilegrid
import shutil

tang_docs_index = """
<html>
<head>
<title>Project Tang HTML Documentation</title>
</head>
<body>
<h1>Project Tang HTML Documentation</h1>
<p>Project Tang is a project to document the Anlogic bitstream and internal architecture.</p>
<p>This repository contains HTML documentation automatically generated from the
<a href="https://github.com/mmicko/prjtang">Project Tang</a> database.
Data generated includes tilemap data and bitstream data for many tile types. Click on any tile to see its bitstream
documentation.
</p>

<p>This HTML documentation was generated at ${datetime} from prjtang commit
<a href="https://github.com/mmicko/prjtang/tree/${commit}">${commit}</a>.</p>
<hr/>
$docs_toc
<hr/>
<p>Licensed under a very permissive <a href="COPYING">CC0 1.0 Universal</a> license.</p>
</body>
</html>
"""

def main():
	devices = database.get_devices()
	shutil.rmtree("work_html", ignore_errors=True)
	os.mkdir("work_html")
	commit_hash = database.get_db_commit()
	build_dt = time.strftime('%Y-%m-%d %H:%M:%S')

	docs_toc = ""
	for architecture in sorted(devices["architectures"].keys()):
		docs_toc += "<h3>{} Architecture</h3>".format(architecture.upper())
		docs_toc += "<h4>Bitstream Documentation</h4>"
		docs_toc += "<ul>"
		for part in sorted(devices["architectures"][architecture]["parts"].keys()):
			docs_toc += '<li><a href="{}">{} Documentation</a></li>'.format('{}.html'.format(part),part.upper())
			html_tilegrid.main(["html_tilegrid", architecture,  part, path.join("work_html",part + ".html")])
		docs_toc += "</ul>"
	index_html = Template(tang_docs_index).substitute(
		datetime=build_dt,
		commit=commit_hash,
		docs_toc=docs_toc
	)
	with open(path.join("work_html", "index.html"), 'w') as f:
		f.write(index_html)
if __name__ == "__main__":
	main()