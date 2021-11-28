#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <streambuf>
#include "version.hpp"
#include "wasmexcept.hpp"
#include "Bitstream.hpp"
#include "Chip.hpp"
#include "Database.hpp"
#include "DatabasePath.hpp"

using namespace std;
using namespace Tang;

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;
    
    std::string database_folder = get_database_path();

    po::options_description options("Allowed options");
    options.add_options()("help,h", "show help");
    options.add_options()("bit", po::value<std::string>(), "output bit file");
    options.add_options()("bin", po::value<std::string>(), "output bin file");
    options.add_options()("fuse", po::value<std::string>(), "output fuse file");
    options.add_options()("bas", po::value<std::string>(), "output bas file");
    options.add_options()("bmk", po::value<std::string>(), "output bmk file");
    options.add_options()("bma", po::value<std::string>(), "output bma file");
    options.add_options()("svf", po::value<std::string>(), "output svf file");
    options.add_options()("rbf", po::value<std::string>(), "output rbf file");
    options.add_options()("db", po::value<std::string>(), "Tang database folder location");

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
        cerr << "Version " << git_describe_str << endl;
        cerr << argv[0] << ": Anlogic bitstream converter" << endl;
        cerr << endl;
        cerr << "Copyright (C) 2021 Miodrag Milanovic <mmicko@gmail.com>" << endl;
        cerr << endl;
        cerr << options << endl;
        return vm.count("help") ? 0 : 1;
    }

    ifstream bitstream_file(vm["input"].as<string>());
    if (!bitstream_file) {
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

    try {
        Bitstream bitstream = Bitstream::read(bitstream_file);
        Chip c = bitstream.deserialise_chip();
        if (vm.count("fuse")) {
            ofstream output_stream(vm["fuse"].as<string>(), ios::out | ios::trunc);
            Bitstream::write_fuse(c, output_stream);
        }
        if (vm.count("bit")) {
            ofstream output_stream(vm["bit"].as<string>(), ios::out | ios::trunc | ios::binary);
            bitstream.write_bit(output_stream);
        }
        if (vm.count("bin")) {
            ofstream output_stream(vm["bin"].as<string>(), ios::out | ios::trunc | ios::binary);
            bitstream.write_bin(output_stream);
        }
        if (vm.count("bas")) {
            ofstream output_stream(vm["bas"].as<string>(), ios::out | ios::trunc);
            bitstream.write_bas(output_stream);
        }
        if (vm.count("bma")) {
            ofstream output_stream(vm["bma"].as<string>(), ios::out | ios::trunc);
            bitstream.write_bma(c, output_stream);
        }
        if (vm.count("bmk")) {
            ofstream output_stream(vm["bmk"].as<string>(), ios::out | ios::trunc | ios::binary);
            bitstream.write_bmk(c, output_stream);
        }
        if (vm.count("rbf")) {
            ofstream output_stream(vm["rbf"].as<string>(), ios::out | ios::trunc | ios::binary);
            bitstream.write_rbf(output_stream);
        }
        if (vm.count("svf")) {
            ofstream output_stream(vm["svf"].as<string>(), ios::out | ios::trunc);
            bitstream.write_svf(c, output_stream);
        }
    } catch (BitstreamParseError &e) {
        cerr << e.what() << endl;
    }
    return 0;
}
