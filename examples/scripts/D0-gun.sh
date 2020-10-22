#! /usr/bin/env bash

NRUNS=10
NEVENTS=10000

for I in $(seq 1 $NRUNS); do

    rpythia8-gun -n $NEVENTS \
		 --output D0-gun.hepmc \
		 --pdg 421 \
		 --px 1. --py 0. --pz 0. \
		 --xProd 1. --yProx 0. --zProd 0. \
		 --config ~/alice/O2DPG/MC/config/PWGHF/pythia8/decayer/force_hadronic_D.cfg \
		 --decay 

    DelphesHepMC propagate.2kG.tcl delphes.root D0-gun.hepmc &&	rm -rf delphes.root
    
done
