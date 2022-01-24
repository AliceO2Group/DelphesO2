#!/usr/bin/env python3


"""
Script to test the indices of the AODs
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

from ROOT import TFile, TCanvas, TH1F
import ROOT
import argparse

ROOT.gInterpreter.Declare("""
    int Count(int x) {
        if(x >= 998)
            return 1;
        return 0;
    }
    """)


def main(input_name, draw=True, vvv=False, tree_names=None):
    f = TFile(input_name, "READ")
    # f.ls()
    lk = f.GetListOfKeys()

    histograms = {}

    def gethisto(k, n=len(lk), l=0, h=len(lk), y="", set_labels=True):
        if histograms.setdefault(k, None) is None:
            h = TH1F(k, k, n, l, h)
            h.GetYaxis().SetTitle(y)
            h.SetDirectory(0)
            if set_labels:
                for i in enumerate(lk):
                    h.GetXaxis().SetBinLabel(i[0]+1, i[1].GetName())
            histograms[k] = h
        return histograms[k]
    empty_dirs = {}

    def check_dir(directory, verbose=False):
        if verbose:
            print(directory, type(directory))
            directory.ls()
        available_trees = ["O2bc", "O2track", "O2collision", "O2mccollision",
                           "O2mcparticle", "O2mctracklabel", "O2mccollisionlabel"]
        if tree_names is not None:
            to_remove = []
            for i in available_trees:
                if i not in tree_names:
                    to_remove.append(i)
            for i in to_remove:
                available_trees.pop(available_trees.index(i))
        for i in available_trees:
            tree = directory.Get(i)
            if verbose:
                print(i, ":")
                tree.Print()

        def check_tree(tn):
            df = ROOT.RDataFrame(directory.GetName()+"/"+tn, input_name)

            def fill_histo(v, y, tag=""):
                h = gethisto(tn+tag, y=y)
                h.Fill(directory.GetName(), v)
                return v

            if df.Count().GetValue() < 1:
                print(directory, "in", input_name, "has",
                      df.Count().GetValue(), f"entries in '{tn}'")
                empty_dirs.setdefault(tn, []).append(
                    f"{input_name}:{directory.GetName()}")
            if tn == "O2track":
                print("In", directory, tn, "has",
                      fill_histo(df.Count().GetValue(), "#Tracks", "amount"),
                      "tracks")
                # df = df.Define("HowMany", "Count(fIndexCollisions)").Sum(
                #     "HowMany").GetValue()
                # print("HowMany:", df)
            elif tn == "O2mctracklabel":
                print("In", directory, tn, "has", df.Count().GetValue(),
                      "mc track labels with mean",
                      fill_histo(df.Mean("fIndexMcParticles").GetValue(), "<fIndexMcParticles>"))
            elif tn == "O2mccollisionlabel":
                print("In", directory, tn, "has", df.Count().GetValue(),
                      "mc collision labels with mean",
                      fill_histo(df.Mean("fIndexMcCollisions").GetValue(), "<fIndexMcCollisions>"))
            elif tn == "O2mcparticle":
                print("In", directory, tn, "has",
                      fill_histo(df.Count().GetValue(), "#Particles"),
                      "particles")

        # check_tree("O2track")
        # check_tree("O2mcparticle")
        # check_tree("O2mctracklabel")
        # check_tree("O2mccollisionlabel")
        for i in available_trees:
            check_tree(i)

    for i in lk:
        d = f.Get(i.GetName())
        if "TDirectoryFile" not in d.ClassName():
            continue
        check_dir(d, verbose=(i == lk[0] and vvv))

    if draw:
        for i in histograms:
            can = TCanvas()
            histograms[i].Draw("HIST")
            can.Update()
            input("Press enter to continue")
    return empty_dirs


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__name__)
    parser.add_argument("--verbose", "-v",
                        action="store_true", help="Verbose mode.")
    parser.add_argument("input_file",
                        type=str,
                        nargs="+",
                        help="Input files")
    parser.add_argument("--trees", "-t",
                        type=str,
                        nargs="+",
                        default=None,
                        help="Name of the trees")

    args = parser.parse_args()

    results = [main(i, tree_names=args.trees) for i in args.input_file]
    empty_dirs = {}
    for i in results:
        for j in i:
            empty_dirs.setdefault(j, []).append(i[j])
    for i in empty_dirs:
        print("\t-", i, empty_dirs[i], "\n")
