#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <streambuf>

#include "Bitstream.hpp"

using namespace std;
using namespace Tang;

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;

    po::options_description options("Allowed options");
    options.add_options()("help,h", "show help");
    options.add_options()("bin", po::value<std::string>(), "output bin file");
    options.add_options()("bas", po::value<std::string>(), "output bas file");
    options.add_options()("fuse", po::value<std::string>(), "output fuse file");

    po::positional_options_description pos;
    options.add_options()("input", po::value<std::string>()->required(), "input bitstream file");
    pos.add("input", 1);

    po::variables_map vm;
    try {
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(options).positional(pos).run();
        po::store(parsed, vm);
        po::notify(vm);
    } catch (std::exception &e) {
        cerr << e.what() << endl << endl;
        goto help;
    }

    if (vm.count("help")) {
    help:
        cerr << "Project Tang - Open Source Tools for Anlogic FPGAs" << endl;
        cerr << "tangbit: Anlogic bitstream converter" << endl;
        cerr << endl;
        cerr << "Copyright (C) 2018 Miodrag Milanovic <miodrag@symbioticeda.com>" << endl;
        cerr << endl;
        cerr << options << endl;
        return vm.count("help") ? 0 : 1;
    }

    ifstream bitstream_file(vm["input"].as<string>());
    if (!bitstream_file) {
        cerr << "Failed to open input file" << endl;
        return 1;
    }
    try {
        Bitstream bitstream = Bitstream::read(bitstream_file);
        bitstream.parse();
    } catch (BitstreamParseError e) {
        cerr << e.what() << endl;
    }
    return 0;
}
