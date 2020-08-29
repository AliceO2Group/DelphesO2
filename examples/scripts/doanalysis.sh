FILEOUTO2="AnalysisResults.root"
DOANALYSIS=1

cp $DELPHESO2_ROOT/examples/scripts/dpl-config_std.json .

### perform O2 analysis
if [ $DOANALYSIS -eq 1 ]; then
  LOGFILE="log_o2.log"
  echo -e "\nRunning the tasks with O2... (logfile: $LOGFILE)"
  rm -f $FILEOUTO2
  if [ ! -f "$AOD3NAME" ]; then
    echo "Error: File $AOD3NAME does not exist."
    #exit 1
  fi
  O2ARGS="--shm-segment-size 16000000000 --configuration json://$PWD/dpl-config_std.json --aod-file @listfiles.txt "
  #O2EXEC="o2-analysis-hftrackindexskimscreator $O2ARGS | o2-analysis-hfcandidatecreator2prong $O2ARGS | o2-analysis-taskdzero $O2ARGS | o2-analysis-qatask $AOD3NAME -b"
  O2EXEC="o2-analysis-hftrackindexskimscreator $O2ARGS --pipeline o2-analysis-hftrackindexskimscreator:30,o2-analysis-hfcandidatecreator2prong:30,o2-analysis-taskdzero:30 -b"
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

