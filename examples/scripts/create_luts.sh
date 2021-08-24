#! /usr/bin/env bash

# Configuration variables
WHAT=default
FIELD=0.5
RMIN=100.
WRITER_PATH=${DELPHESO2_ROOT}/lut/
OUT_PATH=.
OUT_TAG=
PARALLEL_JOBS=1
PARTICLES="0 1 2 3 4"
AUTOTAG="Yes"
VERBOSE=

# List of arguments expected in the input
optstring=":ht:B:R:p:o:T:P:j:vF"
# Get the options
while getopts ${optstring} option; do
    case ${option} in
    h) # display Help
        echo "Script to generate LUTs from LUT writer, arguments:"
        echo "Syntax: ./create_luts.sh [${optstring}]"
        echo "options:"
        echo "-t tag of the LUT writer [default]"
        echo "-B Magnetic field in T [0.5]"
        echo "-R Minimum radius of the track in cm [100]"
        echo "-p Path where the LUT writers are located [\$DELPHESO2_ROOT/lut/]"
        echo "-o Output path where to write the LUTs [.]"
        echo "-T Tag to append to LUTs [\"\"]"
        echo "-P Particles to consider [\"0 1 2 3 4\"]"
        echo "-j Number of parallel processes to use [1]"
        echo "-F Don't use the automatic tagging and use only the one provided instead for the naming of the output files"
        echo "-v Verbose mode"
        echo "-h Show this help"
        exit 0
        ;;
    t)
        WHAT=$OPTARG
        echo " > Setting LUT writer to ${WHAT}"
        ;;
    B)
        FIELD=$OPTARG
        echo " > Setting B field to ${FIELD} T"
        ;;
    R)
        RMIN=$OPTARG
        echo " > Setting minimum radius to ${RMIN} cm"
        ;;
    p)
        WRITER_PATH=$OPTARG
        echo " > Setting LUT writer path to ${WRITER_PATH}"
        ;;
    o)
        OUT_PATH=$OPTARG
        echo " > Setting LUT output path to ${OUT_PATH}"
        ;;
    T)
        OUT_TAG=$OPTARG
        echo " > Setting LUT output tag to ${OUT_TAG}"
        ;;
    P)
        PARTICLES=$OPTARG
        echo " > Setting LUT particles to ${PARTICLES}"
        ;;
    j)
        PARALLEL_JOBS=$OPTARG
        echo " > Setting parallel jobs to ${PARALLEL_JOBS}"
        ;;
    T)
        AUTOTAG=
        echo " > Disabling autotagging mode"
        ;;
    v)
        VERBOSE="Yes"
        echo " > Enabling verbose mode"
        ;;
    \?) # Invalid option
        echo "$0: Error: Invalid option, use [${optstring}]"
        exit
        ;;
    :) # Empty argument
        echo "$0: Error: ust supply an argument to -$OPTARG." >&2
        exit 1
        ;;
    esac
done

if [[ -n ${AUTOTAG} ]]; then
    # Defining the output tag based on the input
    FIELDT=$(echo "${FIELD}*10" | bc)
    FIELDT=${FIELDT%.0}
    OUT_TAG=".${FIELDT}kG.rmin${RMIN}.${WHAT}${OUT_TAG}"
fi

if [[ -n ${VERBOSE} ]]; then
    echo "WHAT='${WHAT}'"
    echo "FIELD='${FIELD}'"
    echo "RMIN='${RMIN}'"
    echo "WRITER_PATH='${WRITER_PATH}'"
    echo "OUT_PATH='${OUT_PATH}'"
    echo "OUT_TAG='${OUT_TAG}'"
    echo "PARALLEL_JOBS='${PARALLEL_JOBS}'"
    echo "PARTICLES='${PARTICLES}'"
    echo "AUTOTAG='${AUTOTAG}'"
fi

if [[ -z ${WRITER_PATH} ]]; then
    echo "Path of the LUT writers not defined, cannot continue"
    exit 1
fi

function do_copy() {
    cp "${1}" . || {
        echo "Cannot find $2: ${1}"
        exit 1
    }
}

do_copy "${WRITER_PATH}/lutWrite.$WHAT.cc" "lut writer"
do_copy "${WRITER_PATH}/DetectorK/HistoManager.cxx"
do_copy "${WRITER_PATH}/DetectorK/HistoManager.h"
do_copy "${WRITER_PATH}/DetectorK/DetectorK.cxx"
do_copy "${WRITER_PATH}/DetectorK/DetectorK.h"
do_copy "${WRITER_PATH}/lutWrite.cc"
if [[ -z ${DELPHESO2_ROOT} ]]; then
    do_copy "${WRITER_PATH}/lutCovm.hh"
fi
cp -r "${WRITER_PATH}/fwdRes" .

echo " --- creating LUTs: config = ${WHAT}, field = ${FIELD} T, min tracking radius = ${RMIN} cm"

function do_lut_for_particle() {
    i=${1}
    root -l -b <<EOF
    .L HistoManager.cxx+
    .L DetectorK.cxx+
    .L lutWrite.${WHAT}.cc

    TDatabasePDG::Instance()->AddParticle("deuteron", "deuteron", 1.8756134, kTRUE, 0.0, 3, "Nucleus", 1000010020);
    TDatabasePDG::Instance()->AddAntiParticle("anti-deuteron", -1000010020);

    TDatabasePDG::Instance()->AddParticle("triton", "triton", 2.8089218, kTRUE, 0.0, 3, "Nucleus", 1000010030);
    TDatabasePDG::Instance()->AddAntiParticle("anti-triton", -1000010030);

    TDatabasePDG::Instance()->AddParticle("helium3", "helium3", 2.80839160743, kTRUE, 0.0, 6, "Nucleus", 1000020030);
    TDatabasePDG::Instance()->AddAntiParticle("anti-helium3", -1000020030);

    const int N = 8;
    const TString pn[N] = {"el", "mu", "pi", "ka", "pr", "de", "tr", "he3"};
    const int pc[N] = {11, 13, 211, 321, 2212, 1000010020, 1000010030, 1000020030 };
    const float field = ${FIELD};
    const float rmin = ${RMIN};
    const int i = ${i};
    const TString out_file = "${OUT_PATH}/lutCovm." + pn[i] + "${OUT_TAG}.dat";
    Printf("Creating LUT for particle ID %i: %s with pdg code %i to %s", i, pn[i].Data(), pc[i], out_file.Data());
    if(i >= 0 && i < N){
        lutWrite_${WHAT}(out_file, pc[i], field, rmin);
    } else{
        Printf("Particle ID %i is too large or too small", i);
    }

EOF

}

root -l -b <<EOF
    .L HistoManager.cxx+
    .L DetectorK.cxx+
EOF

N_RAN=0
for i in ${PARTICLES}; do
    ((N_RAN = N_RAN % PARALLEL_JOBS))
    ((N_RAN++ == 0)) && wait
    do_lut_for_particle "${i}" &
    ((N_RAN + 1))
done

wait

# Checking that the output LUTs are OK
NullSize=""
P=(el mu pi ka pr de tr he3)
for i in $PARTICLES; do
    LUT_FILE=${OUT_PATH}/lutCovm.${P[$i]}${OUT_TAG}.dat
    if [[ ! -s ${LUT_FILE} ]]; then
        echo "LUT file ${LUT_FILE} for particle ${i} has zero size"
        NullSize="${NullSize} ${i}"
    else
        echo "${LUT_FILE} is ok"
    fi
done

if [[ -n $NullSize ]]; then
    echo "Created null sized LUTs!!"
    exit 1
fi
