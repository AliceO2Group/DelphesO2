R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double Bz = 0.2; // [T]
double tof_radius = 100.; // [cm]
double tof_length = 200.; // [cm]
double tof_sigmat = 0.02; // [ns]

void
tof(const char *inputFile = "delphes.root",
   const char *outputFile = "tof.root")
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

  // TOF layer
  o2::delphes::TOFLayer toflayer;
  toflayer.setup(tof_radius, tof_length, tof_sigmat);
  
  // smearer
  o2::delphes::TrackSmearer smearer;
  smearer.useEfficiency(true);
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
  auto hTime0 = new TH1F("hTime0", ";t_{0} (ns)", 1000, -1., 1.);
  auto hBetaP = new TH2F("hBetaP", ";log_{10}(#it{p}/GeV);#beta", 400, -2., 2., 1000, 0.1, 1.1);
  TH2 *hNsigmaP[5];
  const char *pname[5] = {"el", "mu", "pi", "ka", "pr"};
  const char *plabel[5] = {"e", "#mu", "#pi", "K", "p"};
  for (int i = 0; i < 5; ++i)
    hNsigmaP[i] = new TH2F(Form("hNsigmaP_%s", pname[i]), Form(";log_{10}(#it{p}/GeV);n#sigma_{%s}", plabel[i]), 40, -2., 2., 200, -10., 10.);
    
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    
    // loop over tracks, smear and store TOF tracks
    std::vector<Track *> tof_tracks;
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();

      // smear track
      if (!smearer.smearTrack(*track)) continue;

      // select primaries based on 3 sigma DCA cuts
      if (fabs(track->D0 / track->ErrorD0) > 3.) continue;
      if (fabs(track->DZ / track->ErrorDZ) > 3.) continue;

      // check if has TOF
      if (!toflayer.hasTOF(*track)) continue;

      // push track
      tof_tracks.push_back(track);
      
    }

    // compute the event time
    std::array<float, 2> tzero;
    toflayer.eventTime(tof_tracks, tzero);
    hTime0->Fill(tzero[0]);

    // loop over tracks and do PID
    for (auto track : tof_tracks) {
    
      // fill beta-p
      auto p = track->P;
      auto beta = toflayer.getBeta(*track);
      hBetaP->Fill(log10(p), beta);
      
      // fill nsigma
      std::array<float, 5> deltat, nsigma;
      toflayer.makePID(*track, deltat, nsigma);
      for (int i = 0; i < 5; ++i)
	hNsigmaP[i]->Fill(log10(p), nsigma[i]);
      
    }
  }

  auto fout = TFile::Open(outputFile, "RECREATE");
  hTime0->Write();
  hBetaP->Write();
  for (int i = 0; i < 5; ++i)
    hNsigmaP[i]->Write();
  fout->Close();

}
