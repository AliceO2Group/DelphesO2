#!/usr/bin/env python3

"""
Script to check the consistency of the written MC particles, in particular focusing on the order particles are written
"""

from ROOT import TFile
from sys import argv
from ROOT import RDataFrame, TCanvas, RDF, gPad, TLegend, gInterpreter, TDatabasePDG
import argparse
import numpy
import multiprocessing


class bcolors:
    # Colors for bash
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    BOKBLUE = BOLD + OKBLUE
    OKGREEN = "\033[92m"
    BOKGREEN = BOLD + OKGREEN
    WARNING = "\033[93m"
    BWARNING = BOLD + WARNING
    FAIL = "\033[91m"
    BFAIL = BOLD + FAIL
    ENDC = "\033[0m"


# Function to create indices for each particle
gInterpreter.Declare("""
                        int part_index = 0;
                        auto index_maker(Int_t ev) {
                            return part_index++;
                        }
                    """)


def main(filename, verbose=True, pdg_of_interest=[421], events=None, summary=True):
    def get_frame(tree_name, file_name):
        """
        Getter of the frame from the file
        """
        frame = RDataFrame(tree_name, file_name)
        if verbose:
            colNames = frame.GetColumnNames()
            for j in colNames:
                print(j)
        return frame

    df = get_frame("TF_0/O2mcparticle", filename)
    df = df.Define("part_index",
                   "index_maker(fMcCollisionsID)")
    counters = {}

    def count(label, index):
        if not summary:
            return
        c = counters.setdefault(label, [])
        if index not in c:
            c.append(index)

    def print_evt(ev=None, part_max=None):
        pdg_db = TDatabasePDG()
        if ev is not None:
            ev_df = df.Filter(f"fMcCollisionsID == {ev}")
        else:
            ev_df = df.Filter(f"fMcCollisionsID >= 0")
        npy = ev_df.AsNumpy()
        print()
        for i, part_index in enumerate(npy["part_index"]):
            ev = npy["fMcCollisionsID"][i]
            count("events", ev)
            m0 = npy["fMother0"][i]
            m1 = npy["fMother1"][i]
            d0 = npy["fDaughter0"][i]
            d1 = npy["fDaughter1"][i]
            pdg = npy["fPdgCode"][i]
            px = npy["fPx"][i]
            py = npy["fPy"][i]
            pz = npy["fPz"][i]
            part = pdg_db.GetParticle(int(pdg))
            if part:
                part = part.GetName()
            else:
                part = "Undef"

            def daughters():
                idaughters = []
                if d0 > -1 and d1 > -1:
                    for j in range(d0, d1+1):
                        entry = numpy.where(npy["part_index"] == j)
                        if len(entry) > 1:
                            raise ValueError("Entry size is too high!")
                        if len(entry) == 0:
                            raise ValueError("Entry size is too low!")
                        idaughters.append(entry[0][0])
                # Checking indices
                if sorted(idaughters) != idaughters:
                    raise ValueError("Daughters are not in order!")
                return idaughters

            def daughters_pxpypz():
                d_px = 0
                d_py = 0
                d_pz = 0
                d = daughters()
                for j in d:
                    d_px += npy["fPx"][j]
                    d_py += npy["fPy"][j]
                    d_pz += npy["fPz"][j]
                return d_px, d_py, d_pz

            def daughters_pdg(do_abs=False):
                d = daughters()
                pdgs = []
                for j in d:
                    pdgs.append(abs(npy["fPdgCode"][j])
                                if do_abs else npy["fPdgCode"][j])
                return pdgs

            def check_momentum():
                d_p = daughters_pxpypz()
                m_p = [px, py, pz]
                for j in enumerate(m_p):
                    if (j[1] - d_p[j[0]]) > 0.001:
                        raise ValueError("Wrong P", j, "with", d_p)

            def is_decay_channel(desired_pdg_codes, do_abs=True, fill_counter=True):
                pdgs = daughters_pdg(do_abs=do_abs)
                for i in desired_pdg_codes:
                    if i not in pdgs:
                        return False
                if fill_counter:
                    count(f"{part} in {desired_pdg_codes}", part_index)
                return True

            extra = ""
            if m0 < 0 and m1 < 0 and d0 < 1 and d1 < 0:
                extra = "Sterile"
            if d1 <= d0 and d1 > -1:
                extra = bcolors.BWARNING + "Problematic" + bcolors.ENDC
            l = f"  ({part_index}) ev {ev} m0 {m0} m1 {m1}, d0 {d0} d1 {d1}, pdg {pdg} '{part}' {extra}"

            count(part, part_index)
            if verbose or pdg in pdg_of_interest:
                print(l)
            if 1:
                if pdg == 421:
                    is_decay_channel([321, 211], fill_counter=True)
                    check_momentum()
                elif pdg == -421:
                    is_decay_channel([321, 211], fill_counter=True)

    if events is None:
        print_evt()
    else:
        for i in events:
            print_evt(i)
    for i in counters:
        print(i, ":", len(counters[i]))
    if not summary:
        print("Processed", filename)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file_list", type=str, nargs="+",
                        help="Input configuration file")
    parser.add_argument("--events", type=int, nargs="+", default=None,
                        help="Events to analyze")
    parser.add_argument("--njobs", type=int, default=10, help="Number of jobs")
    parser.add_argument("-s", "--summary", action="store_true",
                        help="Flag to show summary after processing a file")
    parser.add_argument("-b", action="store_true", help="Background mode")
    parser.add_argument("-v", action="store_true", help="Verbose mode")
    args = parser.parse_args()
    file_list = args.file_list
    if len(file_list) < 3:
        for i in file_list:
            main(i, verbose=args.v, events=args.events, summary=args.summary)
    else:
        with multiprocessing.Pool(processes=args.njobs) as pool:
            pool.map(main, file_list)
