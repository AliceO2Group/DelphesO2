#!/usr/bin/env python3

from sys import argv
from ROOT import TFile
import ROOT


def main(input_name):
    f = TFile(input_name, "READ")
    # f.ls()
    lk = f.GetListOfKeys()

    def check_dir(directory):
        print(directory, type(directory))
        directory.ls()
        t = {}
        for i in ["O2bc", "O2track", "O2collision", "O2mccollision",
                  "O2mcparticle", "O2mctracklabel", "O2mccollisionlabel"]:
            t[i] = directory.Get(i)
        if False:
            for i in t:
                t[i].Print()
        # t["O2track"].Print()
        ROOT.gInterpreter.Declare("""
        int Count(int x) {
            if(x >= 998)
                return 1;
            return 0;
        }
        """)
        df = ROOT.RDataFrame(directory.GetName()+"/O2track", input_name)
        print("Has", df.Count().GetValue(), "tracks")
        df = df.Define("HowMany", "Count(fCollisionsID)").Sum("HowMany").GetValue()
        print(df)

    for i in lk:
        d = f.Get(i.GetName())
        if "TDirectoryFile" not in d.ClassName():
            continue
        check_dir(d)


if __name__ == "__main__":
    for i in argv[1:]:
        main(i)
