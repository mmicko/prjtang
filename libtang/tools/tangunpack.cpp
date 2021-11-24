#include "Bitstream.hpp"
#include "Chip.hpp"
#include "Database.hpp"
#include "DatabasePath.hpp"
#include "version.hpp"
#include "wasmexcept.hpp"
#include <iostream>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <stdexcept>
#include <streambuf>
#include <fstream>

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
    po::positional_options_description pos;
    options.add_options()("input", po::value<std::string>()->required(), "input bitstream file");
    pos.add("input", 1);
    options.add_options()("textcfg", po::value<std::string>()->required(), "output textual configuration");
    pos.add("textcfg", 1);

    po::variables_map vm;
    try {
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(options).positional(pos).run();
        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (po::required_option &e) {
        cerr << "Error: input file is mandatory." << endl << endl;
        goto help;
    }
    catch (std::exception &e) {
        cerr << "Error: " << e.what() << endl << endl;
        goto help;
    }

    if (vm.count("help")) {
help:
        cerr << "Project Tang - Open Source Tools for Anlogic FPGAs" << endl;
        cerr << "Version " << git_describe_str << endl;
        cerr << argv[0] << ": Anlogic bitstream to text config converter" << endl;
        cerr << endl;
        cerr << "Copyright (C) 2021 Miodrag Milanovic <mmicko@gmail.com>" << endl;
        cerr << endl;
        cerr << "Usage: " << argv[0] << " input.bit [output.config] [options]" << endl;
        cerr << options << endl;
        return vm.count("help") ? 0 : 1;
    }

    ifstream bit_file(vm["input"].as<string>(), ios::binary);
    if (!bit_file) {
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

    //ChipInfo info = get_chip_info(find_device_by_name("eagle_s20"));
}
