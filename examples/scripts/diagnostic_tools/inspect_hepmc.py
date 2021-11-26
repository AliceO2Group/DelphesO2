#!/usr/bin/env python3

"""
Inspector of the HepMC file
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

import pyhepmc_ng as hep
import argparse


poi = {}
poi_names = {2212: "Proton", -2212: "AntiProton",
             2112: "Neutron", -2112: "AntiNeutron",
             3122: "Lambda0", 1000010020: "Deuteron",
             1000020030: "Triton", 1000020040: "Alpha",
             1000020030: "Helium3"}

for i in poi_names:
    poi[i] = 0


def main(file_name, min_event, max_event, verbose):
    print("Reading", file_name, "between",
          min_event, "and", max_event, "events")

    def print_evt(evt):
        def msg(*m):
            if verbose:
                print(*m)
        msg("event_number:", evt.event_number)
        msg("Units:", "momentum_unit:", evt.momentum_unit,
            "length_unit:", evt.length_unit)
        msg(len(evt.particles), "particles:")
        for i in enumerate(evt.particles):
            pdg = i[1].pid
            if pdg in poi:
                poi[pdg] = poi[pdg]+1
                pdg = f"{pdg} is of interest!!!"
            msg(i, "PDG code", pdg)
        msg(len(evt.vertices), "vertices:")
        for i in enumerate(evt.vertices):
            msg("Vertex:", i)
            vertex_pdgs = []
            msg("Input particles")
            for j in i[1].particles_in:
                msg("\t", j, "pdg", j.pid)
            msg("Output particles")
            for j in i[1].particles_out:
                msg("\t", j, "pdg", j.pid)
                vertex_pdgs.append(j.pid)
            if 2212 in vertex_pdgs and 2112 in vertex_pdgs:
                print(evt.event_number, "Has both")
                print(i)
                for j in i[1].particles_out:
                    print(j)

    with hep.open(file_name) as f:
        while True:
            e = f.read()
            if not e:
                break
            if e.event_number < min_event:
                continue
            print_evt(e)
            if e.event_number >= max_event:
                break
    for i in poi:
        print("Number of", poi_names[i]+"s", poi[i])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("hepmcfile", type=str,
                        help="Input hepmc file.")
    parser.add_argument("--start", type=int, default=0,
                        help="Start of the event counter.")
    parser.add_argument("--stop", type=int, default=100,
                        help="Stop of the event counter.")
    parser.add_argument("-v", action="store_true",
                        help="Verbose mode.")
    args = parser.parse_args()
    main(args.hepmcfile, min_event=args.start,
         max_event=args.stop, verbose=args.v)
