#!/usr/bin/env python3

"""
Script to split the directories of a file into several ones with the same structure, useful for ML processing
"""

from multiprocessing import Pool
from ROOT import TFile
import os
import time
import argparse


g_verbose = False
g_out_path = None
g_base_dir = None
g_tag_dir = True


def split_file(input_file):
    processing_time = time.time()
    print(" > Processing file", input_file)
    if g_verbose:
        print("Omogenizing file", f"'{input_file}'", "for ML processing")
    f = TFile(input_file, "READ")
    if g_verbose:
        f.ls()
    lk = f.GetListOfKeys()
    files_created = 0
    for i in lk:
        obj = f.Get(i.GetName())
        if obj.ClassName() == "TDirectoryFile":
            in_path = os.path.dirname(input_file)
            fout_name = input_file.replace(
                ".root", f"_sub{files_created}.root")
            if g_tag_dir:
                tag = in_path.split("/")[-1]
                fout_name = input_file.replace(
                    ".root", f"_{tag}_sub{files_created}.root")
            if g_out_path is not None:
                fout_name = os.path.join(
                    g_out_path, os.path.basename(fout_name))
            if os.path.isfile(fout_name):
                raise RuntimeError("File", fout_name, "already there!")
            fout = TFile(fout_name, "RECREATE")
            if not fout.IsOpen():
                raise RuntimeError("File", fout_name, "is not open!")
            if g_verbose:
                print("Creating omogenized file to", fout)
            files_created += 1
            fout.mkdir(g_base_dir+"0")
            fout.cd(g_base_dir+"0")
            for j in obj.GetListOfKeys():
                if g_verbose:
                    print("Writing", j.ClassName(), j)
                t = obj.Get(j.GetName())
                if t.ClassName() == "TTree":
                    t.CloneTree().Write()
                else:
                    t.Clone().Write()
            if g_verbose:
                fout.ls()
            fout.Close()
    print(" < Processed file", input_file,
          "split into", files_created, "files, in", time.time() - processing_time, "seconds")


def main(input_files, verbose=True, base_dir="DF_", out_path=None, jobs=10):
    global g_verbose
    g_verbose = verbose
    global g_out_path
    g_out_path = out_path
    global g_base_dir
    g_base_dir = base_dir

    print("Omogenizing", len(input_files), "files")
    processing_time = time.time()
    with Pool(jobs) as p:
        p.map(split_file, input_files)
    print("Done, in", time.time() - processing_time, "seconds")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Omogenizer for ML processing")
    parser.add_argument("input_files",
                        type=str,
                        nargs="+",
                        help="Input files")
    parser.add_argument("--base_dir",
                        type=str,
                        default="TF_",
                        help="Name of the base directory, usually `TF_` or `DF_`")
    parser.add_argument("--out_dir", "-o",
                        type=str,
                        default=None,
                        help="Name of the output path, by default it is the same path of the input file")
    parser.add_argument("--sub_dir_tag", "-S",
                        action="store_true",
                        help="Option to tag files with their directory, useful when processing files with the same name in a path and using an output path")
    parser.add_argument("--jobs", "-j",
                        type=int,
                        default=10,
                        help="Number of parallel jobs")
    parser.add_argument("-v",
                        action="store_true", help="Verbose mode")
    args = parser.parse_args()
    main(args.input_files, verbose=args.v,
         out_path=args.out_dir,
         base_dir=args.base_dir, jobs=args.jobs)
