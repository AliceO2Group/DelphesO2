R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

void muon(const char *inputFile = "delphes.root",
	  const char *inputFileAccMuonPID = "muonAccEffPID.root",
	  const char *outputFile = "muon.root") {
  
  // Create chain of root trees
  TChain chain("Delphes");
  chain.Add(inputFile);
  
  // Create object of class ExRootTreeReader
  auto treeReader = new ExRootTreeReader(&chain);
  auto numberOfEntries = treeReader->GetEntries();
  
  // Get pointers to branches used in this analysis
  auto events    = treeReader->UseBranch("Event");
  auto tracks    = treeReader->UseBranch("Track");
  auto particles = treeReader->UseBranch("Particle");

  // MID detector
  o2::delphes::MIDdetector mid;
  mid.setup(inputFileAccMuonPID);
  
  // smearer
  // o2::delphes::TrackSmearer smearer;
  // smearer.useEfficiency(true);
  // smearer.loadTable(11, "lutCovm.el.dat");
  // smearer.loadTable(13, "lutCovm.mu.dat");
  // smearer.loadTable(211, "lutCovm.pi.dat");
  // smearer.loadTable(321, "lutCovm.ka.dat");
  // smearer.loadTable(2212, "lutCovm.pr.dat");

  // logx binning
  const Int_t nbins = 80;
  double xmin = 1.e-2;
  double xmax = 1.e2;
  double logxmin = std::log10(xmin);
  double logxmax = std::log10(xmax);
  double binwidth = (logxmax - logxmin) / nbins;
  double xbins[nbins + 1];
  for (Int_t i=0; i<=nbins; ++i) xbins[i] = std::pow(10., logxmin + i * binwidth);
  
  auto hPtMuons = new TH1F("hPtMuons", "hPtMuons;#it{p_{T}} (GeV/#it{c});", nbins, xbins);
  auto hPtAll   = new TH1F("hPtAll",   "hPtAll;#it{p_{T}} (GeV/#it{c});", nbins, xbins);
 
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    
    // loop over tracks, smear and store MID tracks
    std::vector<Track *> mid_tracks;

    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {
      
      auto track = (Track*) tracks->At(itrack);

      // smear track
      // if (!smearer.smearTrack(*track)) continue;

      // check if has MID
      if (mid.hasMID(*track)) {
	hPtAll->Fill(track->PT);
	mid_tracks.push_back(track);
      }
      
    }

    // loop over MID tracks and do PID

    int multiplicity = mid_tracks.size();
    
    for (auto track : mid_tracks) {
      
      if (mid.isMuon(*track, multiplicity)) hPtMuons->Fill(track->PT);
      
    }

  }
    
  auto fout = TFile::Open(outputFile, "RECREATE");

  hPtMuons ->Write();
  hPtAll   ->Write();

  fout->Close();

}
