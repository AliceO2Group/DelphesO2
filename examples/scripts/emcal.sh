#! /usr/bin/env bash

NJOBS=5        # number of max parallel runs
NRUNS=10       # number of runs
NEVENTS=10000  # number of events in a run

BFIELD=5.      # magnetic field    [kG]
EMCRAD=100.    # EMCAL radius      [cm]
EMCLEN=200.    # EMCAL half length [cm]
EMCETA=1.443   # EMCAL max pseudorapidity

### calculate max eta from geometry
EMCETA=`awk -v a=$EMCRAD -v b=$EMCLEN 'BEGIN {th=atan2(a,b)*0.5; sth=sin(th); cth=cos(th); print -log(sth/cth)}'`
echo "maxEta = $EMCETA"

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.photons.tcl propagate.tcl
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_inel.cfg .
cp $DELPHESO2_ROOT/examples/smearing/emcal.C .

### set magnetic field
sed -i -e "s/set barrel_Bz .*$/set barrel_Bz ${BFIELD}e\-1/" propagate.tcl
sed -i -e "s/double Bz = .*$/double Bz = ${BFIELD}e\-1\;/" emcal.C
### set EMCAL radius
sed -i -e "s/set barrel_Radius .*$/set barrel_Radius ${EMCRAD}e\-2/" propagate.tcl
sed -i -e "s/double emcal_radius = .*$/double emcal_radius = ${EMCRAD}\;/" emcal.C
### set EMCAL length
sed -i -e "s/set barrel_HalfLength .*$/set barrel_HalfLength ${EMCLEN}e\-2/" propagate.tcl
sed -i -e "s/double emcal_length = .*$/double emcal_length = ${EMCLEN}\;/" emcal.C
### set EMCAL acceptance
sed -i -e "s/set barrel_Acceptance .*$/set barrel_Acceptance \{ 0.0 + 1.0 * fabs(eta) < ${EMCETA} \}/" propagate.tcl

### make sure we are clean to run
rm -rf .running* delphes*.root *.log

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

    ### run Delphes and analysis    
    DelphesPythia8 propagate.tcl pythia8.$I.cfg delphes.$I.root &> delphes.$I.log &&
	root -b -q -l "emcal.C(\"delphes.$I.root\", \"emcal.$I.root\")" &> emcal.$I.log &&
	rm -rf delphes.$I.root &&
	rm -rf .running.$I &&
	echo " --- complete run $I" &
    
done

### merge runs when all done
wait
hadd -f emcal.root emcal.*.root && rm -rf emcal.*.root
