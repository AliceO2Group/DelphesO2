#!/usr/bin/env python3

"""
Inspector of the LUT file.
"""

from ROOT import gInterpreter
import argparse

def main(file_name):
    headers = """
        #include "lutCovm.hh"
        #include <iostream>
        #include <fstream>
    """

    gInterpreter.ProcessLine(headers)

    gInterpreter.ProcessLine(f"const char* filename = \"{file_name}\";")
    printer = """
        ifstream lutFile(filename, std::ofstream::binary);
        lutHeader_t lutHeader;
        lutFile.read(reinterpret_cast<char*>(&lutHeader), sizeof(lutHeader));
        lutHeader.print();
    """
    gInterpreter.ProcessLine(printer)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_file", type=str,
                        help="Name of the input file.")
    args = parser.parse_args()
    main(args.input_file)
