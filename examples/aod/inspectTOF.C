R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

#include "createO2tables.h"

using O2Track = o2::track::TrackParCov;

void inspectTOF(const char* filename)
{
  auto fin = TFile::Open(filename);

  auto tcollision = (TTree*)fin->Get("O2collision");
  auto ncollision = tcollision->GetEntries();
  ConnectTreeO2collision(tcollision);

  auto ttracks = (TTree*)fin->Get("O2track");
  auto ntracks = ttracks->GetEntries();
  ConnectTreeO2track(ttracks);

  /** histograms **/
  auto hTZero = new TH1F("hTZero", ";t_{0} (ns)", 1000, -1., 1.);
  auto hBetaP = new TH2F("hBetaP", ";#it{p} (GeV/#it{c});#it{v}/#it{c}", 500., 0., 5., 500, 0.1, 1.1);
  auto hNsigmaP = new TH2F("hNsigmaP", ";#it{p} (GeV/#it{c});n#sigma_{K}", 500., 0., 5., 500, -100., 100.);

  /** loop over collisions **/
  for (int icollision = 0; icollision < ncollision; ++icollision) {
    tcollision->GetEntry(icollision);
    auto t0 = collision.fCollisionTime;
    auto t0e = collision.fCollisionTimeRes;
    hTZero->Fill(t0 * 0.001);
  }

  /** loop over tracks **/
  for (int itrack = 0; itrack < ntracks; ++itrack) {

    /** get track **/
    ttracks->GetEntry(itrack);

    /** get collision and start time **/
    auto icollision = mytracks.fCollisionsID;
    tcollision->GetEntry(icollision);
    auto t0 = collision.fCollisionTime;
    auto t0e = collision.fCollisionTimeRes;

    /** create o2 track **/
    O2Track o2track;
    o2track.setX(mytracks.fX);
    o2track.setAlpha(mytracks.fAlpha);
    o2track.setY(mytracks.fY);
    o2track.setZ(mytracks.fZ);
    o2track.setSnp(mytracks.fSnp);
    o2track.setTgl(mytracks.fTgl);
    o2track.setQ2Pt(mytracks.fSigned1Pt);

    /** get track information **/
    auto p = o2track.getP();
    auto p2 = p * p;
    auto pe = p * mytracks.fSigma1Pt; // this is wrong, needs to add pz contribution
    auto d0 = mytracks.fY;
    auto d0e = mytracks.fSigmaY;

    /** 3-sigma DCA cut on primaries **/
    if (fabs(d0 / d0e) > 1.)
      continue;

    /** get TOF information **/
    auto L = mytracks.fLength;
    auto t = mytracks.fTOFSignal;
    auto tof = t - t0;
    auto beta = L / tof / 0.029979246;
    auto mass = 0.49367700;
    auto mass2 = mass * mass;
    auto texp = L / p / 0.029979246 * hypot(mass, p);
    auto texpe = L / 0.029979246 / p / p * mass * mass * hypot(mass, p) * pe;
    auto sigma = hypot(hypot(20., t0e), texpe);
    auto nsigma = (tof - texp) / sigma;

    hBetaP->Fill(p, beta);
    hNsigmaP->Fill(p, nsigma);
  }

  /** write output **/
  auto fout = TFile::Open(std::string("inspectTOF." + std::string(filename)).c_str(), "RECREATE");
  hTZero->Write();
  hBetaP->Write();
  hNsigmaP->Write();
  fout->Close();

  /** close input **/
  fin->Close();
}
