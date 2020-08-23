#! /usr/bin/env bash

NRUNS=1
NEVENTS=10000
DOANALYSIS=1

BFIELD=5.    # [kG]
SIGMAT=0.020 # [ns]

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
### set time resolution
sed -i -e "s/set TimeResolution .*$/set TimeResolution ${SIGMAT}e\-9/" propagate.tcl

### loop over runs
for I in $(seq 1 $NRUNS); do

    ### copy pythia8 configuration and adjust it
    cp pythia8_ccbar.cfg pythia8.cfg
    echo "Main:numberOfEvents $NEVENTS" >> pythia8.cfg
    echo "Random:seed = $I" >> pythia8.cfg

    ### force hadronic D decays
    cat force_hadronic_D.cfg >> pythia8.cfg
    
    ### run Delphes and analysis
    DelphesPythia8 propagate.tcl pythia8.cfg delphes.root &&
	root -b -q -l "createO2tables.C(\"delphes.root\", \"AODRun5.$I.root\")" &&
	rm -rf delphes.root

done

### merge runs
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
