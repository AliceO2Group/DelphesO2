#!/usr/bin/env python3

"""
Script to check the consistency between O2 tables and delphes.
This allows the user to check the consistency of variables in the AOD and in the delphes file.
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

from ROOT import TFile
from sys import argv
from ROOT import RDataFrame, TCanvas, RDF, gPad, TLegend, gInterpreter
import argparse
import numpy


def check_trees(file_list):
    """
    Function to check that the file is correctly written and has only one folder with the tree
    """
    print("Checking file")
    for i in file_list:
        f = TFile(i, "READ")
        l = f.GetListOfKeys()
        if l.GetEntries() > 2:
            print(i, "Bad file")


def check_corresponding(file_list,
                        origin="AODRun5",
                        friend="delphes",
                        verbose=False,
                        show=False):
    """
    Function to check that the delphes and AODs are consistent
    """
    print("Checking correspondance between O2 tables and delphes output")
    for i in file_list:
        def get_frame(tree_name, file_name):
            frame = RDataFrame(tree_name, file_name)
            if verbose:
                colNames = frame.GetColumnNames()
                for j in colNames:
                    print(j)
            return frame

        df = get_frame("TF_0/O2mcparticle", i)
        df = df.Define("fP", "TMath::Sqrt(fPx*fPx + fPy*fPy + fPz*fPz)")
        df = df.Define("fEta", "-0.5*TMath::Log((fP+fPz)/(fP-fPz))")
        #
        df_reco = get_frame("TF_0/O2track", i)
        df_reco = df_reco.Define("fEta",
                                 "-1.f * TMath::Log(TMath::Tan(0.25f * TMath::Pi() - 0.5f * TMath::ATan(fTgl)))")
        df_reco = df_reco.Define("fPt", "1./TMath::Abs(fSigned1Pt)")
        #
        df_delphes = get_frame("Delphes", i.replace(origin, friend))
        gInterpreter.Declare("""
            auto p(ROOT::VecOps::RVec<Float_t> px, ROOT::VecOps::RVec<Float_t> py, ROOT::VecOps::RVec<Float_t> pz) {
                std::vector<Float_t> v;
                int counter = 0;
                for(auto i : px){
                    v.push_back(TMath::Sqrt(px[counter]*px[counter] + py[counter]*py[counter] + pz[counter]*pz[counter]));
                    counter++;
                }
                return v; 
            }
        """)
        gInterpreter.Declare("""
            auto eta(ROOT::VecOps::RVec<Float_t> p, ROOT::VecOps::RVec<Float_t> pz) {
                std::vector<Float_t> v;
                int counter = 0;
                for(auto i : p){
                    v.push_back(0.5*TMath::Log((p[counter]+pz[counter])/(p[counter]-pz[counter])));
                    counter++;
                }
                return v; 
            }
        """)
        gInterpreter.Declare("""
            auto recomc(ROOT::VecOps::RVec<Float_t> mc, ROOT::VecOps::RVec<Int_t> mclabel, ROOT::VecOps::RVec<TRef> recolabel) {
                std::vector<Float_t> v;
                int counter = 0;
                for(auto i : mc){
                    for(auto j : recolabel){
                        if (mclabel[counter] == j.GetUniqueID()) {v.push_back(i);}
                    }
                    counter++;
                }
                return v; 
            }
        """)
        gInterpreter.Declare("""
            auto diff(ROOT::VecOps::RVec<Float_t> a, ROOT::VecOps::RVec<Float_t> b) {
                std::vector<Float_t> v;
                int counter = 0;
                for(auto i : a){
                    v.push_back(i-b[counter]);
                    counter++;
                }
                return v; 
            }
        """)

        df_delphes = df_delphes.Define("P",
                                       "p(Particle.Px, Particle.Py, Particle.Pz)")
        df_delphes = df_delphes.Define("Eta",
                                       "eta(P, Particle.Pz)")
        df_delphes = df_delphes.Define("RecoEta",
                                       "recomc(Eta, Particle.fUniqueID, Track.Particle)")
        df_delphes = df_delphes.Define(
            "EtaDiff", "diff(RecoEta, Particle.Eta)")
        df_delphes = df_delphes.Define("RecoPhi",
                                       "recomc(Particle.Phi, Particle.fUniqueID, Track.Particle)")
        df_delphes = df_delphes.Define("PhiDiff", "diff(RecoPhi, Track.Phi)")

        canvas_list = []

        def canvas(name, diff=False):
            can = TCanvas(name, name, 800, 1280)
            can.Divide(1, 2)
            canvas_list.append(can)
            return can

        def check(var, nbins, low, up, friend_var="Particle.{}", frame=df):
            h = f"{origin} {var}"
            h = frame.Histo1D(RDF.TH1DModel(h,
                                            h,
                                            nbins, low, up),
                              f"f{var}")
            friend_var = friend_var.format(var)
            h2 = f"{friend} {friend_var}"
            h2 = df_delphes.Histo1D(RDF.TH1DModel(h2,
                                                  h2,
                                                  nbins, low, up),
                                    friend_var)
            h2.SetLineColor(2)
            h2.SetLineStyle(3)
            can = canvas(var, diff=True)
            can.cd(1)
            h.SetDirectory(0)
            h2.SetDirectory(0)
            hdrawn = [h.DrawCopy(), h2.DrawCopy("same")]
            for i in hdrawn:
                i.SetDirectory(0)
            leg = TLegend(.7, .5, .9, .75)
            leg.AddEntry(h.GetValue())
            leg.AddEntry(h2.GetValue())
            leg.Draw()
            gPad.Update()
            can.cd(2)
            hdiff = h.DrawCopy()
            hdiff.SetDirectory(0)
            hdiff.SetName("hdiff")
            hdiff.SetTitle("diff")
            hdiff.Add(h2.GetValue(), -1)
            hdiff.GetYaxis().SetRangeUser(-1, 1)
            gPad.Update()
            for i in range(1, hdiff.GetNbinsX()+1):
                diff = hdiff.GetBinContent(i)
                if diff != 0:
                    return False
            return True

        def correlate(frame, x, y):
            hn = f"{x[0]}_vs_{y[0]}"
            ht = f";{x[0]};{y[0]}"
            h = frame.Histo2D(RDF.TH2DModel(hn,
                                            ht,
                                            x[1], x[2], x[3],
                                            y[1], y[2], y[3]),
                              x[0],
                              y[0])
            can = canvas(hn)
            can.SetLeftMargin(0.15)
            h.Draw("COLZ")
            can.Update()

        def plot(frame, x):
            hn = f"{x[0]}"
            ht = f";{x[0]}"
            h = frame.Histo1D(RDF.TH1DModel(hn,
                                            ht,
                                            x[1], x[2], x[3]),
                              x[0])
            can = canvas(hn)
            can.SetLeftMargin(0.15)
            h.Draw("COLZ")
            can.Update()

        # Comparison of Delphes Particles and O2 Particles
        variables = {"Px": [1000, -100, 100],
                     "Py": [1000, -100, 100],
                     "Pz": [1000, -100, 100],
                     "P": [1000, 0, 100],
                     "Vx": [1000, -100, 100, "Particle.X"],
                     "Vy": [1000, -100, 100, "Particle.Y"],
                     "Vz": [1000, -100, 100, "Particle.Z"],
                     "Eta": [1000, -10, 10],
                     "E": [1000, 0, 1000]}
        for i in variables:
            x = variables[i]
            if not check(i, *x):
                print("Something is wrong for", i)
        # Comparison of Delphes Tracks and O2 Tracks
        check("Eta", 1000, -10, 10, frame=df_reco, friend_var="Track.{}")
        check("Pt", 1000, 0, 30, frame=df_reco, friend_var="Track.PT")
        # check("Pt", 1000, 0, 30, frame=df_reco, friend_var="Particle.PT")
        # Correlation of Delphes variables
        correlate(df_delphes, ["Eta", 1000, -10, 10],
                  ["Particle.Eta", 1000, -10, 10])
        correlate(df_delphes, ["P", 1000, -1, 10],
                  ["Particle.P", 1000, -1, 10])
        correlate(df_delphes, ["Track.Eta", 1000, -10, 10],
                  ["EtaDiff", 1000, -10, 10])
        # Plot of Delphes variables
        plot(df_delphes, ["EtaDiff", 1000, -2, 2])
        plot(df_delphes, ["PhiDiff", 1000, -2, 2])
        if show:
            input("Press enter to continue")
        fout = TFile("table_check.root", "RECREATE")
        fout.cd()
        canvas_list[0].SaveAs("table_check.pdf[")
        for i in canvas_list:
            i.SaveAs("table_check.pdf")
            i.Write()
        canvas_list[0].SaveAs("table_check.pdf]")
        fout.Close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file_list", type=str, nargs="+",
                        help="Space separated list of the AODs to analyze e.g. /tmp/AODRun5.0.root")
    parser.add_argument("-b", action="store_true", help="Background mode")
    parser.add_argument("-v", action="store_true", help="Verbose mode")
    args = parser.parse_args()
    file_list = args.file_list
    check_trees(file_list)
    check_corresponding(file_list, verbose=args.v, show=not args.b)
