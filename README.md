# Project Tang
 
## For FPGA Toolchain Users

Project Tang enables a fully open-source flow for Anlogic FPGAs using [Yosys](https://github.com/YosysHQ/yosys)
for Verilog synthesis and [nextpnr](https://github.com/YosysHQ/nextpnr) for place and route.
Project Tang itself provides the device database and tools for bitstream creation.

### Getting Started

### Current Status
 
### Development Boards

## For Developers

This repository contains both tools and scripts which allow you to document the
bit-stream format of Anlogic series FPGAs.

Translation of offical documents can be found [here](https://github.com/kprasadvnsi/Anlogic_Doc_English).

### Quickstart Guide

Take latest TD distribution from [Sipeed.com](https://mega.nz/folder/5AAiSBwB#0jbOgqcrIMJVU9FvCzqb0A/folder/ZY50yRhI)
Point out TD_HOME to your TangDinasty installation and set environment.

```
export TD_HOME=/opt/TD
source environment.sh
```

## Process

To create chip database run:

```
python3 create_database.py
```

In order to get HTML representation of tilegrid data after run:

```
python3 html_all.py
```

### Parts

#### [Tools](tools)

Miscellaneous tools for exploring the database and experimenting with bitstreams.

#### [Util](util)

Python libraries used for fuzzers and other purposes

### Database

Running the all fuzzers in order will produce a database which documents the
bitstream format in the [database](database) directory.

## Credits

Code is heavily based on [prjtrellis](https://github.com/YosysHQ/prjtrellis). Special thanks goes to [@gatecat](https://github.com/gatecat)
## Contributing

There are a couple of guidelines when contributing to Project Tang which are
listed here.

### Sending

All contributions should be sent as
[GitHub Pull requests](https://help.github.com/articles/creating-a-pull-request-from-a-fork/).

### License

All code in the Project Tang repository is licensed under the very permissive
[ISC Licence](COPYING). A copy can be found in the [`COPYING`](COPYING) file.

All new contributions must also be released under this license.

### Code of Conduct

By contributing you agree to the [code of conduct](CODE_OF_CONDUCT.md). We
follow the open source best practice of using the [Contributor
Covenant](https://www.contributor-covenant.org/) for our Code of Conduct.
