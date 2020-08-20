#! /usr/bin/env bash

NJOBS=5
NRUNS=10
NEVENTS=10000

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tcl .
cp $DELPHESO2_ROOT/examples/smearing/luts/lutCovm.* .
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_inel.cfg .
cp $DELPHESO2_ROOT/examples/smearing/tof.C .

### loop over runs
for I in $(seq 1 $NRUNS); do

    ### wait for a free slot
    while [ $(ls .running.* 2> /dev/null | wc -l) -ge $NJOBS ]; do
	echo " --- waiting for a free slot"
	sleep 1
    done
    
    ### book the slot
    echo " --- starting run $I"
    touch .running.$I
    
    ### copy pythia8 configuration and adjust it
    cp pythia8_inel.cfg pythia8.$I.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.$I.cfg
    echo "Random:seed = $I" >> pythia8.$I.cfg
    echo "Beams:allowVertexSpread on " >> pythia8.$I.cfg
    echo "Beams:sigmaTime 60." >> pythia8.$I.cfg

    ### run Delphes and analysis    
    DelphesPythia8 propagate.2kG.tcl pythia8.$I.cfg delphes.$I.root  &> /dev/null &&
	root -b -q -l "tof.C(\"delphes.$I.root\", \"tof.$I.root\")"  &> /dev/null &&
	rm -rf delphes.$I.root &&
	rm -rf .running.$I &&
	echo " --- complete run $I" &
    
done

### merge runs when all done
wait
hadd -f tof.root tof.*.root && rm -rf tof.*.root

### clean
rm propagate.2kG.tcl lutCovm.* pythia8_inel.cfg tof.C
