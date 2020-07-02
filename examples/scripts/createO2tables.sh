#! /usr/bin/env bash

NRUNS=1
NEVENTS=1000

cp ../cards/propagate.2kG.tcl .
cp ../smearing/luts/lutCovm.* .
rm delphes.root
for I in $(seq 1 $NRUNS); do

    cp pythia8_inel.cfg pythia8.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
    echo "Random:seed = $I" >> pythia8.cfg
    
    DelphesPythia8 propagate.2kG.tcl pythia8.cfg delphes.root &&
	root -b -q -l "createO2tables.C(\"delphes.root\", \"AODRun5.$I.root\")" &&
	rm -rf delphes.root
done

hadd -f AODRun5Tot.root AODRun5.*.root && rm -rf AODRun5.*.root
source clean.sh
