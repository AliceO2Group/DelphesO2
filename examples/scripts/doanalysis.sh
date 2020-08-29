FILEOUTQA="AnalysisResultsQA.root"
FILEOUTO2="AnalysisResults.root"
DOQA=1
DOANALYSIS=0

cp $DELPHESO2_ROOT/examples/scripts/dpl-config_std.json .

### perform QA analysis
if [ $DOQA -eq 1 ]; then
  LOGFILE="log_qa.log"
  echo -e "\nRunning the tasks with O2... (logfile: $LOGFILE)"
  rm -f $FILEOUTQA
  O2ARGS="--shm-segment-size 16000000000 --configuration json://$PWD/dpl-config_std.json --aod-file @listfiles.txt"
  O2EXEC="o2-analysis-qatask --pipeline o2-analysis-qatask:100 $O2ARGS -b"
  TMPSCRIPT="tmpscript.sh"
  cat << EOF > $TMPSCRIPT # Create a temporary script with the full O2 commands.
#!/bin/bash
$O2EXEC
EOF
  $ENVO2 bash $TMPSCRIPT > $LOGFILE 2>&1 # Run the script in the O2 environment.
  rm -f $TMPSCRIPT
  mv AnalysisResults.root $FILEOUTQA
fi

### perform O2 analysis
if [ $DOANALYSIS -eq 1 ]; then
  LOGFILE="log_o2.log"
  echo -e "\nRunning the tasks with O2... (logfile: $LOGFILE)"
  rm -f $FILEOUTO2
  O2ARGS="--shm-segment-size 16000000000 --configuration json://$PWD/dpl-config_std.json --aod-file @listfiles.txt "
  O2EXEC="o2-analysis-hftrackindexskimscreator $O2ARGS --nreaders 100 --pipeline o2-analysis-hftrackindexskimscreator:100,o2-analysis-hfcandidatecreator2prong:100,o2-analysis-taskdzero:100 -b"
  TMPSCRIPT="tmpscript.sh"
  cat << EOF > $TMPSCRIPT # Create a temporary script with the full O2 commands.
#!/bin/bash
$O2EXEC
EOF
  $ENVO2 bash $TMPSCRIPT > $LOGFILE 2>&1 # Run the script in the O2 environment.
  rm -f $TMPSCRIPT
fi
