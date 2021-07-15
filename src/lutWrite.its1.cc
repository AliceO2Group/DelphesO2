/// @author Nicolo' Jacazio
/// @email: nicolo.jacazio@cern.ch
/// @since  30/10/2020
/// @file   lutWrite.its1.cc

#include "lutWrite.cc"

void fatInit_its1(float field = 0.5, float rmin = 100.)
{

  fat.SetBField(field);
  fat.SetdNdEtaCent(400.);
  fat.MakeAliceCurrent();
  fat.SetAtLeastHits(4);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  fat.PrintLayout();
}

void lutWrite_its1(const char* filename = "lutCovm.dat", int pdg = 211,
                   float field = 0.2, float rmin = 20.)
{

  // init FAT
  fatInit_its1(field, rmin);
  // write
  lutWrite(filename, pdg, field);
}
