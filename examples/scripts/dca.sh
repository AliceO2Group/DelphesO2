#! /usr/bin/env bash

NRUNS=10
NEVENTS=10000

for I in $(seq 1 $NRUNS); do

    cp pythia8_inel.cfg pythia8.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
    echo "Random:seed = $I" >> pythia8.cfg
    
    DelphesPythia8 propagate.2kG.tcl pythia8.cfg delphes.root &&
	root -b -q -l "dca.C(\"delphes.root\", \"dca.$I.root\")" &&
	rm -rf delphes.root
done

hadd -f dca.root dca.*.root && rm -rf dca.*.root
