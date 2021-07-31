/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "lutWrite.cc"

float scale = 1.;

void
fatInit_v12(float field = 0.5, float rmin = 100.)
{
  fat.SetBField(field);
  // new ideal Pixel properties?
  Double_t x0IB     = 0.001;
  Double_t x0OB     = 0.01;
  Double_t xrhoIB     = 2.3292e-02; // 100 mum Si
  Double_t xrhoOB     = 2.3292e-01; // 1000 mum Si
  
  Double_t resRPhiIB     = 0.00025;
  Double_t resZIB        = 0.00025;
  Double_t resRPhiOB     = 0.00100;
  Double_t resZOB        = 0.00100;
  Double_t eff           = 0.98;
  fat.AddLayer((char*)"vertex",           0.0,      0,        0); // dummy vertex for matrix calculation
  fat.AddLayer((char*)"bpipe0",  0.48 * scale, 0.00042, 2.772e-02); // 150 mum Be
  //
  fat.AddLayer((char*)"B00",     0.50 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"B01",     1.20 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"B02",     2.50 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  //
  fat.AddLayer((char*)"bpipe1",   3.7 * scale,   0.0014, 9.24e-02); // 500 mum Be
  //
  fat.AddLayer((char*)"B03",     3.75 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B04",     7.00 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B05",     12.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B06",     20.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B07",     30.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B08",     45.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B09",     60.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B10",     80.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"B11",     100. * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.SetAtLeastHits(4);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  fat.PrintLayout();
}

void
lutWrite_v12(const char *filename = "lutCovm.dat", int pdg = 211, float field = 0.5, float rmin = 100.)
{

  // init FAT
  fatInit_v12(field, rmin);
  // write
  lutWrite(filename, pdg, field, 0, 0, 0, 1); // last flag: use parametrisation for forward resolution
  
}
  
