#!/usr/bin/env python3

"""
Script to check if an AOD is sane and can be used for analysis or not
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

from common import get_default_parser, msg, print_all_warnings, run_in_parallel, verbose_msg, warning_msg, set_verbose_mode
from ROOT import TFile
import multiprocessing
import os

bad_files = multiprocessing.Manager().dict()


def main(input_file_name="/tmp/AO2D.root", verbose=False):
    global bad_files
    verbose_msg("Checking file", input_file_name)
    input_file = TFile(input_file_name, "READ")
    if verbose:
        input_file.ls()
    list_of_keys = input_file.GetListOfKeys()

    def inspect(name, tree_name):
        tree_name = f"{name}/{tree_name}"
        t = input_file.Get(tree_name)
        if not t:
            warning_msg("Did not get tree", tree_name)
            return -1
        if verbose:
            input_file.Get(name).ls()
        verbose_msg(tree_name, t.GetEntries())
        return t.GetEntries()

    for df_index, i in enumerate(list_of_keys):
        if i.GetName() == "metaData":
            continue

        def add_bad():
            # print(i.GetName())
            bad_files.setdefault(input_file_name, []).append(i.GetName())

        dictionary_of_counts = {"O2bc": None,
                                "O2collision": None,
                                "O2track": None,
                                "O2trackcov": None,
                                "O2trackextra": None}
        for j in dictionary_of_counts:
            dictionary_of_counts[j] = inspect(i.GetName(), j)
            if dictionary_of_counts[j] < 0:
                add_bad()

        def must_be_same(*args):
            counts = []
            names = []
            for k in args:
                counts.append(dictionary_of_counts[k])
                names.append(k)
            if len(set(counts)) != 1:
                add_bad()
                warning_msg("Did not get equal counts for", ", ".join(names),
                            counts, "in DF", df_index, "/", len(list_of_keys), ":", i.GetName())
        must_be_same("O2track", "O2trackcov", "O2trackextra")


if __name__ == "__main__":
    parser = get_default_parser(description=__doc__)
    parser.add_argument("input_files",
                        type=str,
                        nargs="+",
                        help="Input files to check")
    parser.add_argument("--output", "-o",
                        type=str,
                        default=None,
                        help="Output file with good files only")
    args = parser.parse_args()
    set_verbose_mode(args)

    input_files = []
    for i in args.input_files:
        i = os.path.normpath(i)
        if i.endswith(".root"):
            input_files.append(i)
        elif i.endswith(".txt"):
            with open(i, "r") as f:
                for j in f:
                    j = j.strip()
                    input_files.append(os.path.join(os.path.abspath(os.path.dirname(i)),
                                                    os.path.normpath(j)))

    run_in_parallel(args.njobs, main, input_files,
                    "Checking file", linearize_single_core=True)
    if len(bad_files) > 0:
        warning_msg("There were", len(bad_files), "bad files")
        for i in bad_files:
            msg(i)

    if args.output is not None:
        msg("Writing good files to", args.output)
        with open(args.output, "w") as f:
            for i in input_files:
                if not i in bad_files:
                    f.write(i+"\n")
