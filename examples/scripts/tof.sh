#! /usr/bin/env bash

NJOBS=5        # number of max parallel runs
NRUNS=10       # number of runs
NEVENTS=10000  # number of events in a run

BFIELD=5.      # magnetic field  [kG]
SIGMAT=0.020   # time resolution [ns]
TAILLX=1.0     # tail on left    [q]
TAILRX=1.3     # tail on right   [q]
TOFRAD=100.    # TOF radius      [cm]
TOFLEN=200.    # TOF half length [cm]
TOFETA=1.443   # TOF max pseudorapidity

### calculate max eta from geometry
TOFETA=`awk -v a=$TOFRAD -v b=$TOFLEN 'BEGIN {th=atan2(a,b)*0.5; sth=sin(th); cth=cos(th); print -log(sth/cth)}'`
echo "maxEta = $TOFETA"

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tails.tcl propagate.tcl
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_inel.cfg .
cp $DELPHESO2_ROOT/examples/smearing/tof.C .

### set magnetic field
sed -i -e "s/set barrel_Bz .*$/set barrel_Bz ${BFIELD}e\-1/" propagate.tcl
### set TOF radius
sed -i -e "s/set barrel_Radius .*$/set barrel_Radius ${TOFRAD}e\-2/" propagate.tcl
sed -i -e "s/double tof_radius = .*$/double tof_radius = ${TOFRAD}\;/" tof.C
### set TOF length
sed -i -e "s/set barrel_HalfLength .*$/set barrel_HalfLength ${TOFLEN}e\-2/" propagate.tcl
sed -i -e "s/double tof_length = .*$/double tof_length = ${TOFLEN}\;/" tof.C
### set TOF acceptance
sed -i -e "s/set barrel_Acceptance .*$/set barrel_Acceptance \{ 0.0 + 1.0 * fabs(eta) < ${TOFETA} \}/" propagate.tcl
### set TOF time resolution and tails
sed -i -e "s/set barrel_TimeResolution .*$/set barrel_TimeResolution ${SIGMAT}e\-9/" propagate.tcl
sed -i -e "s/set barrel_TailRight .*$/set barrel_TailRight ${TAILRX}/" propagate.tcl
sed -i -e "s/set barrel_TailLeft  .*$/set barrel_TailLeft ${TAILLX}/" propagate.tcl
sed -i -e "s/double tof_sigmat = .*$/double tof_sigmat = ${SIGMAT}\;/" tof.C

### create LUTs
BFIELDT=`awk -v a=$BFIELD 'BEGIN {print a*0.1}'`
$DELPHESO2_ROOT/examples/scripts/create_luts.sh werner $BFIELDT $TOFRAD

### loop over runs
rm -f .running.* delphes.*.root
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
