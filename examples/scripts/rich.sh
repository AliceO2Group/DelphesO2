#! /usr/bin/env bash

NJOBS=5                # number of max parallel runs
NRUNS=5                # number of runs
NEVENTS=10000          # number of events in a run

LUTTAG="werner"        # LUT tag name
PY8CFG="pythia8_ccbar" # pythia8 configuration
BFIELD=5.              # magnetic field   [kG]
RICHRAD=100.           # RICH radius      [cm]
RICHLEN=200.           # RICH half length [cm]
RICHETA=1.443          # RICH max pseudorapidity

### calculate max eta from geometry
RICHETA=`awk -v a=$RICHRAD -v b=$RICHLEN 'BEGIN {th=atan2(a,b)*0.5; sth=sin(th); cth=cos(th); print -log(sth/cth)}'`
echo "maxEta = $RICHETA"

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tcl propagate.tcl
cp $DELPHESO2_ROOT/examples/pythia8/$PY8CFG.cfg pythia8.cfg
cp $DELPHESO2_ROOT/examples/smearing/rich.C .

### adjust pythia8 configuration
echo "" >> pythia8.cfg
echo "### run time configuration" >> pythia8.cfg
echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
echo "Beams:allowVertexSpread off " >> pythia8.cfg
echo "Beams:sigmaTime 60." >> pythia8.cfg

### set magnetic field
sed -i -e "s/set barrel_Bz .*$/set barrel_Bz ${BFIELD}e\-1/" propagate.tcl
### set TOF radius
sed -i -e "s/set barrel_Radius .*$/set barrel_Radius ${RICHRAD}e\-2/" propagate.tcl
sed -i -e "s/double rich_radius = .*$/double rich_radius = ${RICHRAD}\;/" rich.C
### set TOF length
sed -i -e "s/set barrel_HalfLength .*$/set barrel_HalfLength ${RICHLEN}e\-2/" propagate.tcl
sed -i -e "s/double rich_length = .*$/double rich_length = ${RICHLEN}\;/" rich.C
### set TOF acceptance
sed -i -e "s/set barrel_Acceptance .*$/set barrel_Acceptance \{ 0.0 + 1.0 * fabs(eta) < ${RICHETA} \}/" propagate.tcl

### create LUTs
BFIELDT=`awk -v a=$BFIELD 'BEGIN {print a*0.1}'`
$DELPHESO2_ROOT/examples/scripts/create_luts.sh $LUTTAG $BFIELDT $TOFRAD

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
    cp pythia8.cfg pythia8.$I.cfg
    echo "Random:setSeed on" >> pythia8.$I.cfg
    echo "Random:seed = $I" >> pythia8.$I.cfg
    
    ### run Delphes and analysis    
    DelphesPythia8 propagate.tcl pythia8.$I.cfg delphes.$I.root  &> delphes.$I.log &&
	root -b -q -l "rich.C(\"delphes.$I.root\", \"rich.$I.root\")" &> rich.$I.log &&
	rm -rf delphes.$I.root &&
	rm -rf .running.$I &&
	echo " --- complete run $I" &
    
done

### merge runs when all done
wait
hadd -f rich.root rich.*.root && rm -rf rich.*.root
