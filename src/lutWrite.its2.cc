/// @author Nicolo' Jacazio
/// @email: nicolo.jacazio@cern.ch
/// @since  30/10/2020
/// @file   lutWrite.its2.cc

#include "lutWrite.cc"

void fatInit_its2(float field = 0.5, float rmin = 100.)
{
  fat.SetBField(field);
  fat.SetdNdEtaCent(400.);

  const Double_t x0BP = 0.00224;     // 800 mum Be
  const Double_t xrhoBP = 1.4784e-1; // 800 mum Be

  fat.AddLayer((char*)"vertex", 0, 0);              // dummy vertex for matrix calculation
  fat.AddLayer((char*)"bpipe", 1.98, x0BP, xrhoBP); // beam pipe

  // X/X0 of the inner barrel, values taken from the TDR of ITS upgrade
  const Double_t x0IB = 0.0035; // X/X0 of the inner barrel (first three layers)
  // X/X0 of the outer barrel, values taken from the TDR of ITS upgrade
  const Double_t x0OB = 0.008; // X/X0 of the outer barrel
  // Resolution in Rphi values taken from A. Kalweit (table presented at the
  // Physics Forum on 24th Feb. 2021) the value is in the middle of 15 and 30
  // micron pitch
  const Double_t resRPhi = 0.0006;
  // Resolution in Z, values taken from A. Kalweit (table presented at the
  // Physics Forum on 24th Feb. 2021) the value is in the middle of 15 and 30
  // micron pitch
  const Double_t resZ = 0.0006;
  const Double_t eff = 0.98;            // Efficiency (lower limit)
  const Double_t xrhoIBOB = 1.1646e-01; // Surface density for 500 mum thick Si

  fat.AddLayer((char*)"il0", 2.3, x0IB, xrhoIBOB, resRPhi, resZ, eff);
  fat.AddLayer((char*)"il1", 3.1, x0IB, xrhoIBOB, resRPhi, resZ, eff);
  fat.AddLayer((char*)"il2", 3.9, x0IB, xrhoIBOB, resRPhi, resZ, eff);
  fat.AddLayer((char*)"ml3", 19.4, x0OB, xrhoIBOB, resRPhi, resZ, eff);
  fat.AddLayer((char*)"ml4", 24.7, x0OB, xrhoIBOB, resRPhi, resZ, eff);
  fat.AddLayer((char*)"ol5", 35.3, x0OB, xrhoIBOB, resRPhi, resZ, eff);
  fat.AddLayer((char*)"ol6", 40.5, x0OB, xrhoIBOB, resRPhi, resZ, eff);

  fat.AddTPC(0.1, 0.1); // TPC
  fat.SetAtLeastHits(4);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  fat.PrintLayout();
}

void lutWrite_its2(const char* filename = "lutCovm.dat", int pdg = 211,
                   float field = 0.2, float rmin = 20.)
{

  // init FAT
  fatInit_its2(field, rmin);
  // write
  lutWrite(filename, pdg, field);
}
