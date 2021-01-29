R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double Bz = 0.2; // [T]
double rich_radius = 100.; // [cm]
double rich_length = 200.; // [cm]

void
rich(const char *inputFile = "delphes.root",
     const char *outputFile = "rich.root")
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

  // RICH detector
  o2::delphes::RICHdetector richdetector;
  richdetector.setup(rich_radius, rich_length);
  richdetector.setIndex(1.03);
  richdetector.setRadiatorLength(2.);
  richdetector.setEfficiency(0.4);
  richdetector.setSigma(7.e-3);
  
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
  
  TH1F *h = new TH1F("h","hist with log x axis",nbins,xbins);

  // histograms
  auto hAngleP = new TH2F("hAngleP", ";#it{p} (GeV/#it{c});#theta (rad)", nbins, xbins, 250, 0., 0.25);
  TH2 *hAngleP_true[5];
  TH1 *hGenP[5], *hGenPt[5];
  TH1 *hAccP[5], *hAccPt[5];
  TH1 *hRecP[5], *hRecPt[5];
  TH2 *hNsigmaP[5], *hNsigmaP_true[5][5];
  TH2 *hNsigmaPt[5], *hNsigmaPt_true[5][5];
  const char *pname[5] = {"el", "mu", "pi", "ka", "pr"};
  const char *plabel[5] = {"e", "#mu", "#pi", "K", "p"};
  std::map<int, int> pidmap = { {11, 0}, {13, 1}, {211, 2}, {321, 3}, {2212, 4} };
  std::map<int, double> pidmass = { {0, 0.00051099891}, {1, 0.10565800}, {2, 0.13957000}, {3, 0.49367700}, {4, 0.93827200} };
  for (int i = 0; i < 5; ++i) {
    hGenP[i] = new TH1F(Form("hGenP_%s", pname[i]), ";#it{p} (GeV/#it{c})", nbins, xbins);
    hGenPt[i] = new TH1F(Form("hGenPt_%s", pname[i]), ";#it{p} (GeV/#it{c})", nbins, xbins);
    hAccP[i] = new TH1F(Form("hAccP_%s", pname[i]), ";#it{p} (GeV/#it{c})", nbins, xbins);
    hAccPt[i] = new TH1F(Form("hAccPt_%s", pname[i]), ";#it{p} (GeV/#it{c})", nbins, xbins);
    hRecP[i] = new TH1F(Form("hRecP_%s", pname[i]), ";#it{p} (GeV/#it{c})", nbins, xbins);
    hRecPt[i] = new TH1F(Form("hRecPt_%s", pname[i]), ";#it{p} (GeV/#it{c})", nbins, xbins);
    hAngleP_true[i] = new TH2F(Form("hAngleP_true_%s", pname[i]), ";#it{p} (GeV/#it{c});#theta (rad)", nbins, xbins, 250, 0., 0.25);
    hNsigmaP[i] = new TH2F(Form("hNsigmaP_%s", pname[i]), Form(";#it{p} (GeV/#it{c});n#sigma_{%s}", plabel[i]), nbins, xbins, 200, -10., 10.);
    hNsigmaPt[i] = new TH2F(Form("hNsigmaPt_%s", pname[i]), Form(";#it{p_{T}} (GeV/#it{c});n#sigma_{%s}", plabel[i]), nbins, xbins, 200, -10., 10.);
    for (int j = 0; j < 5; ++j) {
      hNsigmaP_true[i][j] = new TH2F(Form("hNsigmaP_%s_true_%s", pname[i], pname[j]), Form(";#it{p} (GeV/#it{c});n#sigma_{%s}", plabel[i]), nbins, xbins, 200, -10., 10.);
      hNsigmaPt_true[i][j] = new TH2F(Form("hNsigmaPt_%s_true_%s", pname[i], pname[j]), Form(";#it{p_{T}} (GeV/#it{c});n#sigma_{%s}", plabel[i]), nbins, xbins, 200, -10., 10.);
    }
  }
  
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    
    // loop over tracks and smear
    std::vector<Track *> tof_tracks;
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();

      auto pdg = std::abs(track->PID);
      auto ipdg = pidmap[pdg];
      auto true_m = pidmass[ipdg];
      auto true_p = track->P;
      auto true_pt = track->PT;

      if (std::fabs(track->Eta) > 0.5) continue;

      hGenP[ipdg]->Fill(true_p);
      hGenPt[ipdg]->Fill(true_pt);
      
      // smear track
      if (!smearer.smearTrack(*track)) continue;
      auto p = track->P;
      auto pt = track->PT;

      // select primaries based on 3 sigma DCA cuts
      if (fabs(track->D0 / track->ErrorD0) > 3.) continue;
      if (fabs(track->DZ / track->ErrorDZ) > 3.) continue;

      // check if has RICH
      if (!richdetector.hasRICH(*track)) continue;

      // fill beta-p
      auto measurement = richdetector.getMeasuredAngle(*track);
      auto angle = measurement.first;
      auto anglee = measurement.second;
      if (anglee == 0.) continue;

      hRecP[ipdg]->Fill(true_p);
      hRecPt[ipdg]->Fill(true_pt);
      
      hAngleP->Fill(p, angle);
      hAngleP_true[ipdg]->Fill(p, angle);

      // make pid
      std::array<float, 5> deltaangle, nsigma;
      richdetector.makePID(*track, deltaangle, nsigma);
      for (int i = 0; i < 5; ++i) {
	hNsigmaP[i]->Fill(p, nsigma[i]);
	hNsigmaPt[i]->Fill(pt, nsigma[i]);
	hNsigmaP_true[i][ipdg]->Fill(p, nsigma[i]);	
	hNsigmaPt_true[i][ipdg]->Fill(pt, nsigma[i]);	
      }
      
    }
  }

  auto fout = TFile::Open(outputFile, "RECREATE");
  hAngleP->Write();
  for (int i = 0; i < 5; ++i) {
    hGenP[i]->Write();
    hGenPt[i]->Write();
    hAccP[i]->Write();
    hAccPt[i]->Write();
    hRecP[i]->Write();
    hRecPt[i]->Write();
    hAngleP_true[i]->Write();
    hNsigmaP[i]->Write();
    hNsigmaPt[i]->Write();
    for (int j = 0; j < 5; ++j) {      
      hNsigmaP_true[i][j]->Write();
      hNsigmaPt_true[i][j]->Write();
    }
  }
  fout->Close();

}
