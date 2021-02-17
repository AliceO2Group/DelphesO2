/// @author Nicolo' Jacazio
/// @email: nicolo.jacazio@cern.ch
/// @since  30/10/2020
/// @file   lutWrite.its3.cc

#include "lutWrite.cc"

// Adds foam spacers between inner layers
const bool add_foam = true;
// Puts foam spacers mid-way betwen layers
const bool foam_middle = false;

void fatInit_its3(float field = 0.5, float rmin = 100.) {
  fat.SetBField(field);
  fat.SetdNdEtaCent(400.);

  Double_t x0BP = 0.0014;     // 500 mum Be
  Double_t xrhoBP = 9.24e-02; // 500 mum Be

  // dummy vertex for matrix calculation
  fat.AddLayer((char *)"vertex", 0.0, 0, 0);
  fat.AddLayer((char *)"bpipe", 1.625, x0BP, xrhoBP); // 500 mum Be beam pipe

  Double_t x0IB = 0.0005;       // X/X0 of the inner barrel (first three layers)
  Double_t x0OB = 0.008;        // X/X0 of the outer barrel
  Double_t resRPhi = 0.0006;    // Resolution in Rphi
  Double_t resZ = 0.0006;       // Resolution in Z
  Double_t eff = 0.98;          // Efficiency
  Double_t xrhoIB = 1.1646e-02; // Surface density for 50 mum thick Si
  Double_t xrhoOB = 1.1646e-01; // Surface density for 500 mum thick Si

  const Double_t x0Foam = 0.0008;   // X0 ~710cm for 0.6cm thick foam
  float foam_radius = 9.370 * x0IB; // width of a chip

  fat.AddLayer((char *)"ddd1", 1.8, x0IB, xrhoIB, resRPhi, resZ, eff);
  if (foam_middle)
    foam_radius = (2.4 - 1.8) / 2;
  if (add_foam)
    fat.AddLayer((char *)"foam1", 1.8 + foam_radius, x0Foam); // Foam spacer
  fat.AddLayer((char *)"ddd2", 2.4, x0IB, xrhoIB, resRPhi, resZ, eff);
  if (foam_middle)
    foam_radius = (3.0 - 2.4) / 2;
  if (add_foam)
    fat.AddLayer((char *)"foam2", 2.4 + foam_radius, x0Foam); // Foam spacer
  fat.AddLayer((char *)"ddd3", 3.0, x0IB, xrhoIB, resRPhi, resZ, eff);
  if (foam_middle)
    foam_radius = (19.4 - 3.0) / 2;
  if (add_foam)
    fat.AddLayer((char *)"foam3", 3.0 + foam_radius, x0Foam); // Foam spacer
  // Structural cylinder?
  fat.AddLayer((char *)"ddd4", 19.4, x0OB, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd5", 24.7, x0OB, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd6", 35.3, x0OB, xrhoOB, resRPhi, resZ, eff);
  fat.AddLayer((char *)"ddd7", 40.5, x0OB, xrhoOB, resRPhi, resZ, eff);

  fat.AddTPC(0.1, 0.1); // TPC
  fat.SetAtLeastHits(4);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  fat.PrintLayout();
}

void lutWrite_its3(const char *filename = "lutCovm.dat", int pdg = 211,
                   float field = 0.2, float rmin = 20.) {

  // init FAT
  fatInit_its3(field, rmin);
  // write
  lutWrite(filename, pdg, field);
}
