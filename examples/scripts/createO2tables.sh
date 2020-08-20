#! /usr/bin/env bash

NRUNS=1
NEVENTS=1000

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tcl .
cp $DELPHESO2_ROOT/examples/smearing/luts/lutCovm.* .
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_ccbar.cfg .
# cp $DELPHESO2_ROOT/examples/aod/createO2tables.C .

### loop over runs
for I in $(seq 1 $NRUNS); do

    ### copy pythia8 configuration and adjust it
    cp pythia8_ccbar.cfg pythia8.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
    echo "Random:seed = $I" >> pythia8.cfg

    ### run Delphes and analysis
    DelphesPythia8 propagate.2kG.tcl pythia8.cfg delphes.root &&
	root -b -q -l "createO2tables.C(\"delphes.root\", \"AODRun5.$I.root\")" &&
	rm -rf delphes.root

done

### merge runs
hadd -f AODRun5Tot.root AODRun5.*.root && rm -rf AODRun5.*.root

### clean
# rm *.tcl *.cfg *.dat *.C
