#! /usr/bin/env bash

NRUNS=2
NEVENTS=4000

for I in $(seq 1 $NRUNS); do

    cp pythia8_inel.cfg pythia8.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
    echo "Random:seed = $I" >> pythia8.cfg
    
    DelphesPythia8 propagate.2kG.tcl pythia8.cfg delphes.root &&
	root -b -q -l "createO2tables.C(\"delphes.root\", \"AODRun3.$I.root\")" &&
	rm -rf delphes.root
done

hadd -f AODRun3Tot.root AODRun3.*.root && rm -rf AODRun3.*.root
