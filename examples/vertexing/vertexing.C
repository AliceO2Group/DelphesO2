R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double Bfield = 0.5;

int eventOfInterest = -1;

void
vertexing(const char *inputFile = "delphes.root",
	  const char *outputfile = "vertexing.root")
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

  // vertex finder
  o2::delphes::Vertex vertex;
  o2::delphes::VertexFitter vertexer;
  vertexer.setup(Bfield, true, true);

  // smearer
  o2::delphes::TrackSmearer smearer;
  if (Bfield == 0.2) {
    smearer.loadTable(11, "lutCovm.el.2kG.dat");
    smearer.loadTable(13, "lutCovm.mu.2kG.dat");
    smearer.loadTable(211, "lutCovm.pi.2kG.dat");
    smearer.loadTable(321, "lutCovm.ka.2kG.dat");
    smearer.loadTable(2212, "lutCovm.pr.2kG.dat");
  } else if (Bfield == 0.5) {
    smearer.loadTable(11, "lutCovm.el.5kG.dat");
    smearer.loadTable(13, "lutCovm.mu.5kG.dat");
    smearer.loadTable(211, "lutCovm.pi.5kG.dat");
    smearer.loadTable(321, "lutCovm.ka.5kG.dat");
    smearer.loadTable(2212, "lutCovm.pr.5kG.dat");
  } else {
    std::cout << " --- invalid B field: " << Bfield << std::endl;
    return;
  }
    
  // histograms
  auto hR = new TH2F("hR", ";R_{gen} (mm);R_{rec} (mm)", 1000, 0., 1000., 1000, 0., 1000.);
  auto hZ = new TH2F("hZ", ";z_{gen} (mm);z_{rec} (mm)", 1000, -1000., 1000., 1000, -1000., 1000.);
  auto hPhi = new TH2F("hPhi", ";#varphi_{gen} (rad);#varphi_{rec} (rad)", 1000, -M_PI, M_PI, 1000, -M_PI, M_PI);

  auto hMass = new TH1F("hMass", ";m_{rec} (GeV)", 3000, 0., 3.);

  auto hR_gen = new TH1F("hR_gen", ";R_{gen} (mm)", 2000, 0., 1000.);
  hR_gen->SetFillColor(kRed+1);
  hR_gen->SetFillStyle(3003);
  auto hZ_gen = new TH1F("hZ_gen", ";z_{gen} (mm)", 400, -2000., 2000.);
  hZ_gen->SetFillColor(kRed+1);
  hZ_gen->SetFillStyle(3003);
  auto hPhi_gen = new TH1F("hPhi_gen", ";#varphi_{gen} (rad)", 20, -M_PI, M_PI);
  hPhi_gen->SetFillColor(kRed+1);
  hPhi_gen->SetFillStyle(3003);
  
  auto hR_rec = new TH1F("hR_rec", ";R_{rec} (mm)", 2000, 0., 1000.);
  hR_rec->Sumw2();
  auto hZ_rec = new TH1F("hZ_rec", ";z_{rec} (mm)", 400, -2000., 2000.);
  hZ_rec->Sumw2();
  auto hPhi_rec = new TH1F("hPhi_rec", ";#varphi_{rec} (rad)", 20, -M_PI, M_PI);
  hPhi_rec->Sumw2();
  
  auto hdR = new TH1F("hdR", ";#DeltaR (mm)", 500, -2.5, 2.5);
  auto hdX = new TH1F("hdX", ";#Deltax (mm)", 500, -2.5, 2.5);
  auto hdXX = new TH2F("hdXX", ";log_{10}(x_{gen}/mm);#Deltax (mm)", 400, -2., 2., 500, -2.5, 2.5);
  auto hdY = new TH1F("hdY", ";#Deltay (mm)", 500, -2.5, 2.5);
  auto hdZ = new TH1F("hdZ", ";#Deltaz (mm)", 500, -2.5, 2.5);
  auto hdPhi = new TH1F("hdPhi", ";#Delta#varphi (mrad)", 500, -250., 250.);
  auto hdMass = new TH1F("hdMass", ";#Deltam (MeV)", 200, -1000., 1000.);

  auto hdXY = new TH2D("hdXY", ";#Deltax (mm);#Deltay (mm)", 500, -2.5, 2.5, 500, -2.5, 2.5);
  
  auto hdRR = new TH2F("hdRR", ";log_{10}(R_{gen}/mm);#DeltaR (mm)", 400, -2., 2., 500, -2.5, 2.5);
  auto hdRR_p = new TProfile("hdRR_p", ";log_{10}(R_{gen}/mm);#DeltaR (mm)", 400, -2., 2., "S");
  auto hdPhiR = new TH2F("hdPhiR", ";log_{10}(R_{gen}/mm);#Delta#varphi (mrad)", 400, -2., 2., 500, -250., 250.);
  auto hdPhiR_p = new TProfile("hdPhiR_p", ";log_{10}(R_{gen}/mm);#Delta#varphi (mrad)", 400, -2., 2., "S");
  auto hdZR = new TH2F("hdZR", ";log_{10}(R_{gen}/mm);#Deltaz (mm)", 400, -2., 2., 500, -2.5, 2.5);
  auto hdZR_p = new TProfile("hdZR_p", ";log_{10}(R_{gen}/mm);#Deltaz (mm)", 400, -2., 2., "S");

  // output tree
  int n;
  int ch[024];
  float pt[1024], eta[1024], phi[1024];
  float x[1024], y[1024], z[1024];
  //
  float vpt[1024], veta[1024], vphi[1024];
  float vx[1024], vy[1024], vz[1024];
  TFile *fout = TFile::Open("secondary.root", "RECREATE");
  TTree *tout = new TTree("secondary", "Secondary tree");
  tout->Branch("n", &n, "n/I");
  tout->Branch("ch", &ch, "ch[n]/I");
  tout->Branch("pt", &pt, "pt[n]/F");
  tout->Branch("eta", &eta, "eta[n]/F");
  tout->Branch("phi", &phi, "phi[n]/F");
  tout->Branch("x", &x, "x[n]/F");
  tout->Branch("y", &y, "y[n]/F");
  tout->Branch("z", &z, "z[n]/F");
  //
  tout->Branch("vpt", &vpt, "vpt[n]/F");
  tout->Branch("veta", &veta, "veta[n]/F");
  tout->Branch("vphi", &vphi, "vphi[n]/F");
  tout->Branch("vx", &vx, "vx[n]/F");
  tout->Branch("vy", &vy, "vy[n]/F");
  tout->Branch("vz", &vz, "vz[n]/F");

  int nattempts = 0;
  int nfails = 0;
  
  TLorentzVector LV1, LV2, LV;  
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    //    treeReader->ReadEntry(100);
    std::map<GenParticle *, std::vector<Track *>> decays;

    // loop over tracks
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {
      auto track = (Track *)tracks->At(itrack);

      // skip primary tracks
      //      if (track->X == 0. && track->Y == 0. && track->Z == 0.) continue;

      // get corresponding particle
      auto particle = (GenParticle *)track->Particle.GetObject();

      // skip if no mother information
      if (particle->M1 < 0) continue;
      auto mother = (GenParticle *)particles->At(particle->M1);
      if (!mother) {
	std::cout << "why is that? PDG=" << particle->PID << "(" << itrack << "), mother=" << particle->M1 << std::endl;
	continue;
      }

      if (false && abs(mother->PID) == 3122) decays[mother].push_back(track); // Lambda0
      if (false && abs(mother->PID) ==  310) decays[mother].push_back(track); // K0s
      if (true  && abs(mother->PID) ==  421) decays[mother].push_back(track); // D0

    }

    // loop over all decays
    for (auto &decay : decays) {
      auto mother = decay.first;
      auto tracks = decay.second;

      if (tracks.size() != 2) continue;

      // smear tracks
      auto smearedTracks = tracks;      
      for (auto &smearedTrack : smearedTracks)
       	smearer.smearTrack(*smearedTrack);
      
      auto t1 = smearedTracks[0];
      auto t2 = smearedTracks[1];
      auto p1 = (GenParticle *)t1->Particle.GetObject();
      auto p2 = (GenParticle *)t2->Particle.GetObject();

      // generated values
      auto R_gen = hypot(p1->X, p1->Y);
      auto X_gen = p1->X;
      auto Y_gen = p1->Y;
      auto Z_gen = p1->Z;
      auto Phi_gen = atan2(p1->Y, p1->X);
      hR_gen->Fill(R_gen);
      hZ_gen->Fill(Z_gen);
      hPhi_gen->Fill(Phi_gen);

      // fit vertex
      nattempts++;
      if (!vertexer.fitVertex(*t1, *t2, vertex)) {
	nfails++;
	//	continue;
	
	std::cout << " --- fit vertex failed " << std::endl << std::endl;
	
	// print out for chSVFit.C macro
	std::cout << std::scientific << std::setprecision(8);
	std::cout << std::endl;
	std::cout << "// true decay vtx \n";
	std::cout << "float vx = " << p1->X * 0.1 << ", vy = " << p1->Y * 0.1 << ", vz = " << p1->Z * 0.1 << "; \n\n";
	std::cout << "// Delphes tracks at decay vtx \n";
	std::cout << "Track t0 {" << p1->PT << ", " << p1->Eta << ", " << p1->Phi
		  << ", " << p1->X * 0.1 << ", " << p1->Y * 0.1 << ", " << p1->Z * 0.1 << ", " << p1->Charge << "}; \n";
	std::cout << "Track t1 {" << p2->PT << ", " << p2->Eta << ", " << p2->Phi
		  << ", " << p2->X * 0.1 << ", " << p2->Y * 0.1 << ", " << p2->Z * 0.1 << ", " << p2->Charge << "}; \n\n";
	std::cout << "// Delphes tracks at primary vtx \n";
	std::cout << "Track t0v {" << t1->PT << ", " << t1->Eta << ", " << t1->Phi
		  << ", " << t1->Xd * 0.1 << ", " << t1->Yd * 0.1 << ", " << t1->Zd * 0.1 << ", " << t1->Charge << "}; \n";
	std::cout << "Track t1v {" << t2->PT << ", " << t2->Eta << ", " << t2->Phi
		  << ", " << t2->Xd * 0.1 << ", " << t2->Yd * 0.1 << ", " << t2->Zd * 0.1 << ", " << t2->Charge << "}; \n\n";
	//	  std::cout << std::setprecision(10);

	// save in file
	n = 0;
	for (auto &track : tracks) {
	  GenParticle *particle = (GenParticle *)track->Particle.GetObject();
	  ch[n] = track->Charge;
	  pt[n] = track->PT;
	  eta[n] = track->Eta;
	  phi[n] = track->Phi;
	  x[n] = track->Xd;
	  y[n] = track->Yd;
	  z[n] = track->Zd;
	  //
	  vpt[n] = particle->PT;
	  veta[n] = particle->Eta;
	  vphi[n] = particle->Phi;
	  vx[n] = particle->X;
	  vy[n] = particle->Y;
	  vz[n] = particle->Z;
	  n++;
	  
	}
	tout->Fill();

	continue;
      }

      auto mass1 = p1->Mass; 
      auto mass2 = p2->Mass; 
      LV1.SetPtEtaPhiM(t1->PT, t1->Eta, t1->Phi, mass1);
      LV2.SetPtEtaPhiM(t2->PT, t2->Eta, t2->Phi, mass2);
      LV = LV1 + LV2;

      // fill histograms
      auto R_rec = hypot(vertex.x, vertex.y);
      auto X_rec = vertex.x;
      auto Y_rec = vertex.y;
      auto Z_rec = vertex.z;
      auto Phi_rec = atan2(vertex.y, vertex.x);
      hR->Fill(R_gen, R_rec);
      hZ->Fill(Z_gen, Z_rec);
      hPhi->Fill(Phi_gen, Phi_rec);
      hMass->Fill(LV.Mag());

      hR_rec->Fill(R_gen);
      hZ_rec->Fill(Z_gen);
      hPhi_rec->Fill(Phi_gen);

      hdR->Fill(R_rec - R_gen);
      hdRR->Fill(log10(R_gen), R_rec - R_gen);
      hdRR_p->Fill(log10(R_gen), R_rec - R_gen);
      hdX->Fill(X_rec - X_gen);
      hdXX->Fill(log10(X_gen), X_rec - X_gen);
      hdY->Fill(Y_rec - Y_gen);
      hdZ->Fill(Z_rec - Z_gen);
      hdXY->Fill(X_rec - X_gen, Y_rec - Y_gen);
      hdZR->Fill(log10(R_gen), Z_rec - Z_gen);
      hdZR_p->Fill(log10(R_gen), Z_rec - Z_gen);
      hdPhi->Fill(1.e3 * atan2(sin(Phi_rec - Phi_gen), cos(Phi_rec - Phi_gen)));
      hdPhiR->Fill(log10(R_gen), 1.e3 * atan2(sin(Phi_rec - Phi_gen), cos(Phi_rec - Phi_gen)));
      hdPhiR_p->Fill(log10(R_gen), 1.e3 * atan2(sin(Phi_rec - Phi_gen), cos(Phi_rec - Phi_gen)));
      hdMass->Fill(1.e3 * (LV.Mag() - mother->Mass));

      
    }

  }

  // divide
  hR_rec->Divide(hR_rec, hR_gen, 1., 1., "B");
  hZ_rec->Divide(hZ_rec, hZ_gen, 1., 1., "B");
  hPhi_rec->Divide(hPhi_rec, hPhi_gen, 1., 1., "B");
  
  // draw
  auto c = new TCanvas("c", "c", 1500, 1000);
  c->Divide(3, 2);
  c->cd(1);
  hR_rec->Draw();
  c->cd(2);
  hZ_rec->Draw();
  c->cd(3);
  hPhi_rec->Draw();
  c->cd(4)->SetLogy();
  hdR->Draw("box");
  c->cd(5)->SetLogy();
  hdZ->Draw("box");
  c->cd(6)->SetLogy();
  hdPhi->Draw("box");

  auto cMass = new TCanvas("cMass", "cMass", 800, 800);
  hMass->Draw();

  auto cPerformance = new TCanvas("cPerformance", "cPerformance", 1000, 1000);
  cPerformance->Divide(2, 2);
  cPerformance->cd(1)->SetLogy();
  hdR->Draw();
  cPerformance->cd(2)->SetLogy();
  hdZ->Draw();
  cPerformance->cd(3)->SetLogy();
  hdPhi->Draw();
  cPerformance->cd(4)->SetLogy();
  hdMass->Draw();
  cPerformance->SaveAs("vertexing.performance.png");
  
  // write
  fout->cd();
  hR->Write();
  hdR->Write();
  hZ->Write();
  hdZ->Write();
  hPhi->Write();
  hdPhi->Write();
  tout->Write();
  fout->Close();

  auto ffout = TFile::Open(outputfile, "RECREATE");
  hdR->Write();
  hdRR->Write();
  hdRR_p->Write();
  hdX->Write();
  hdXX->Write();
  hdY->Write();
  hdZ->Write();
  hdXY->Write();
  hdZR->Write();
  hdZR_p->Write();
  hdPhi->Write();
  hdPhiR->Write();
  hdPhiR_p->Write();
  hMass->Write();
  ffout->Close();
  
  std::cout << "attempted " << nattempts << " fits: " << nfails << " failed " << std::endl;
}
