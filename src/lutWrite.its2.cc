/// @author Nicolo' Jacazio
/// @email: nicolo.jacazio@cern.ch
/// @since  30/10/2020
/// @file   lutWrite.its2.cc

#include "lutWrite.cc"

void fatInit_its2(float field = 0.5, float rmin = 100.) {
  fat.SetBField(field);
  fat.SetdNdEtaCent(400.);

  fat.AddLayer((char *)"bpipe", 2.0, 0.0022); // beam pipe
  fat.AddLayer((char *)"vertex", 0, 0); // dummy vertex for matrix calculation

  // new ideal Pixel properties?
  Double_t x0 = 0.0050;
  Double_t resRPhi = 0.0006;
  Double_t resZ = 0.0006;
  Double_t eff = 0.98;
  Double_t xrhoOB = 1.1646e-01; // 500 mum Si

  fat.AddLayer((char *)"ddd1", 2.3, x0, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd2", 3.1, x0, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd3", 3.9, x0, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd4", 19.4, x0, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd5", 24.7, x0, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd6", 35.3, x0, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd7", 40.5, x0, xrhoOB, resRPhi, resZ, eff);

  fat.AddTPC(0.1, 0.1); // TPC
  fat.SetAtLeastHits(4);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  fat.PrintLayout();
}

void lutWrite_its2(const char *filename = "lutCovm.dat", int pdg = 211,
                   float field = 0.2, float rmin = 20.) {

  // init FAT
  fatInit_its2(field, rmin);
  // write
  lutWrite(filename, pdg, field, rmin);
}
