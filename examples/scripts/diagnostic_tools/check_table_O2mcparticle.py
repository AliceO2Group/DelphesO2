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


def main(filename, verbose=True, pdg_of_interest=[421], event_filters=None, summary=True):
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
            return False
        return True

    def print_evt(event_filter=">= 0"):
        pdg_db = TDatabasePDG()
        ev_df = df.Filter(f"fMcCollisionsID {event_filter}")
        npy = ev_df.AsNumpy()
        print()
        lastmother = 0
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

            def getpname(pdg_code):
                p = pdg_db.GetParticle(int(pdg_code))
                if p:
                    p = p.GetName()
                else:
                    p = "Undef"
                return p
            part = getpname(pdg)
            summary_line = f"  ({part_index}) ev {ev} m0 {m0} m1 {m1}, d0 {d0} d1 {d1}, pdg {pdg} '{part}'"
            if abs(pdg) not in [21, 2101, 2103, 2203, 1, 2, 3, 4, 5] and m0 > -1:
                if lastmother != m0 and count("mothers", m0):
                    raise ValueError("Duplicate mothers for ", summary_line)
            lastmother = m0
            if d1 > -1 and d0 > d1:
                raise ValueError("d0 < d1:", summary_line)

            def daughters():
                idaughters = []
                if d0 > -1 and d1 > -1:
                    for j in range(d0, d1+1):
                        entry = numpy.where(npy["part_index"] == j)[0]
                        if len(entry) > 1:
                            raise ValueError("Entry size is too high!")
                        if len(entry) == 0:
                            raise ValueError("Entry size is too low!")
                        entry = entry[0]
                        d_m0 = npy["fMother0"][entry]
                        d_m1 = npy["fMother1"][entry]
                        if d_m0 != part_index and d_m1 != part_index:
                            raise ValueError("Daughter has a different mother!",
                                             d_m0, d_m1, "w.r.t.", part_index)
                        if d_m0 == d_m1:
                            raise ValueError("Daughter has same mother!",
                                             d_m0, d_m1)
                        idaughters.append(entry)
                # Checking that indices are increasing
                if sorted(idaughters) != idaughters:
                    raise ValueError("Daughters are not in order!")
                # Checking that indices have no holes
                if idaughters != [*range(idaughters[0], idaughters[-1]+1)]:
                    raise ValueError("Daughters have hole in indices!",
                                     idaughters)
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

            def daughters_pdg():
                d = daughters()
                d_pdgs = []
                for j in d:
                    d_pdgs.append(npy["fPdgCode"][j])
                return d_pdgs

            def check_momentum():
                d_p = daughters_pxpypz()
                m_p = [px, py, pz]
                for j in enumerate(m_p):
                    if (j[1] - d_p[j[0]]) > 0.001:
                        raise ValueError("Wrong P", j, "with", d_p)

            def is_decay_channel(desired_pdg_codes, fill_counter=True, min_prongs=0, max_prongs=10):
                d = daughters()
                d_pdgs = daughters_pdg()
                if len(d) >= min_prongs and len(d) <= max_prongs:
                    print(pdg, part, "decaying in")
                    for i, j in enumerate(d_pdgs):
                        print(" >", j, getpname(
                            j), "index", d[i], npy["part_index"][d[i]], "m0", npy["fMother0"][d[i]], "m1", npy["fMother1"][d[i]])
                if desired_pdg_codes is not None:
                    for i in desired_pdg_codes:
                        if i not in d_pdgs:
                            return False
                if fill_counter:
                    count(
                        f"{bcolors.BOKGREEN} {pdg} {part} {bcolors.ENDC} in {d_pdgs}", part_index)
                return True

            extra = []
            if m0 < 0 and m1 < 0 and d0 < 1 and d1 < 0:
                extra.append("Sterile")
            if d1 <= d0 and d1 > -1:
                extra.append(bcolors.BWARNING + "Problematic" + bcolors.ENDC)
            if pdg in pdg_of_interest:
                extra.append(bcolors.BOKGREEN +
                             "PDG of interest" + bcolors.ENDC)
            extra = " ".join(extra)

            count(part, part_index)
            if verbose or pdg in pdg_of_interest:
                print(summary_line, extra)
            if pdg in pdg_of_interest:
                check_momentum()
                is_decay_channel(None, fill_counter=True)

    if event_filters is None:
        print_evt()
    else:
        for i in event_filters:
            if i.isdigit():
                i = f"== {i}"
            print_evt(i)
    for i in counters:
        print(i, ":", len(counters[i]))
    if not summary:
        print("Processed", filename)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file_list", type=str, nargs="+",
                        help="Input configuration file")
    parser.add_argument("--events", "-e", type=str, nargs="+", default=None,
                        help="Events to analyze e.g. 0 1 2 or < 10")
    parser.add_argument("--njobs", type=int, default=10, help="Number of jobs")
    parser.add_argument("--pdg", "-p", nargs="+", type=int,
                        default=0, help="PDG of interest")
    parser.add_argument("-s", "--summary", action="store_true",
                        help="Flag to show summary after processing a file")
    parser.add_argument("-b", action="store_true", help="Background mode")
    parser.add_argument("-v", action="store_true", help="Verbose mode")
    args = parser.parse_args()
    file_list = args.file_list
    if len(file_list) < 3:
        for i in file_list:
            main(i,
                 verbose=args.v,
                 event_filters=args.events,
                 summary=args.summary,
                 pdg_of_interest=args.pdg)
    else:
        with multiprocessing.Pool(processes=args.njobs) as pool:
            pool.map(main, file_list)
