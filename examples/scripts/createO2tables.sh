#! /usr/bin/env bash

### run configuration
NJOBS=4        # number of max parallel runs
NRUNS=10       # number of runs
NEVENTS=10000  # number of events in a run
DOANALYSIS=0   # run O2 analysis

### detector configuration
BFIELD=5.      # magnetic field  [kG]
SIGMAT=0.020   # time resolution [ns]
TOFRAD=100.    # TOF radius      [cm]
TOFLEN=200.    # TOF half length [cm]

### copy relevant files in the working directory
cp $DELPHESO2_ROOT/examples/cards/propagate.2kG.tcl propagate.tcl
cp $DELPHESO2_ROOT/examples/smearing/luts/lutCovm.* .
cp $DELPHESO2_ROOT/examples/pythia8/pythia8_ccbar.cfg .
cp $DELPHESO2_ROOT/examples/pythia8/decays/force_hadronic_D.cfg .
cp $DELPHESO2_ROOT/examples/aod/createO2tables.C .
cp $DELPHESO2_ROOT/examples/scripts/dpl-config_std.json .

### set magnetic field
sed -i -e "s/set Bz .*$/set Bz ${BFIELD}e\-1/" propagate.tcl
sed -i -e "s/double Bz = .*$/double Bz = ${BFIELD}e\-1\;/" createO2tables.C
sed -i -e "s/\"d_bz\": .*$/\"d_bz\": \"${BFIELD}\"\,/" dpl-config_std.json
### set TOF radius
sed -i -e "s/set Radius .*$/set Radius ${TOFRAD}e\-2/" propagate.tcl
sed -i -e "s/double tof_radius = .*$/double tof_radius = ${TOFRAD}\;/" createO2tables.C
### set TOF length
sed -i -e "s/set HalfLength .*$/set HalfLength ${TOFLEN}e\-2/" propagate.tcl
sed -i -e "s/double tof_length = .*$/double tof_length = ${TOFLEN}\;/" createO2tables.C
### set TOF time resolution
sed -i -e "s/set TimeResolution .*$/set TimeResolution ${SIGMAT}e\-9/" propagate.tcl
sed -i -e "s/double tof_sigmat = .*$/double tof_sigmat = ${SIGMAT}\;/" createO2tables.C

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
    DelphesPythia8 propagate.tcl pythia8.$I.cfg delphes.$I.root &> delphes.$I.log && \
	root -b -q -l "createO2tables.C(\"delphes.$I.root\", \"AODRun5.$I.root\")" &> createO2tables.$I.log && \
	rm -rf delphes.root && \
	rm -rf .running.$I && \
	echo " --- complete run $I" &

done

### merge runs when all done
wait
hadd -f AODRun5Tot.root AODRun5.*.root && rm -rf AODRun5.*.root

FILEOUTO2="AnalysisResults.root"
AOD3NAME=AODRun5Tot.root

### perform O2 analysis
if [ $DOANALYSIS -eq 1 ]; then
  LOGFILE="log_o2.log"
  echo -e "\nRunning the tasks with O2... (logfile: $LOGFILE)"
  rm -f $FILEOUTO2
  if [ ! -f "$AOD3NAME" ]; then
    echo "Error: File $AOD3NAME does not exist."
    exit 1
  fi
  O2ARGS="--shm-segment-size 16000000000 --configuration json://$PWD/dpl-config_std.json --aod-file $AOD3NAME"
  O2EXEC="o2-analysis-hftrackindexskimscreator $O2ARGS | o2-analysis-hfcandidatecreator2prong $O2ARGS | o2-analysis-taskdzero $O2ARGS -b"
  TMPSCRIPT="tmpscript.sh"
  cat << EOF > $TMPSCRIPT # Create a temporary script with the full O2 commands.
#!/bin/bash
$O2EXEC
EOF
  $ENVO2 bash $TMPSCRIPT # Run the script in the O2 environment.
  #$ENVO2 bash $TMPSCRIPT > $LOGFILE 2>&1 # Run the script in the O2 environment.
  #if [ ! $? -eq 0 ]; then echo "Error"; exit 1; fi # Exit if error.
  rm -f $TMPSCRIPT
fi

### clean
rm *.tcl *.cfg *.dat *.C
