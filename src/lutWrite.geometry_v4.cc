#include "lutWrite.cc"

float scale = 1.;

void
fatInit_geometry_v4(float field = 0.5, float rmin = 100.)
{
  fat.SetBField(field);
  // new ideal Pixel properties?
  Double_t x0IB     = 0.001;
  Double_t x0OB     = 0.005;
  Double_t xrhoIB     = 1.1646e-02; // 50 mum Si
  Double_t xrhoOB     = 1.1646e-01; // 500 mum Si
  
  Double_t resRPhiIB     = 0.00025;
  Double_t resZIB        = 0.00025;
  Double_t resRPhiOB     = 0.00050;
  Double_t resZOB        = 0.00050;
  Double_t eff           = 0.98;
  fat.SetBField(0.5); // Tesla
  fat.SetIntegrationTime(100.e-6); // 100 ns (as in LoI)
  fat.SetMaxRadiusOfSlowDetectors(0.00001); // no slow detectors
  fat.SetAvgRapidity(0.0);
  fat.SetLhcUPCscale(1.);

  fat.AddLayer((char*)"vertex",           0.0,      0,        0); // dummy vertex for matrix calculat ion
  // fat.AddLayer((char*)"bpipe0",  0.48 * scale, 0.00042, 2.772e-02); // 150 mum Be
  
  fat.AddLayer((char*)"ddd0",     0.50 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"ddd1",     1.20 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  fat.AddLayer((char*)"ddd2",     2.50 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  
  // fat.AddLayer((char*)"bpipe1",   5.7 * scale,   0.0014, 9.24e-02); // 500 mum Be
  
  fat.AddLayer((char*)"ddd3",     7.00 * scale,  x0IB, xrhoIB, resRPhiIB, resZIB, eff);
  
  fat.AddLayer((char*)"ddd4",     10.0 * scale,  x0OB,     xrhoOB,   resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd5",     13.0 * scale,  x0OB,     xrhoOB,   resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd6",     16.0 * scale,  x0OB,     xrhoOB,   resRPhiOB, resZOB, eff);

  fat.AddLayer((char*)"ddd7",     25.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd8",     40.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddLayer((char*)"ddd9",     45.0 * scale,  x0OB, xrhoOB, resRPhiOB, resZOB, eff);
  fat.AddTPC(0.1, 0.1);

  fat.SetAtLeastHits(5);
  fat.SetAtLeastCorr(4);
  fat.SetAtLeastFake(0);
  //
  fat.SetMinRadTrack(rmin);
  //
  // fat.PrintLayout();
  // fat.SolveViaBilloir(0);
  // fat.MakeStandardPlots(0,2,2,"LutQA");
}

void
lutWrite_geometry_v4(const char *filename = "lutCovm.dat", int pdg = 211, float field = 0.5, float rmin = 100.)
{

  // init FAT
  fatInit_geometry_v4(field, rmin);
  // write
  lutWrite(filename, pdg, field);
  
}
  
