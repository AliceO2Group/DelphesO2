R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double Bz = 0.2; // [T]
double emcal_radius = 100.; // [cm]
double emcal_length = 200.; // [cm]

void
emcal(const char *inputFile = "delphes.root",
      const char *outputFile = "emcal.root")
{
  
  // Create chain of root trees
  TChain chain("Delphes");
  chain.Add(inputFile);
  
  // Create object of class ExRootTreeReader
  auto treeReader = new ExRootTreeReader(&chain);
  auto numberOfEntries = treeReader->GetEntries();
  
  // Get pointers to branches used in this analysis
  auto events = treeReader->UseBranch("Event");
  auto tracks = treeReader->UseBranch("Track");
  auto neutrals = treeReader->UseBranch("Neutral");
  auto particles = treeReader->UseBranch("Particle");

  // histograms
  auto hE_el = new TH1F("hE_el", ";log_{10}(energy/GeV);", 400, -2., 2.);
  auto hE_ga = new TH1F("hE_ga", ";log_{10}(energy/GeV);", 400, -2., 2.);
  
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    
    // loop over charged tracks
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();

      // only electron tracks
      if (particle->PID != 11) continue;
      
      // check if has hit the calorimeter
      auto x = track->XOuter * 0.1; // [cm]
      auto y = track->YOuter * 0.1; // [cm]
      auto z = track->ZOuter * 0.1; // [cm]
      auto hasHit = fabs(hypot(x, y) - emcal_radius) < 0.001 && fabs(z) < emcal_length;
      if (!hasHit) continue;

      // get energy, smear and fill histogram
      auto E = track->P4().Energy();
      auto Ee = 0.01 + 0.05 / sqrt(E);
      E += gRandom->Gaus(0., E * Ee);
      hE_el->Fill(log10(E));
      
    }

    // loop over neutral tracks
    for (Int_t itrack = 0; itrack < neutrals->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)neutrals->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();

      // only photon tracks
      if (particle->PID != 22) continue;
      
      // check if has hit the calorimeter
      auto x = track->XOuter * 0.1; // [cm]
      auto y = track->YOuter * 0.1; // [cm]
      auto z = track->ZOuter * 0.1; // [cm]
      auto hasHit = fabs(hypot(x, y) - emcal_radius) < 0.001 && fabs(z) < emcal_length;
      if (!hasHit) continue;

      // get energy, smear and fill histogram
      auto E = track->P4().Energy();
      auto Ee = 0.01 + 0.05 / sqrt(E);
      E += gRandom->Gaus(0., E * Ee);
      hE_ga->Fill(log10(E));
      
    }

  }

  auto fout = TFile::Open(outputFile, "RECREATE");
  hE_el->Write();
  hE_ga->Write();  
  fout->Close();
  
}
