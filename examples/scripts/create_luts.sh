#! /usr/bin/env bash

WHAT=default
FIELD=0.5
RMIN=100.

[ -z "$1" ] || WHAT=$1
[ -z "$2" ] || FIELD=$2
[ -z "$3" ] || RMIN=$3

cp    $DELPHESO2_ROOT/lut/lutWrite.$WHAT.cc . || { echo "cannot find lut writer: $DELPHESO2_ROOT/lut/lutWrite.$WHAT.cc" ; exit 1; }
cp    $DELPHESO2_ROOT/lut/DetectorK/DetectorK.cxx .
cp    $DELPHESO2_ROOT/lut/DetectorK/DetectorK.h .
cp -r $DELPHESO2_ROOT/lut/fwdRes .
cp    $DELPHESO2_ROOT/lut/lutWrite.cc .

echo " --- creating LUTs: config = $WHAT, field = $FIELD T, min tracking radius = $RMIN cm" 

prepare_libs_macro() {
    echo "void create_libs() {"
    echo "   gROOT->ProcessLine(\".L DetectorK.cxx+\");"
    echo "}"
}

prepare_luts_macro() {

    echo "R__LOAD_LIBRARY(DetectorK_cxx.so)"
    echo "#include \"lutWrite.$WHAT.cc\""
    echo "void create_luts() {"
    echo "   lutWrite_$WHAT(\"lutCovm.el.dat\",   11, $FIELD, $RMIN);"
    echo "   lutWrite_$WHAT(\"lutCovm.mu.dat\",   13, $FIELD, $RMIN);"
    echo "   lutWrite_$WHAT(\"lutCovm.pi.dat\",  211, $FIELD, $RMIN);"
    echo "   lutWrite_$WHAT(\"lutCovm.ka.dat\",  321, $FIELD, $RMIN);"
    echo "   lutWrite_$WHAT(\"lutCovm.pr.dat\", 2212, $FIELD, $RMIN);"
    echo "}"

}

prepare_libs_macro > create_libs.C
root -b -q -l create_libs.C

prepare_luts_macro > create_luts.C
root -b -q -l create_luts.C

