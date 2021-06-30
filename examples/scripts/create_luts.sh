#! /usr/bin/env bash

if [[ $* == *"-h"* ]]; then
    echo "Script to generate LUTs from LUT writer, arguments:"
    echo "1) tag of the LUT writer [default]"
    echo "2) Magnetic field in T [0.5]"
    echo "3) Minimum radius of the track in cm [100]"
    exit 0
fi

WHAT=default
FIELD=0.5
RMIN=100.

[ -z "$1" ] || WHAT=$1
[ -z "$2" ] || FIELD=$2
[ -z "$3" ] || RMIN=$3

cp    "$DELPHESO2_ROOT/lut/lutWrite.$WHAT.cc" . || { echo "cannot find lut writer: $DELPHESO2_ROOT/lut/lutWrite.$WHAT.cc" ; exit 1; }
cp    "$DELPHESO2_ROOT/lut/DetectorK/DetectorK.cxx" .
cp    "$DELPHESO2_ROOT/lut/DetectorK/DetectorK.h" .
cp -r "$DELPHESO2_ROOT/lut/fwdRes" .
cp    "$DELPHESO2_ROOT/lut/lutWrite.cc" .
cp    "$DELPHESO2_ROOT/lut/lutCovm.hh" .

echo " --- creating LUTs: config = $WHAT, field = $FIELD T, min tracking radius = $RMIN cm"

for i in 0 1 2 3 4; do
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
    const float rmin = ${RMIN}f;
    const int i = ${i};
    lutWrite_${WHAT}("lutCovm." + pn[i] + ".dat", pc[i], field, rmin);

EOF
done

# Checking that the output LUTs are OK
NullSize=""
for i in el mu pi ka pr; do
    if [[ ! -s lutCovm.$i.dat ]]; then
        echo "${i} has zero size"
        NullSize="${NullSize} ${i}"
    fi
done

if [[ ! -z $NullSize ]]; then
    echo "Created null sized LUTs!!"
    exit 1
fi
