#! /usr/bin/env bash

NRUNS=10
NEVENTS=10000

for I in $(seq 1 $NRUNS); do

    cp pythia8_inel.cfg pythia8.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
    echo "Random:seed = $I" >> pythia8.cfg
    
    DelphesPythia8 propagate.2kG.tcl pythia8.cfg delphes.root &&
	root -b -q -l "dca.C(\"delphes.root\", \"dca.nsigma.$I.root\", true)" &&
	root -b -q -l "dca.C(\"delphes.root\", \"dca.mm.$I.root\", false)" &&
	rm -rf delphes.root
done

hadd -f dca.nsigma.root dca.nsigma.*.root && rm -rf dca.nsigma.*.root
hadd -f dca.mm.root dca.mm.*.root && rm -rf dca.mm.*.root
