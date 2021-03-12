R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double tof_radius = 100.; // [cm]
double tof_length = 200.; // [cm]
double tof_sigmat = 0.02; // [ns]
double tof_sigma0 = 0.20; // [ns]

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
  toflayer.setup(tof_radius, tof_length, tof_sigmat, tof_sigma0);
  
  // smearer
  o2::delphes::TrackSmearer smearer;
  smearer.useEfficiency(true);
  smearer.loadTable(11, "lutCovm.el.dat");
  smearer.loadTable(13, "lutCovm.mu.dat");
  smearer.loadTable(211, "lutCovm.pi.dat");
  smearer.loadTable(321, "lutCovm.ka.dat");
  smearer.loadTable(2212, "lutCovm.pr.dat");

  // logx binning
  const Int_t nbins = 80;
  double xmin = 1.e-2;
  double xmax = 1.e2;
  double logxmin = std::log10(xmin);
  double logxmax = std::log10(xmax);
  double binwidth = (logxmax - logxmin) / nbins;
  double xbins[nbins + 1];
  xbins[0] = xmin;
  for (Int_t i = 1; i <= nbins; ++i)
    xbins[i] = xmin + std::pow(10., logxmin + i * binwidth);
  
  // histograms
  auto hTime0 = new TH1F("hTime0", ";t_{0} (ns)", 1000, -1., 1.);
  auto hBetaP = new TH2F("hBetaP", ";#it{p} (GeV/#it{c});#beta", nbins, xbins, 1000, 0.1, 1.1);
  TH2 *hNsigmaPt[5], *hNsigmaPt_true[5][5];
  const char *pname[5] = {"el", "mu", "pi", "ka", "pr"};
  const char *plabel[5] = {"e", "#mu", "#pi", "K", "p"};
  for (int i = 0; i < 5; ++i) {
    hNsigmaPt[i] = new TH2F(Form("hNsigmaPt_%s", pname[i]), Form(";#it{p_{T}} (GeV/#it{c});n#sigma_{%s}", plabel[i]), nbins, xbins, 200, -10., 10.);
    for (int j = 0; j < 5; ++j) {
      hNsigmaPt_true[i][j] = new TH2F(Form("hNsigmaPt_%s_true_%s", pname[i], pname[j]), Form(";#it{p_{T}} (GeV/#it{c});n#sigma_{%s}", plabel[i]), nbins, xbins, 200, -10., 10.);
    }
  }

  std::map<int, int> pidmap = { {11, 0}, {13, 1}, {211, 2}, {321, 3}, {2212, 4} };

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
    
      auto pdg = std::abs(track->PID);
      auto ipdg = pidmap[pdg];

      // fill beta-p
      auto p = track->P;
      auto pt = track->PT;
      auto beta = toflayer.getBeta(*track);
      hBetaP->Fill(p, beta);
      
      // fill nsigma
      std::array<float, 5> deltat, nsigma;
      toflayer.makePID(*track, deltat, nsigma);
      for (int i = 0; i < 5; ++i) {
	hNsigmaPt[i]->Fill(pt, nsigma[i]);
	hNsigmaPt_true[i][ipdg]->Fill(pt, nsigma[i]);
      }
      
    }
  }

  auto fout = TFile::Open(outputFile, "RECREATE");
  hTime0->Write();
  hBetaP->Write();
  for (int i = 0; i < 5; ++i) {
    hNsigmaPt[i]->Write();
    for (int j = 0; j < 5; ++j) {
      hNsigmaPt_true[i][j]->Write();
    }
  }
  fout->Close();

}
