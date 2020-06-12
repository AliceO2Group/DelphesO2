R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double Bz = 0.2;

double cosPA_cut = 0.99;
double dca_cut = 5.;

void
K0s(const char *inputFile = "delphes.root",
    const char *outputFile = "K0s.root")
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
  
  // vertexer
  o2::delphes::Vertex vertex;
  o2::delphes::VertexFitter vertexer;
  vertexer.setup(Bz, true, true);

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
  auto hMassPt_gen = new TH2F("hMassPt_gen", ";log_{10}(#it{p}_{T} / GeV);m_{#pi#pi} (GeV)", 40, -2., 2., 1000, 0., 2.);
  auto hMassPt_rec = new TH2F("hMassPt_rec", ";log_{10}(#it{p}_{T} / GeV);m_{#pi#pi} (GeV)", 40, -2., 2., 1000, 0., 2.);
  auto hMassPt_true = new TH2F("hMassPt_true", ";log_{10}(#it{p}_{T} / GeV);m_{#pi#pi} (GeV)", 40, -2., 2., 1000, 0., 2.);
  auto hCosPAPt_rec = new TH2F("hCosPAPt_rec", ";log_{10}(#it{p}_{T} / GeV);1 - cos #vartheta_{pointing}", 40, -2., 2., 1000, 0., 1.);
  auto hCosPAPt_true = new TH2F("hCosPAPt_true", ";log_{10}(#it{p}_{T} / GeV);1 - cos #vartheta_{pointing}", 40, -2., 2., 1000, 0., 1.);
  auto hDCAPt_rec = new TH2F("hDCAPt_rec", ";log_{10}(#it{p}_{T} / GeV);d_{tracks} (mm)", 40, -2., 2., 1000, 0., 100.);
  auto hDCAPt_true = new TH2F("hDCAPt_true", ";log_{10}(#it{p}_{T} / GeV);d_{tracks} (mm)", 40, -2., 2., 1000, 0., 100.);
  
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);

    // loop over particles
    for (Int_t iparticle = 0; iparticle < particles->GetEntries(); ++iparticle) {
      auto particle = (GenParticle *)particles->At(iparticle);
      if (particle->PID != 310) continue;
      hMassPt_gen->Fill(log10(particle->PT), particle->Mass);
    }

    std::vector<Track *> positiveTracks, negativeTracks;
    
    // loop over tracks
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();

      // smear track
      if (!smearer.smearTrack(*track)) continue;

      // pretend we have perfect PID
      if (abs(particle->PID) != 211) continue;
      
      // reject primaries based on 3 sigma DCA cuts
      if (fabs(track->D0 / track->ErrorD0) < 3.) continue;
      if (fabs(track->DZ / track->ErrorDZ) < 3.) continue;
      
      // fill positive/negative track vectors
      if (track->Charge > 0.)
	positiveTracks.push_back(track);
      else if (track->Charge < 0.)
	negativeTracks.push_back(track);
      else continue;
      
    }

    // reconstruct decays
    TLorentzVector LV1, LV2, LV;
    for (auto positiveTrack : positiveTracks) {
      for (auto negativeTrack : negativeTracks) {
	
	// make a copy of the tracks
	auto t1 = *positiveTrack;
	auto t2 = *negativeTrack;

	// fit vertex
	if (!vertexer.fitVertex(t1, t2, vertex)) continue;

	auto mass1 = 0.13957000;
	auto mass2 = 0.13957000;
	LV1.SetPtEtaPhiM(t1.PT, t1.Eta, t1.Phi, mass1);
	LV2.SetPtEtaPhiM(t2.PT, t2.Eta, t2.Phi, mass2);
	LV = LV1 + LV2;

	auto mass = LV.Mag();
	auto pt = LV.Pt();
	
	// compute cosine of pointing angle
	auto vtxV = TVector3(vertex.x, vertex.y, vertex.z).Unit();
	auto dirV = LV.Vect().Unit();
	auto cosPA = std::cos(vtxV.Angle(dirV));

	// compute distance between tracks at secondary vertex
	auto dca = sqrt( (t1.X - t2.X) * (t1.X - t2.X) + (t1.Y - t2.Y) * (t1.Y - t2.Y) + (t1.Z - t2.Z) * (t1.Z - t2.Z) );

	hCosPAPt_rec->Fill(log10(pt), 1. - cosPA);
	hDCAPt_rec->Fill(log10(pt), dca);
	  
	if (cosPA > cosPA_cut && dca < dca_cut) {
	  hMassPt_rec->Fill(log10(pt), mass);
	}

	auto p1 = (GenParticle *)t1.Particle.GetObject();
	auto p2 = (GenParticle *)t2.Particle.GetObject();

	if (p1->M1 != p2->M1) continue;
	auto mother = (GenParticle *)particles->At(p1->M1);
	if (mother->PID != 310) continue;

	hMassPt_true->Fill(log10(pt), mass);
	hCosPAPt_true->Fill(log10(pt), 1. - cosPA);
	hDCAPt_true->Fill(log10(pt), dca);


      }
    }
  }

  auto fout = TFile::Open(outputFile, "RECREATE");
  hMassPt_gen->Write();
  hMassPt_rec->Write();
  hMassPt_true->Write();
  hCosPAPt_rec->Write();
  hCosPAPt_true->Write();
  hDCAPt_rec->Write();
  hDCAPt_true->Write();
  fout->Close();
  
}
