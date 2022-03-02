#!/usr/bin/env python3

"""
Script to check the consistency of the written MC particles, in particular focusing on the order particles are written
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

from ROOT import RDataFrame, TCanvas, RDF, gPad, TLegend, gInterpreter, TDatabasePDG
import argparse
import numpy
from os import path
try:
    from common import run_cmd, warning_msg, bcolors
except:
    raise FileNotFoundError("Cannot find common.py in path, you can download it via: wget https://raw.githubusercontent.com/AliceO2Group/DelphesO2/master/examples/scripts/common.py")
import multiprocessing


# Function to create indices for each particle
gInterpreter.Declare("""
                        int part_index = 0;
                        int index_maker(Int_t ev) {
                            return part_index++;
                        }
                    """)
# Function to check if it is physical primary
gInterpreter.Declare("""
                        enum MCParticleFlags : uint8_t {
                        ProducedByTransport = 0x1,
                        FromBackgroundEvent = 0x2, // Particle from background event (may have been used several times)
                        PhysicalPrimary = 0x4      // Particle is a physical primary according to ALICE definition
                        };
                        bool physPrim(UChar_t flag) {
                            return (flag & PhysicalPrimary) == PhysicalPrimary;
                            uint8_t f = static_cast<uint8_t>(flag);
                            //Printf("%i %c", f, flag);
                            return (f & PhysicalPrimary) == PhysicalPrimary;
                        }
                        bool producedTransport(UChar_t flag) {
                            return (flag & ProducedByTransport) == ProducedByTransport;
                            uint8_t f = static_cast<uint8_t>(flag);
                            //Printf("%i %c", f, flag);
                            return (f & ProducedByTransport) == ProducedByTransport;
                        }
                    """)


def main(filename, verbose=True, pdg_of_interest=[421], event_filters=None, summary=True, continue_on_inconsistency=True):
    def get_frame(file_name, df_index=0, tree_name="O2mcparticle_001"):
        """
        Getter of the frame from the file
        """
        if not path.isfile(file_name):
            raise ValueError("Did not find AOD file", file_name)
        sub_names = run_cmd(f"rootls {file_name}").strip().split()
        df_name = []
        for i in sub_names:
            if not i.startswith("DF_") and not i.startswith("TF_"):
                continue
            df_name.append(i)
        df_name = df_name[df_index]
        print(df_name)
        frame = RDataFrame(f"{df_name}/{tree_name}", file_name)
        if verbose:
            colNames = frame.GetColumnNames()
            for j in enumerate(colNames):
                print(j, frame.GetColumnType(j[1]))
        return frame

    df = get_frame(filename)
    df = df.Define("part_index",
                   "index_maker(fIndexMcCollisions)")
    df = df.Define("isPhysicalPrimary",
                   "physPrim(fFlags)")
    df = df.Define("isProducedByTransport",
                   "producedTransport(fFlags)")
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
        ev_df = df.Filter(f"fIndexMcCollisions {event_filter}")
        npy = ev_df.AsNumpy()
        print()
        lastmother = 0
        for i, part_index in enumerate(npy["part_index"]):
            ev = npy["fIndexMcCollisions"][i]
            count("events", ev)
            if 0:
                m0 = npy["fMother0"][i]
                m1 = npy["fMother1"][i]
                d0 = npy["fDaughter0"][i]
                d1 = npy["fDaughter1"][i]
            else:
                m_arr = npy["fIndexArray_Mothers"][i]
                d_arr = npy["fIndexSlice_Daughters"][i]
                m_size = npy["fIndexArray_Mothers_size"][i]
                # print(m_size)
                # print("Mothers", m_arr)
                # print("Daughters", d_arr)

                if len(m_arr) == 0:
                    m0 = -1
                    m1 = -1
                else:
                    m0 = m_arr[0]
                    m1 = m_arr[int(m_size)-1]
                d0 = d_arr[0]
                d1 = d_arr[1]
                # print(d_arr)
            pdg = npy["fPdgCode"][i]
            px = npy["fPx"][i]
            py = npy["fPy"][i]
            pz = npy["fPz"][i]
            is_ps = bool(npy["isPhysicalPrimary"][i])
            is_pt = bool(npy["isProducedByTransport"][i])
            process = npy["fStatusCode"][i]

            def getpname(pdg_code):
                p = pdg_db.GetParticle(int(pdg_code))
                if p:
                    p = p.GetName()
                else:
                    p = "Undef"
                return p
            part = getpname(pdg)
            summary_line = f"  ({part_index}) ev {ev} m0 {m0} m1 {m1}, d0 {d0} d1 {d1}, pdg {pdg} '{part}', physical primary {is_ps}, in transport {is_pt}, process {process}"
            if abs(pdg) not in [21, 2101, 2103, 2203, 1, 2, 3, 4, 5] and m0 > -1:
                if lastmother != m0 and count("mothers", m0):
                    raise ValueError("Duplicate mothers for ", summary_line)
            lastmother = m0
            if d1 > -1 and d0 > d1:
                if not continue_on_inconsistency:
                    raise ValueError("d0 > d1:", summary_line)
                else:
                    warning_msg("d0 > d1 for", part_index)

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
                        if 0:
                            d_m0 = npy["fMother0"][entry]
                            d_m1 = npy["fMother1"][entry]
                        else:
                            d_m0 = npy["fIndexArray_Mothers"][entry][0]
                            d_m1 = npy["fIndexArray_Mothers"][entry][int(
                                npy["fIndexArray_Mothers_size"][entry])-1]

                        if d_m0 != part_index and d_m1 != part_index:
                            if not continue_on_inconsistency:
                                raise ValueError("Daughter", j,
                                                 "has a different mother!",
                                                 "d_m0", d_m0, "d_m1", d_m1, "w.r.t.", part_index)
                            else:
                                warning_msg("Daughter", j,
                                            "has a different mother!",
                                            "d_m0", d_m0, "d_m1", d_m1, "w.r.t.", part_index)
                        if d_m0 == d_m1 and 0:
                            raise ValueError("Daughter has same mother!",
                                             d_m0, d_m1)
                        idaughters.append(entry)
                if len(idaughters) == 0:
                    warning_msg("Found no daughters")
                    return idaughters
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
                if len(d) == 0:
                    return None
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
                if d_p is None:
                    return
                m_p = [px, py, pz]
                m_p_d = {0: "Px", 1: "Py", 2: "Pz"}
                for j in enumerate(m_p):
                    if (j[1] - d_p[j[0]]) > 0.001:
                        e_msg = ["Non-closure in", m_p_d[j[0]], "=", d_p]
                        if not continue_on_inconsistency:
                            raise ValueError(*e_msg)
                        else:
                            warning_msg(*e_msg)

            def is_decay_channel(desired_pdg_codes, fill_counter=True, min_prongs=0, max_prongs=10):
                d = daughters()
                d_pdgs = daughters_pdg()
                if len(d) >= min_prongs and len(d) <= max_prongs:
                    print(pdg, part, "decaying in")
                    for i, j in enumerate(d_pdgs):
                        if 0:
                            this_m0 = npy["fMother0"][d[i]]
                            this_m1 = npy["fMother1"][d[i]]
                        else:
                            this_m0 = npy["fIndexArray_Mothers"][d[i]][0]
                            this_m1 = npy["fIndexArray_Mothers"][d[i]][int(
                                npy["fIndexArray_Mothers_size"][d[i]])-1]

                        print(" >", j, getpname(j),
                              "index", d[i], npy["part_index"][d[i]], "m0", this_m0, "m1", this_m1, " -> physical primary", npy["isPhysicalPrimary"][d[i]])
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
            if d1 < 0 and d1 != d0:
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
                        default=[0], help="PDG of interest")
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
