#! /usr/bin/env bash

### run configuration
NJOBS=3        # number of max parallel runs
NRUNS=3        # number of runs
NEVENTS=1000   # number of events in a run

### detector configuration
BFIELD=5.      # magnetic field  [kG]
SIGMAT=0.020   # time resolution [ns]
RADIUS=100.    # radius      [cm]
LENGTH=200.    # half length [cm]
ETAMAX=1.443   # max pseudorapidity

### calculate max eta from geometry
ETAMAX=`awk -v a=$RADIUS -v b=$LENGTH 'BEGIN {th=atan2(a,b)*0.5; sth=sin(th); cth=cos(th); print -log(sth/cth)}'`

### verbose
echo " --- running createO2tables.sh "
echo "  nJobs   = $NJOBS "
echo "  nRuns   = $NRUNS "
echo "  nEvents = $NEVENTS "
echo " --- with detector configuration "
echo "  bField  = $BFIELD [kG] "
echo "  sigmaT  = $SIGMAT [ns] "
echo "  radius  = $RADIUS [cm] "
echo "  length  = $LENGTH [cm] "
echo "  etaMax  = $ETAMAX      "
echo " --- start processing the runs "

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tcl propagate.tcl
cp $DELPHESO2_ROOT/examples/smearing/luts/lutCovm.* .
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_ccbar.cfg .
cp $DELPHESO2_ROOT/examples/pythia8/decays/force_hadronic_D.cfg .
cp $DELPHESO2_ROOT/examples/aod/createO2tables.h .
cp $DELPHESO2_ROOT/examples/aod/createO2tables.C .

### set magnetic field
sed -i -e "s/set barrel_Bz .*$/set barrel_Bz ${BFIELD}e\-1/" propagate.tcl
sed -i -e "s/double Bz = .*$/double Bz = ${BFIELD}e\-1\;/" createO2tables.C
sed -i -e "s/\"d_bz\": .*$/\"d_bz\": \"${BFIELD}\"\,/" dpl-config_std.json
### set radius
sed -i -e "s/set barrel_Radius .*$/set barrel_Radius ${RADIUS}e\-2/" propagate.tcl
sed -i -e "s/double tof_radius = .*$/double tof_radius = ${RADIUS}\;/" createO2tables.C
### set length
sed -i -e "s/set barrel_HalfLength .*$/set barrel_HalfLength ${LENGTH}e\-2/" propagate.tcl
sed -i -e "s/double tof_length = .*$/double tof_length = ${LENGTH}\;/" createO2tables.C
### set acceptance
sed -i -e "s/set barrel_Acceptance .*$/set barrel_Acceptance \{ 0.0 + 1.0 * fabs(eta) < ${ETAMAX} \}/" propagate.tcl
### set time resolution
sed -i -e "s/set barrel_TimeResolution .*$/set barrel_TimeResolution ${SIGMAT}e\-9/" propagate.tcl
sed -i -e "s/double tof_sigmat = .*$/double tof_sigmat = ${SIGMAT}\;/" createO2tables.C

### make sure we are clean to run
rm -rf .running* delphes*.root *.log

### loop over runs
for I in $(seq 0 $(($NRUNS - 1))); do

    ### wait for a free slot
    while [ $(ls .running.* 2> /dev/null | wc -l) -ge $NJOBS ]; do
	echo " --- waiting for a free slot"
	sleep 1
    done

    ### book the slot
    echo " --- starting run $I"
    touch .running.$I
    
    ### copy pythia8 configuration and adjust it
    cp pythia8_ccbar.cfg pythia8.$I.cfg
    ### number of events and random seed
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.$I.cfg
    echo "Random:seed = $I" >> pythia8.$I.cfg
    ### collision time spread [mm/c]
    echo "Beams:allowVertexSpread on " >> pythia8.$I.cfg
    echo "Beams:sigmaTime 60." >> pythia8.$I.cfg
    
    ### force hadronic D decays
    cat force_hadronic_D.cfg >> pythia8.$I.cfg
    
    ### run Delphes and analysis
    #$(($I*$NEVENTS))
    DelphesPythia8 propagate.tcl pythia8.$I.cfg delphes.$I.root &> delphes.$I.log && \
	root -b -q -l "createO2tables.C(\"delphes.$I.root\", \"AODRun5.$I.root\", 0)" &> createO2tables.$I.log && \
#	rm -rf delphes.$I.root && \
	rm -rf .running.$I && \
	echo " --- complete run $I" &

done

### merge runs when all done
echo " --- waiting for runs to be completed "
wait
echo " --- all runs are processed, so long"
ls AODRun5.*.root >> listfiles.txt
