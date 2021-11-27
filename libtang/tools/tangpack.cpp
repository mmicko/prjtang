#include "ChipConfig.hpp"
#include "Bitstream.hpp"
#include "Chip.hpp"
#include "Database.hpp"
#include "DatabasePath.hpp"
#include "Tile.hpp"
#include "BitDatabase.hpp"
#include "version.hpp"
#include "wasmexcept.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include <stdexcept>
#include <streambuf>
#include <fstream>
#include <iomanip>

using namespace std;

int main(int argc, char *argv[])
{
    using namespace Tang;
    namespace po = boost::program_options;

    std::string database_folder = get_database_path();

    po::options_description options("Allowed options");
    options.add_options()("help,h", "show help");
    options.add_options()("verbose,v", "verbose output");
    options.add_options()("db", po::value<std::string>(), "Tang database folder location");
    options.add_options()("usercode", po::value<uint32_t>(), "USERCODE to set in bitstream");
    po::positional_options_description pos;
    options.add_options()("input", po::value<std::string>()->required(), "input textual configuration");
    pos.add("input", 1);
    options.add_options()("bit", po::value<std::string>(), "output bitstream file");
    pos.add("bit", 1);

    po::variables_map vm;

    try {
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(options).positional(pos).run();
        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (po::required_option& e) {
        cerr << "Error: input file is mandatory." << endl << endl;
        goto help;
    }
    catch (std::exception& e) {
        cerr << "Error: " << e.what() << endl << endl;
        goto help;
    }

    if (vm.count("help")) {
help:
        cerr << "Project Tang - Open Source Tools for Anlogic FPGAs" << endl;
        cerr << "Version " << git_describe_str << endl;
        cerr << argv[0] << ": Anlogic bitstream packer" << endl;
        cerr << endl;
        cerr << "Copyright (C) 2021 Miodrag Milanovic <mmicko@gmail.com>" << endl;
        cerr << endl;
        cerr << "Usage: " << argv[0] << " input.config [output.bit] [options]" << endl;
        cerr << options << endl;
        return vm.count("help") ? 0 : 1;
    }

    ifstream config_file(vm["input"].as<string>());
    if (!config_file) {
        cerr << "Failed to open input file" << endl;
        return 1;
    }

    if (vm.count("db")) {
        database_folder = vm["db"].as<string>();
    }

    try {
        load_database(database_folder);
    } catch (runtime_error &e) {
        cerr << "Failed to load Tang database: " << e.what() << endl;
        return 1;
    }

    string textcfg((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());

    ChipConfig cc;
    try {
        cc = ChipConfig::from_string(textcfg);
    } catch (runtime_error &e) {
        cerr << "Failed to process input config: " << e.what() << endl;
        return 1;
    }

    Chip c = cc.to_chip();
    if (vm.count("usercode"))
        c.usercode = vm["usercode"].as<uint32_t>();

    map<string, string> bitopts;

    Bitstream b = Bitstream::serialise_chip(c, bitopts);
    if (vm.count("bit")) {
        ofstream bit_file(vm["bit"].as<string>(), ios::binary);
        if (!bit_file) {
            cerr << "Failed to open output file" << endl;
            return 1;
        }
        b.write_bit(bit_file);
    }

    return 0;
}
