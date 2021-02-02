R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

// ROOT includes
#include "TMath.h"
#include "TChain.h"
#include "TClonesArray.h"

// Delphes includes
#include "ExRootAnalysis/ExRootTreeReader.h"

// O2 includes
#include "DetectorsVertexing/PVertexer.h"
#include "DetectorsVertexing/PVertexerHelpers.h"
#include "CommonDataFormat/BunchFilling.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"

// DelphesO2 includes
#include "TrackSmearer.hh"
#include "TOFLayer.hh"
#include "RICHdetector.hh"
#include "TrackUtils.hh"

#include "createO2tables.h"

const double Bz = 0.2;          // [T]
const double tof_radius = 100.; // [cm]
const double tof_length = 200.; // [cm]
const double tof_sigmat = 0.02; // [ns]

void createO2tables(const char* inputFile = "delphes.root",
                    const char* outputFile = "AODRun5.root",
                    int eventOffset = 0,
                    const bool do_vertexing = true)
{
  if ((inputFile != NULL) && (inputFile[0] == '\0')) {
    Printf("input file is empty, returning");
    return;
  }

  if (do_vertexing) {
    o2::base::GeometryManager::loadGeometry();
    o2::base::Propagator::initFieldFromGRP("o2sim_grp.root");
  }

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

  // TOF layer
  o2::delphes::TOFLayer toflayer;
  toflayer.setup(tof_radius, tof_length, tof_sigmat);

  // create output
  auto fout = TFile::Open(outputFile, "RECREATE");
  TTree* tBC = MakeTreeO2bc();
  TTree* fTracks = MakeTreeO2track();
  TTree* tEvents = MakeTreeO2collision();
  TTree* tMCvtx = MakeTreeO2mccollision();
  TTree* tKinematics = MakeTreeO2mcparticle();
  TTree* tLabels = MakeTreeO2mctracklabel();
  TTree* tCollisionLabels = MakeTreeO2mccollisionlabel();

  const UInt_t mTrackX = 0xFFFFFFFF;
  const UInt_t mTrackAlpha = 0xFFFFFFFF;
  const UInt_t mtrackSnp = 0xFFFFFFFF;
  const UInt_t mTrackTgl = 0xFFFFFFFF;
  const UInt_t mTrack1Pt = 0xFFFFFFFF;     // Including the momentun at the inner wall of TPC
  const UInt_t mTrackCovDiag = 0xFFFFFFFF; // Including the chi2
  const UInt_t mTrackCovOffDiag = 0xFFFFFFFF;
  const UInt_t mTrackSignal = 0xFFFFFFFF; // PID signals and track length

  int fOffsetLabel = 0;
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {

    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);

    // loop over particles
    for (Int_t iparticle = 0; iparticle < particles->GetEntries(); ++iparticle) {
      auto particle = (GenParticle*)particles->At(iparticle);

      particle->SetUniqueID(iparticle + fOffsetLabel); // not sure this is needed, to be sure

      mcparticle.fMcCollisionsID = ientry + eventOffset;
      mcparticle.fPdgCode = particle->PID;
      mcparticle.fStatusCode = particle->Status;
      mcparticle.fFlags = 0;
      mcparticle.fMother0 = particle->M1;
      if (mcparticle.fMother0 > -1)
        mcparticle.fMother0 += fOffsetLabel;
      mcparticle.fMother1 = particle->M2;
      if (mcparticle.fMother1 > -1)
        mcparticle.fMother1 += fOffsetLabel;
      mcparticle.fDaughter0 = particle->D1;
      if (mcparticle.fDaughter0 > -1)
        mcparticle.fDaughter0 += fOffsetLabel;
      mcparticle.fDaughter1 = particle->D2;
      if (mcparticle.fDaughter1 > -1)
        mcparticle.fDaughter1 += fOffsetLabel;
      mcparticle.fWeight = 1.;

      mcparticle.fPx = particle->Px;
      mcparticle.fPy = particle->Py;
      mcparticle.fPz = particle->Pz;
      mcparticle.fE = particle->E;

      mcparticle.fVx = particle->X;
      mcparticle.fVy = particle->Y;
      mcparticle.fVz = particle->Z;
      mcparticle.fVt = particle->T;

      tKinematics->Fill();
    }
    fOffsetLabel += particles->GetEntries();

    // loop over tracks
    std::vector<O2Track> tracks_for_vertexing;
    std::vector<Track*> tof_tracks;
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track*)tracks->At(itrack);
      auto particle = (GenParticle*)track->Particle.GetObject();

      O2Track o2track; // tracks in internal O2 format
      o2::delphes::TrackUtils::convertTrackToO2Track(*track, o2track, true);
      if (!smearer.smearTrack(o2track, track->PID)) { // Skipping inefficient/not correctly smeared tracks
        continue;
      }
      o2::delphes::TrackUtils::convertO2TrackToTrack(o2track, *track, true);

      // fill the label tree
      Int_t alabel = particle->GetUniqueID();
      mctracklabel.fLabel = TMath::Abs(alabel);
      mctracklabel.fLabelMask = 0;
      tLabels->Fill();

      // set track information
      mytracks.fCollisionsID = ientry + eventOffset;
      mytracks.fX = o2track.getX();
      mytracks.fAlpha = o2track.getAlpha();
      mytracks.fY = o2track.getY();
      mytracks.fZ = o2track.getZ();
      mytracks.fSnp = o2track.getSnp();
      mytracks.fTgl = o2track.getTgl();
      mytracks.fSigned1Pt = o2track.getQ2Pt();

      // Modified covariance matrix
      // First sigmas on the diagonal
      mytracks.fSigmaY = TMath::Sqrt(o2track.getSigmaY2());
      mytracks.fSigmaZ = TMath::Sqrt(o2track.getSigmaZ2());
      mytracks.fSigmaSnp = TMath::Sqrt(o2track.getSigmaSnp2());
      mytracks.fSigmaTgl = TMath::Sqrt(o2track.getSigmaTgl2());
      mytracks.fSigma1Pt = TMath::Sqrt(o2track.getSigma1Pt2());

      mytracks.fRhoZY = (Char_t)(128. * o2track.getSigmaZY() / mytracks.fSigmaZ / mytracks.fSigmaY);
      mytracks.fRhoSnpY = (Char_t)(128. * o2track.getSigmaSnpY() / mytracks.fSigmaSnp / mytracks.fSigmaY);
      mytracks.fRhoSnpZ = (Char_t)(128. * o2track.getSigmaSnpZ() / mytracks.fSigmaSnp / mytracks.fSigmaZ);
      mytracks.fRhoTglY = (Char_t)(128. * o2track.getSigmaTglY() / mytracks.fSigmaTgl / mytracks.fSigmaY);
      mytracks.fRhoTglZ = (Char_t)(128. * o2track.getSigmaTglZ() / mytracks.fSigmaTgl / mytracks.fSigmaZ);
      mytracks.fRhoTglSnp = (Char_t)(128. * o2track.getSigmaTglSnp() / mytracks.fSigmaTgl / mytracks.fSigmaSnp);
      mytracks.fRho1PtY = (Char_t)(128. * o2track.getSigma1PtY() / mytracks.fSigma1Pt / mytracks.fSigmaY);
      mytracks.fRho1PtZ = (Char_t)(128. * o2track.getSigma1PtZ() / mytracks.fSigma1Pt / mytracks.fSigmaZ);
      mytracks.fRho1PtSnp = (Char_t)(128. * o2track.getSigma1PtSnp() / mytracks.fSigma1Pt / mytracks.fSigmaSnp);
      mytracks.fRho1PtTgl = (Char_t)(128. * o2track.getSigma1PtTgl() / mytracks.fSigma1Pt / mytracks.fSigmaTgl);

      //FIXME this needs to be fixed
      mytracks.fITSClusterMap = 3;
      mytracks.fFlags = 4;

      //FIXME this also needs to be fixed
      mytracks.fTrackEtaEMCAL = 0; //track->GetTrackEtaOnEMCal();
      mytracks.fTrackPhiEMCAL = 0; //track->GetTrackPhiOnEMCal();

      // check if has hit the TOF
      if (toflayer.hasTOF(*track)) {
        mytracks.fLength = track->L * 0.1;           // [cm]
        mytracks.fTOFSignal = track->TOuter * 1.e12; // [ps]
        mytracks.fTOFExpMom = track->P * 0.029979246;
        // if primary push to TOF tracks
        if (fabs(mytracks.fY) < 3. * mytracks.fSigmaY && fabs(mytracks.fZ) < 3. * mytracks.fSigmaZ)
          tof_tracks.push_back(track);
      } else {
        mytracks.fLength = -999.f;
        mytracks.fTOFSignal = -999.f;
        mytracks.fTOFExpMom = -999.f;
      }
      if (do_vertexing) {
        tracks_for_vertexing.push_back(o2track);
      }
      fTracks->Fill();
      // fill histograms
    }

    // compute the event time
    std::array<float, 2> tzero;
    toflayer.eventTime(tof_tracks, tzero);

    // fill collision information
    collision.fBCsID = ientry + eventOffset;
    bc.fGlobalBC = ientry + eventOffset;
    if (do_vertexing) { // Performing vertexing
      o2::BunchFilling bcfill;
      bcfill.setDefault();
      o2::vertexing::PVertexer vertexer;
      vertexer.setValidateWithFT0(kFALSE);
      vertexer.setBunchFilling(bcfill);
      vertexer.init();

      gsl::span<const o2::MCCompLabel> lblITS;
      gsl::span<const O2Track> span_tracks_for_vertexing(tracks_for_vertexing);
      std::vector<o2::vertexing::PVertex> vertices;
      std::vector<o2::vertexing::GIndex> vertexTrackIDs;
      std::vector<o2::vertexing::V2TRef> v2tRefs;
      std::vector<o2::MCEventLabel> lblVtx;
      vertexer.setStartIR({0, 0});
      const int n_vertices = vertexer.process(span_tracks_for_vertexing, vertices, vertexTrackIDs, v2tRefs);
      Printf("Found %i vertices with %zu tracks", n_vertices, tracks_for_vertexing.size());
      if (n_vertices == 0) {
        collision.fPosX = 0.f;
        collision.fPosY = 0.f;
        collision.fPosZ = 0.f;
        collision.fCovXX = 0.f;
        collision.fCovXY = 0.f;
        collision.fCovXZ = 0.f;
        collision.fCovYY = 0.f;
        collision.fCovYZ = 0.f;
        collision.fCovZZ = 0.f;
        collision.fChi2 = 0.01f;
        collision.fN = 0;
      } else {
        collision.fPosX = vertices[0].getX();
        collision.fPosY = vertices[0].getY();
        collision.fPosZ = vertices[0].getZ();
        collision.fCovXX = vertices[0].getSigmaX2();
        collision.fCovXY = vertices[0].getSigmaXY();
        collision.fCovXZ = vertices[0].getSigmaXZ();
        collision.fCovYY = vertices[0].getSigmaY2();
        collision.fCovYZ = vertices[0].getSigmaYZ();
        collision.fCovZZ = vertices[0].getSigmaZ2();
        collision.fChi2 = vertices[0].getChi2();
        collision.fN = vertices[0].getNContributors();
      }
    } else {
      collision.fPosX = 0.f;
      collision.fPosY = 0.f;
      collision.fPosZ = 0.f;
      collision.fCovXX = 0.f;
      collision.fCovXY = 0.f;
      collision.fCovXZ = 0.f;
      collision.fCovYY = 0.f;
      collision.fCovYZ = 0.f;
      collision.fCovZZ = 0.f;
      collision.fChi2 = 0.01f;
      collision.fN = tracks->GetEntries();
    }
    collision.fCollisionTime = tzero[0] * 1.e3;    // [ps]
    collision.fCollisionTimeRes = tzero[1] * 1.e3; // [ps]
    tEvents->Fill();
    tBC->Fill();

    mccollision.fBCsID = ientry + eventOffset;
    mccollision.fGeneratorsID = 0;
    mccollision.fPosX = 0.;
    mccollision.fPosY = 0.;
    mccollision.fPosZ = 0.;
    mccollision.fT = 0.;
    mccollision.fWeight = 0.;
    mccollision.fImpactParameter = 0.;
    tMCvtx->Fill();

    mccollisionlabel.fLabel = ientry + eventOffset;
    mccollisionlabel.fLabelMask = 0;
    tCollisionLabels->Fill();
  }

  fout->mkdir("TF_0");
  fout->cd("TF_0");
  TreeList->Write();
  fout->ls();
  fout->Close();
}
