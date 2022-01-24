#!/usr/bin/env python3

"""
Script to compare two AOD files.
This can be used to compare the consistency of the branches of AODs 
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

from ROOT import TFile
import argparse
from sys import argv


def main(file1, file2, dir1, dir2):
    print("Comparing content of", file1, file2,
          "the first file is considered the reference")

    def get_file(f, d, verbose=False):
        f = TFile(f, "READ")
        if not f.IsOpen():
            raise RuntimeError("File", f, "is not open!")
        if d is None:
            f.ls()
        else:
            if verbose:
                f.Get(d).ls()
            if not f.Get(d):
                f.ls()
                raise RuntimeError("file", f, "does not have directory", d)
            l = f.Get(d).GetListOfKeys()
            t = {}
            for i in l:
                i = i.GetName()
                t[i] = f.Get(f"{d}/{i}")
            return f, t

    tree1 = get_file(file1, dir1)

    missing_trees = {}
    tree2 = get_file(file2, dir2)
    for i in tree2[1]:
        missing_trees[i] = tree2[1][i]

    for i in tree1[1]:
        print("Checking", f"'{i}'")
        if i not in tree2[1]:
            print(i, "not present in", file2)
            continue
        missing_trees.pop(i)
        branches1 = tree1[1][i].GetListOfBranches()
        branches2 = tree2[1][i].GetListOfBranches()
        missing_branches = {}
        for k in branches2:
            missing_branches[k] = [k.GetName(), k.GetTitle()]
        for j in branches1:
            print("  > Branch", j.GetName(), j.GetTitle())
            has_it = False
            for k in branches2:
                if (j.GetName() == k.GetName()) and (j.GetTitle() == k.GetTitle()):
                    has_it = True
                    missing_branches.pop(k)
                    break
            if not has_it:
                print("  *** Branch", j, "is not in", file2, "***")
        if len(missing_branches) > 0:
            print("!!!!!!! Missing branches that are in", file2,
                  "but not in", file1, "are:", missing_branches)
        print(i, "is consistent")

    if len(missing_trees) > 0:
        print("!!!!!!! Missing Trees that are in", file2,
              "but not in", file1, ":")
        for i in missing_trees:
            print(missing_trees[i])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file1", type=str, help="First file to check")
    parser.add_argument("file2", type=str, help="Second file to check")
    parser.add_argument("--dir1", type=str, default=None,
                        help="First directory to check")
    parser.add_argument("--dir2", type=str, default=None,
                        help="Second directory to check")
    parser.add_argument("-b", action="store_true", help="Background mode")
    parser.add_argument("-v", action="store_true", help="Verbose mode")
    args = parser.parse_args()
    main(args.file1, args.file2, args.dir1, args.dir2)
