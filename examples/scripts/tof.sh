#! /usr/bin/env bash

NJOBS=5        # number of max parallel runs
NRUNS=10       # number of runs
NEVENTS=10000  # number of events in a run

BFIELD=5.      # magnetic field  [kG]
SIGMAT=0.020   # time resolution [ns]
TOFRAD=100.    # TOF radius      [cm]
TOFLEN=200.    # TOF half length [cm]

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tcl propagate.tcl
cp $DELPHESO2_ROOT/examples/smearing/luts/lutCovm.* .
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_inel.cfg .
cp $DELPHESO2_ROOT/examples/smearing/tof.C .

### set magnetic field
sed -i -e "s/set Bz .*$/set Bz ${BFIELD}e\-1/" propagate.tcl
sed -i -e "s/double Bz = .*$/double Bz = ${BFIELD}e\-1\;/" tof.C
### set TOF radius
sed -i -e "s/set Radius .*$/set Radius ${TOFRAD}e\-2/" propagate.tcl
sed -i -e "s/double tof_radius = .*$/double tof_radius = ${TOFRAD}\;/" tof.C
### set TOF length
sed -i -e "s/set HalfLength .*$/set HalfLength ${TOFLEN}e\-2/" propagate.tcl
sed -i -e "s/double tof_length = .*$/double tof_length = ${TOFLEN}\;/" tof.C
### set TOF time resolution
sed -i -e "s/set TimeResolution .*$/set TimeResolution ${SIGMAT}e\-9/" propagate.tcl
sed -i -e "s/double tof_sigmat = .*$/double tof_sigmat = ${SIGMAT}\;/" tof.C

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
    DelphesPythia8 propagate.tcl pythia8.$I.cfg delphes.$I.root  &> delphes.$I.log &&
	root -b -q -l "tof.C(\"delphes.$I.root\", \"tof.$I.root\")" &> tof.$I.log &&
	rm -rf delphes.$I.root &&
	rm -rf .running.$I &&
	echo " --- complete run $I" &
    
done

### merge runs when all done
wait
hadd -f tof.root tof.*.root && rm -rf tof.*.root
