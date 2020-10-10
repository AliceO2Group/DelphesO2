/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "lutWrite.cc"

void
fatInit_scenario3(float field = 0.5, float rmin = 100.)
{
  fat.SetBField(field);
  fat.SetdNdEtaCent(400.);
  // new ideal Pixel properties?
  Double_t x0IB     = 0.0005;
  Double_t x0OB     = 0.005;
  Double_t xrhoIB   = 1.1646e-02; // 50 mum Si
  Double_t xrhoOB   = 1.1646e-01; // 500 mum Si

  Double_t x0BP     = 0.0014; // 500 mum Be
  Double_t xrhoBP   = 9.24e-02; // 500 mum Be
  Double_t x0BF     = 0.0004; // 150 mum Be
  Double_t xrhoBF   = 2.77e-02; // 150 mum Be
  
  Double_t resRPhiIB     = 0.0001;
  Double_t resZIB        = 0.0001;
  Double_t resRPhiOB     = 0.0005;
  
  Double_t resZOB        = 0.0005;
  Double_t eff           = 0.98;
  fat.AddLayer((char*)"vertex", 0.0,     0,      0); // dummy vertex for matrix calculation
  fat.AddLayer((char*)"bfoil",  0.5,     x0BF, xrhoBF);
  fat.AddLayer((char*)"ddd0",   0.5075,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"ddd1",   1.5,     x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"ddd2",   2.8,     x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"ddd3",   3.8,     x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"bpipe",  3.9,     x0BP, xrhoBP); // 500 mum Be
  //
  fat.AddLayer((char*)"ddd3a",  8.0,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd4",   20.,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd5",   25.,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd7",   40.,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd8",   55.,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"dddY",   80.,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"dddX",  100.,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.SetAtLeastHits(4);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  fat.PrintLayout();
}

void
lutWrite_scenario3(const char *filename = "lutCovm.dat", int pdg = 211, float field = 0.2, float rmin = 20.)
{

  // init FAT
  fatInit_scenario3(field, rmin);
  // write
  lutWrite(filename, pdg, field, rmin);
  
}
