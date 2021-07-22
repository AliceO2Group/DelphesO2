class SmearO2KineGenerator : public o2::eventgen::GeneratorFromO2Kine
{

public:

  SmearO2KineGenerator(const char *name) : GeneratorFromO2Kine(name) { };
  bool Init() override {
    auto retval = o2::eventgen::GeneratorFromO2Kine::Init();
    setContinueMode(true);
    return retval; };

  //  bool importParticles() override {
  
protected:

  
};


void
smear_o2_kine(const char *o2kinefilename)
{

  o2::delphes::TrackSmearer smearer;
  smearer.loadTable(11,   "lutCovm.el.dat");
  smearer.loadTable(13,   "lutCovm.mu.dat");
  smearer.loadTable(211,  "lutCovm.pi.dat");
  smearer.loadTable(321,  "lutCovm.ka.dat");
  smearer.loadTable(2212, "lutCovm.pr.dat");
  
  auto gen = new SmearO2KineGenerator(o2kinefilename);
  gen->Init();

  // loop over events
  while (gen->importParticles()) {
    auto particles = gen->getParticles();

    // loop over particles
    for (auto & particle : particles) {

      // we did not transport them before, we do not smear them either
      if (std::fabs(particle.Eta()) > 1.0) continue;
      
      // only particles to be transported, which are flagged as status code = 1
      // the particles that have been transported already have status code = 0
      if (particle.GetStatusCode() != 1) continue;

      // only particles that we know how to smear
      // namely el, mu, pi, ka, pr
      auto pdg = std::abs(particle.GetPdgCode());
      if (pdg != 11 && pdg != 13 && pdg != 211 && pdg != 321 && pdg != 2212) continue;

      // convert particle to o2 track and smear it
      O2Track o2track;
      o2::delphes::TrackUtils::convertTParticleToO2Track(particle, o2track);
      float nch = 1600.;
      o2track.print();
      if (!smearer.smearTrack(o2track, pdg, nch)) continue;
      o2track.print();

    }
  }
  
}
