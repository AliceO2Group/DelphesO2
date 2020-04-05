R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

void
vertexing(const char *inputFile = "delphes.root")
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
  vertexer.setup(0.5, true, true);
  
  // histograms
  auto hR = new TH2F("hR", "", 1000, 0., 1000., 1000, 0., 1000.);
  auto hZ = new TH2F("hZ", "", 1000, -1000., 1000., 1000, -1000., 1000.);
  auto hPhi = new TH2F("hPhi", "", 1000, -M_PI, M_PI, 1000, -M_PI, M_PI);
  
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    std::map<GenParticle *, std::vector<Track *>> decays;
    
    // loop over tracks
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {
      auto track = (Track *)tracks->At(itrack);

      // skip primary tracks
      if (track->X == 0. && track->Y == 0. && track->Z == 0.)
	continue;

      // get corresponding particle and mother
      auto particle = (GenParticle *)track->Particle.GetObject();
      auto mother = (GenParticle *)particles->At(particle->M1);

      if (true  && abs(mother->PID) == 3122) decays[mother].push_back(track); // Lambda0
      if (false && abs(mother->PID) ==  310) decays[mother].push_back(track); // K0s

    }

    // loop over all decays
    for (auto &decay : decays) {
      auto mother = decay.first;
      auto tracks = decay.second;

      auto t1 = tracks[0];
      auto t2 = tracks[1];
      auto p1 = (GenParticle *)t1->Particle.GetObject();
      auto p2 = (GenParticle *)t2->Particle.GetObject();

      // fit vertex
      if (!vertexer.fitVertex(*t1, *t2, vertex)) {
	std::cout << " --- fit vertex failed " << std::endl;

	std::cout << "particles: pt, eta, phi, x, y, z, charge, pdg" << std::endl; 
	std::cout << "       p1: "
		  << p1->PT << " " << p1->Eta << " " << p1->Phi << " "
		  << p1->X  << " " << p1->Y   << " " << p1->Z   << " "
		  << p1->Charge << " " << p1->PID << std::endl;
	std::cout << "       p2: "
		  << p2->PT << " " << p2->Eta << " " << p2->Phi << " "
		  << p2->X  << " " << p2->Y   << " " << p2->Z   << " "
		  << p2->Charge << " " << p2->PID << std::endl;
	
	std::cout << "   tracks: pt, eta, phi, x, y, z, charge, pdg" << std::endl; 
	std::cout << "       t1: "
		  << t1->PT << " " << t1->Eta << " " << t1->Phi << " "
		  << t1->Xd << " " << t1->Yd  << " " << t1->Zd  << " "
		  << t1->Charge << " " << t1->PID << std::endl;
	std::cout << "       t2: "
		  << t2->PT << " " << t2->Eta << " " << t2->Phi << " "
		  << t2->Xd << " " << t2->Yd  << " " << t2->Zd  << " "
		  << t2->Charge << " " << t2->PID << std::endl;
	
	continue;
      }

      // fill histograms
      hR->Fill(hypot(p1->X, p1->Y), hypot(vertex.x, vertex.y));
      hZ->Fill(p1->Z, vertex.z);
      hPhi->Fill(atan2(p1->Y, p1->X), atan2(vertex.y, vertex.x));
      
    }

  }

  // draw
  auto c = new TCanvas("c", "c", 1500, 500);
  c->Divide(3, 1);
  c->cd(1);
  hR->Draw("box");
  c->cd(2);
  hZ->Draw("box");
  c->cd(3);
  hPhi->Draw("box");
  
}
