#! /usr/bin/env bash

WHAT=default
FIELD=0.5
RMIN=100.
WRITER_PATH=$DELPHESO2_ROOT/lut/
OUT_PATH=.
OUT_TAG=
PARTICLES="0 1 2 3 4"

# Get the options
while getopts ":t:B:R:p:o:T:P:h" option; do
    case $option in
    h) # display Help
        echo "Script to generate LUTs from LUT writer, arguments:"
        echo "Syntax: ./create_luts.sh [-h|t|B|R|p|o|T|P]"
        echo "options:"
        echo "-t tag of the LUT writer [default]"
        echo "-B Magnetic field in T [0.5]"
        echo "-R Minimum radius of the track in cm [100]"
        echo "-p Path where the LUT writers are located [\$DELPHESO2_ROOT/lut/]"
        echo "-o Output path where to write the LUTs [.]"
        echo "-T Tag to append to LUTs [\"\"]"
        echo "-P Particles to consider [\"0 1 2 3 4\"]"
        echo "-h Show this help"
        exit
        ;;
    t)
        WHAT=$OPTARG
        echo " > Setting LUT writer to ${WHAT}"
        ;;
    B)
        FIELD=$OPTARG
        echo " > Setting B field to ${FIELD}"
        ;;
    R)
        RMIN=$OPTARG
        echo " > Setting minimum radius to ${RMIN}"
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
    \?) # Invalid option
        echo "Error: Invalid option, use [-h|t|B|R|p|o|T|P]"
        exit
        ;;
    esac
done

cp    "${WRITER_PATH}/lutWrite.$WHAT.cc" . || { echo "cannot find lut writer: ${WRITER_PATH}/lutWrite.$WHAT.cc" ; exit 1; }
cp    "${WRITER_PATH}/DetectorK/DetectorK.cxx" .
cp    "${WRITER_PATH}/DetectorK/DetectorK.h" .
cp -r "${WRITER_PATH}/fwdRes" .
cp    "${WRITER_PATH}/lutWrite.cc" .

echo " --- creating LUTs: config = ${WHAT}, field = ${FIELD} T, min tracking radius = ${RMIN} cm"

for i in $PARTICLES; do
    root -l -b <<EOF
    .L DetectorK.cxx+
    .L lutWrite.${WHAT}.cc

    TDatabasePDG::Instance()->AddParticle("deuteron", "deuteron", 1.8756134, kTRUE, 0.0, 3, "Nucleus", 1000010020);
    TDatabasePDG::Instance()->AddAntiParticle("anti-deuteron", -1000010020);

    TDatabasePDG::Instance()->AddParticle("helium3", "helium3", 2.80839160743, kTRUE, 0.0, 6, "Nucleus", 1000020030);
    TDatabasePDG::Instance()->AddAntiParticle("anti-helium3", -1000020030);

    const TString pn[7] = {"el", "mu", "pi", "ka", "pr", "de", "he3"};
    const int pc[7] = {11, 13, 211, 321, 2212, 1000010020, 1000020030 };
    const float field = ${FIELD}f;
    const float rmin = ${RMIN};
    const int i = ${i};
    lutWrite_${WHAT}("${OUT_PATH}/lutCovm." + pn[i] + "${OUT_TAG}.dat", pc[i], field, rmin);

EOF
done

# Checking that the output LUTs are OK
NullSize=""
P=(el mu pi ka pr de he3)
for i in $PARTICLES; do
    if [[ ! -s lutCovm.${P[$i]}.dat ]]; then
        echo "${i} has zero size"
        NullSize="${NullSize} ${i}"
    else
        echo "lutCovm.${P[$i]}.dat is ok"
    fi
done

if [[ -n $NullSize ]]; then
    echo "Created null sized LUTs!!"
    exit 1
fi
