R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

bool smear = true;
bool nsigma = true;
double Bz = 0.2;

bool hasStrangeAncestor(GenParticle *particle, TClonesArray *particles)
{
  auto imother = particle->M1;
  if (imother == -1) return false;
  auto mother = (GenParticle *)particles->At(imother);
  auto pid = mother->PID;

  switch (abs(pid)) {
  case 310:  // K0s
  case 3122: // Lambda
  case 3112: // Sigma-
  case 3222: // Sigma+
  case 3312: // Xi-
  case 3334: // Omega-
    return true;
  }

  return hasStrangeAncestor(mother, particles);
}

bool hasHeavyAncestor(GenParticle *particle, TClonesArray *particles)
{
  auto imother = particle->M1;
  if (imother == -1) return false;
  auto mother = (GenParticle *)particles->At(imother);
  auto pid = mother->PID;

  switch (abs(pid)) {
  case 411:  // D+
  case 421:  // D0
  case 431:  // Ds+
  case 4122: // Lambdac+
  case 4132: // Xic0
  case 4232: // Xic+
  case 511:  // B0
  case 521:  // B+
  case 531:  // Bs0
  case 541:  // Bc+
  case 5122: // Lambdab0
  case 5132: // Xib-
  case 5232: // Xib0
  case 5332: // Omegab-
    return true;
  }

  return hasHeavyAncestor(mother, particles);
}

void
dca(const char *inputFile = "delphes.root",
    const char *outputFile = "dca.root")
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
  auto particles = treeReader->UseBranch("Particle");
  
  // smearer
  o2::delphes::TrackSmearer smearer;
  if (Bz == 0.2) {
    smearer.loadTable(11, "lutCovm.el.2kG.dat");
    smearer.loadTable(13, "lutCovm.mu.2kG.dat");
    smearer.loadTable(211, "lutCovm.pi.2kG.dat");
    smearer.loadTable(321, "lutCovm.ka.2kG.dat");
    smearer.loadTable(2212, "lutCovm.pr.2kG.dat");
  } else if (Bz == 0.5) {
    smearer.loadTable(11, "lutCovm.el.5kG.dat");
    smearer.loadTable(13, "lutCovm.mu.5kG.dat");
    smearer.loadTable(211, "lutCovm.pi.5kG.dat");
    smearer.loadTable(321, "lutCovm.ka.5kG.dat");
    smearer.loadTable(2212, "lutCovm.pr.5kG.dat");
  } else {
    std::cout << " --- invalid Bz field: " << Bz << std::endl;
    return;
  }
  
  // histograms
  std::string title = ";log_{10} (#it{p}_{T} / GeV);d_{0} (mm)";
  if (nsigma) title = ";log_{10} (#it{p}_{T} / GeV);n#sigma_{d_{0}}";
  auto hDCAxy = new TH2F("hDCAxy", title.c_str(), 40, -2., 2., 1000, -100., 100.);
  auto hDCAxy_primary = new TH2F("hDCAxy_primary", title.c_str(), 40, -2., 2., 1000, -100., 100.);
  auto hDCAxy_secondary = new TH2F("hDCAxy_secondary", title.c_str(), 40, -2., 2., 1000, -100., 100.);
  auto hDCAxy_secondary_strange = new TH2F("hDCAxy_secondary_strange", title.c_str(), 40, -2., 2., 1000, -100., 100.);
  auto hDCAxy_secondary_heavy = new TH2F("hDCAxy_secondary_heavy", title.c_str(), 40, -2., 2., 1000, -100., 100.);

  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    
    // loop over tracks
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();
      auto imother = particle->M1;
      auto mother = imother != -1 ? (GenParticle *)particles->At(imother) : (GenParticle *)nullptr;

      // smear track if requested
      if (smear)
	if (!smearer.smearTrack(*track)) continue;
	  

      // look only at pions
      if (abs(particle->PID) != 211) continue;

      auto D0 = track->D0;
      if (nsigma) D0 /= track->ErrorD0;
      
      // fill histograms
      hDCAxy->Fill(log10(track->PT), D0);

      if (!hasStrangeAncestor(particle, particles) && !hasHeavyAncestor(particle, particles)) {
	hDCAxy_primary->Fill(log10(track->PT), D0);
      }
      else {
	hDCAxy_secondary->Fill(log10(track->PT), D0);

	if (hasHeavyAncestor(particle, particles))	    
	  hDCAxy_secondary_heavy->Fill(log10(track->PT), D0);
	else if (hasStrangeAncestor(particle, particles))
	  hDCAxy_secondary_strange->Fill(log10(track->PT), D0);
	else {
	  std::cout << particle->X << " " << particle->Y << " " << particle->Z << std::endl;
	  std::cout << " --- mother PDG: " << mother->PID << std::endl;
	}	
      }
    }
  }

  auto c = new TCanvas("c", "c", 800, 800);
  hDCAxy->Draw();
  
  auto fout = TFile::Open(outputFile, "RECREATE");
  hDCAxy->Write();
  hDCAxy_primary->Write();
  hDCAxy_secondary->Write();
  hDCAxy_secondary_strange->Write();
  hDCAxy_secondary_heavy->Write();
  fout->Close();
  
}
