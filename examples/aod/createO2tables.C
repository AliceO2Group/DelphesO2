R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

#include <algorithm> // std::shuffle
#include <random> // std::default_random_engine
#include <chrono> // std::chrono::system_clock

// ROOT includes
#include "TMath.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TRandom3.h"
#include "TDatabasePDG.h"

// Delphes includes
#include "ExRootAnalysis/ExRootTreeReader.h"

// O2 includes
#include "DetectorsVertexing/PVertexer.h"
#include "DetectorsVertexing/PVertexerHelpers.h"
#include "CommonDataFormat/BunchFilling.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"
#include "DataFormatsFT0/RecPoints.h"

// DelphesO2 includes
#include "TrackSmearer.hh"
#include "TOFLayer.hh"
#include "RICHdetector.hh"
#include "MIDdetector.hh"
#include "TrackUtils.hh"

#include "createO2tables.h"

// Detector parameters
const double Bz = 0.2; // [T]
// TOF
const double tof_radius = 100.; // [cm] Radius of the TOF detector (used to compute acceptance)
const double tof_length = 200.; // [cm] Length of the TOF detector (used to compute acceptance)
const double tof_sigmat = 0.02; // [ns] Resolution of the TOF detector
const double tof_sigmat0 = 0.2; // [ns] Time spread of the vertex
// RICH
const double rich_radius = 100.; // [cm] Radius of the RICH detector (used to compute acceptance)
const double rich_length = 200.; // [cm] Length of the RICH detector (used to compute acceptance)
const double rich_index = 1.03;
const double rich_radiator_length = 2.;
const double rich_efficiency = 0.4;
const double rich_sigma = 7.e-3;
// MID
const char* inputFileAccMuonPID = "muonAccEffPID.root";

// Simulation parameters
const bool do_vertexing = true;

// Class to hold the information for the O2 vertexing
class TrackAlice3 : public o2::track::TrackParCov
{
  using timeEst = o2::dataformats::TimeStampWithError<float, float>;

 public:
  TrackAlice3() = default;
  ~TrackAlice3() = default;
  TrackAlice3(const TrackAlice3& src) = default;
  TrackAlice3(const o2::track::TrackParCov& src, const float t = 0, const float te = 1, const int label = 0) : o2::track::TrackParCov(src), mTimeMUS{t, te}, mLabel{label} {}
  const timeEst& getTimeMUS() const { return mTimeMUS; }
  const int mLabel;

 private:
  timeEst mTimeMUS; ///< time estimate in ns
};

template <typename T>
bool IsSecondary(const T& particleTree, const int& index)
{
  auto particle = (GenParticle*)particleTree->At(index);
  if (particle->M1 < 0) {
    return false;
  }

  auto mother = (GenParticle*)particleTree->At(particle->M1);
  if (!mother) {
    return false;
  }
  // Ancore di salvezza :)
  if ((particle->M1 == particle->M2) && (particle->M1 == 0)) {
    return false;
  }
  if (abs(mother->PID) <= 8) {
    return false;
  }
  // 100% secondaries if true here
  switch (abs(mother->PID)) {
    // K0S
    case 310:
    // Lambda
    case 3122:
    // Sigma0
    case 3212:
    // Sigma-
    case 3112:
    // Sigma+
    case 3222:
    // Xi-
    case 3312:
    // Xi0
    case 3322:
    // Omega-
    case 3334:
      return true;
      break;
  }

  return IsSecondary(particleTree, particle->M1);
}

int createO2tables(const char* inputFile = "delphes.root",
                   const char* outputFile = "AODRun5.root",
                   int eventOffset = 0)
{
  if ((inputFile != NULL) && (inputFile[0] == '\0')) {
    Printf("input file is empty, returning");
    return 0;
  }

  TDatabasePDG::Instance()->AddParticle("deuteron", "deuteron", 1.8756134, kTRUE, 0.0, 1, "Nucleus", 1000010020);
  TDatabasePDG::Instance()->AddAntiParticle("anti-deuteron", -1000010020);

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
  smearer.loadTable(11, "lutCovm.el.dat");
  smearer.loadTable(13, "lutCovm.mu.dat");
  smearer.loadTable(211, "lutCovm.pi.dat");
  smearer.loadTable(321, "lutCovm.ka.dat");
  smearer.loadTable(2212, "lutCovm.pr.dat");

  // TOF layer
  o2::delphes::TOFLayer toflayer;
  toflayer.setup(tof_radius, tof_length, tof_sigmat, tof_sigmat0);
  // RICH layer
  o2::delphes::RICHdetector richdetector;
  richdetector.setup(rich_radius, rich_length);
  richdetector.setIndex(rich_index);
  richdetector.setRadiatorLength(rich_radiator_length);
  richdetector.setEfficiency(rich_efficiency);
  richdetector.setSigma(rich_sigma);
  // MID detector
  o2::delphes::MIDdetector midDetector;
  printf("creating MID detector...\n");
  bool isMID = midDetector.setup(inputFileAccMuonPID);
  printf("isMID = %d\n", isMID);

  // create output
  auto fout = TFile::Open(outputFile, "RECREATE");
  // Make output Trees
  MakeTreeO2bc();
  MakeTreeO2track();
  MakeTreeO2trackCov();
  MakeTreeO2trackExtra();
  MakeTreeO2rich();
  MakeTreeO2mid();
  MakeTreeO2collision();
  MakeTreeO2collisionExtra();
  MakeTreeO2mccollision();
  MakeTreeO2mcparticle();
  MakeTreeO2mctracklabel();
  MakeTreeO2mccollisionlabel();

  const UInt_t mTrackX = 0xFFFFFFFF;
  const UInt_t mTrackAlpha = 0xFFFFFFFF;
  const UInt_t mtrackSnp = 0xFFFFFFFF;
  const UInt_t mTrackTgl = 0xFFFFFFFF;
  const UInt_t mTrack1Pt = 0xFFFFFFFF;     // Including the momentun at the inner wall of TPC
  const UInt_t mTrackCovDiag = 0xFFFFFFFF; // Including the chi2
  const UInt_t mTrackCovOffDiag = 0xFFFFFFFF;
  const UInt_t mTrackSignal = 0xFFFFFFFF; // PID signals and track length

  // Counters
  int fOffsetLabel = 0;
  int fTrackCounter = 0; // Counter for the track index, needed for derived tables e.g. RICH. To be incremented at every track filled!

  // Random generator for reshuffling tracks when reading them
  std::default_random_engine e(std::chrono::system_clock::now().time_since_epoch().count()); // time-based seed:
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {                               // Loop over events
    // Adjust start indices for this event in all trees by adding the number of entries of the previous event
    for (auto i = 0; i < kTrees; ++i) {
      eventextra.fStart[i] += eventextra.fNentries[i];
      eventextra.fNentries[i] = 0;
    }

    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);

    for (Int_t iparticle = 0; iparticle < particles->GetEntries(); ++iparticle) { // Loop over particles
      auto particle = (GenParticle*)particles->At(iparticle);

      particle->SetUniqueID(iparticle + fOffsetLabel); // not sure this is needed, to be sure

      mcparticle.fIndexMcCollisions = ientry + eventOffset;
      mcparticle.fPdgCode = particle->PID;
      mcparticle.fStatusCode = particle->Status;
      mcparticle.fFlags = 0;
      if (IsSecondary(particles, iparticle)) {
        mcparticle.fFlags |= 1;
      }
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

      mcparticle.fVx = particle->X * 0.1;
      mcparticle.fVy = particle->Y * 0.1;
      mcparticle.fVz = particle->Z * 0.1;
      mcparticle.fVt = particle->T;

      FillTree(kMcParticle);
    }
    fOffsetLabel += particles->GetEntries();

    // loop over tracks
    std::vector<TrackAlice3> tracks_for_vertexing;
    std::vector<o2::InteractionRecord> bcData;
    std::vector<Track*> tof_tracks;
    const int multiplicity = tracks->GetEntries();

    // Build index array of tracks to randomize track writing order
    std::vector<int> tracks_indices(tracks->GetEntries());              // vector with tracks->GetEntries()
    std::iota(std::begin(tracks_indices), std::end(tracks_indices), 0); // Fill with 0, 1, ...
    std::shuffle(tracks_indices.begin(), tracks_indices.end(), e);

    // Flags to check that all the indices are written
    bool did_first = tracks->GetEntries() == 0;
    bool did_last = tracks->GetEntries() == 0;
    for (Int_t itrack : tracks_indices) { // Loop over tracks
      if (itrack == 0) {
        did_first = true;
      }
      if (itrack == tracks->GetEntries() - 1) {
        did_last = true;
      }
      if (itrack < 0) {
        Printf("Got a negative index!");
        return 1;
      }

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
      mctracklabel.fIndexMcParticles = TMath::Abs(alabel);
      mctracklabel.fMcMask = 0;
      FillTree(kMcTrackLabel);

      // set track information
      mytracks.fIndexCollisions = ientry + eventOffset;
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
      // check if has hit on RICH
      if (richdetector.hasRICH(*track)) {
        const auto measurement = richdetector.getMeasuredAngle(*track);
        rich.fIndexCollisions = ientry + eventOffset;
        rich.fIndexTracks = fTrackCounter; // Index in the Track table
        rich.fRICHSignal = measurement.first;
        rich.fRICHSignalError = measurement.second;
        std::array<float, 5> deltaangle, nsigma;
        richdetector.makePID(*track, deltaangle, nsigma);
        rich.fRICHDeltaEl = deltaangle[0];
        rich.fRICHDeltaMu = deltaangle[1];
        rich.fRICHDeltaPi = deltaangle[2];
        rich.fRICHDeltaKa = deltaangle[3];
        rich.fRICHDeltaPr = deltaangle[4];
        rich.fRICHNsigmaEl = nsigma[0];
        rich.fRICHNsigmaMu = nsigma[1];
        rich.fRICHNsigmaPi = nsigma[2];
        rich.fRICHNsigmaKa = nsigma[3];
        rich.fRICHNsigmaPr = nsigma[4];
        FillTree(kRICH);
      }
      // check if it is within the acceptance of the MID
      if (isMID) {
        if (midDetector.hasMID(*track)) {
          mid.fIndexCollisions = ientry + eventOffset;
          mid.fIndexTracks = fTrackCounter; // Index in the Track table
          mid.fMIDIsMuon = midDetector.isMuon(*track, multiplicity);
          FillTree(kMID);
        }
      }
      if (do_vertexing) {
        o2::InteractionRecord ir(ientry + eventOffset, 0);
        const float t = (ir.bc2ns() + gRandom->Gaus(0., 100.)) * 1e-3;
        tracks_for_vertexing.push_back(TrackAlice3{o2track, t, 100.f * 1e-3, TMath::Abs(alabel)});
      }
      FillTree(kTracks);
      FillTree(kTracksCov);
      FillTree(kTracksExtra);
      fTrackCounter++;
      // fill histograms
    }
    if (eventextra.fNentries[kTracks] != eventextra.fNentries[kTracksCov] || eventextra.fNentries[kTracks] != eventextra.fNentries[kTracksExtra]) {
      Printf("Issue with the counters");
      return 1;
    }
    if (!did_first) {
      Printf("Did not read first track");
      return 1;
    }
    if (!did_last) {
      Printf("Did not read last track");
      return 1;
    }

    // compute the event time
    std::array<float, 2> tzero;
    if (!toflayer.eventTime(tof_tracks, tzero) && tof_tracks.size() > 0) {
      Printf("Issue when evaluating the start time");
      return 1;
    }

    // fill collision information
    collision.fIndexBCs = ientry + eventOffset;
    bc.fGlobalBC = ientry + eventOffset;
    if (do_vertexing) { // Performing vertexing
      o2::BunchFilling bcfill;
      bcfill.setDefault();
      o2::vertexing::PVertexer vertexer;
      vertexer.setValidateWithIR(kFALSE);
      vertexer.setBunchFilling(bcfill);
      vertexer.init();

      std::vector<o2::MCCompLabel> lblTracks;
      std::vector<o2::vertexing::PVertex> vertices;
      std::vector<o2::vertexing::GIndex> vertexTrackIDs;
      std::vector<o2::vertexing::V2TRef> v2tRefs;
      std::vector<o2::MCEventLabel> lblVtx;
      lblVtx.emplace_back(ientry + eventOffset, 1);
      std::vector<o2::dataformats::GlobalTrackID> idxVec; // here we will the global IDs of all used tracks
      idxVec.reserve(tracks_for_vertexing.size());
      for (unsigned i = 0; i < tracks_for_vertexing.size(); i++) {
        lblTracks.emplace_back(tracks_for_vertexing[i].mLabel, ientry + eventOffset, 1, false);
        idxVec.emplace_back(i, o2::dataformats::GlobalTrackID::ITS);
      }
      vertexer.setStartIR({0, 0});
      const int n_vertices = vertexer.process(gsl::span<const TrackAlice3>{tracks_for_vertexing},
                                              idxVec,
                                              gsl::span<o2::InteractionRecord>{bcData},
                                              vertices,
                                              vertexTrackIDs,
                                              v2tRefs,
                                              gsl::span<const o2::MCCompLabel>{lblTracks},
                                              lblVtx);
      // Printf("Found %i vertices with %zu tracks", n_vertices, tracks_for_vertexing.size());
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
        collision.fFlags = 0;
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
        collision.fFlags = 0;
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
      collision.fFlags = 0;
      collision.fChi2 = 0.01f;
      collision.fN = tracks->GetEntries();
    }
    collision.fCollisionTime = tzero[0];    // [ns]
    collision.fCollisionTimeRes = tzero[1]; // [ns]
    FillTree(kEvents);
    FillTree(kBC);

    mccollision.fIndexBCs = ientry + eventOffset;
    mccollision.fGeneratorsID = 0;
    mccollision.fPosX = 0.;
    mccollision.fPosY = 0.;
    mccollision.fPosZ = 0.;
    mccollision.fT = 0.;
    mccollision.fWeight = 0.;
    mccollision.fImpactParameter = 0.;
    FillTree(kMcCollision);

    mccollisionlabel.fIndexMcCollisions = ientry + eventOffset;
    mccollisionlabel.fMcMask = 0;
    FillTree(kMcCollisionLabel);

    FillTree(kEventsExtra);
  }

  Printf("Writing tables for %i events", eventextra.fStart[kEvents] + 1);
  TString out_dir = outputFile;
  out_dir.ReplaceAll(".root", "");
  out_dir.ReplaceAll("AODRun5.", "");
  if (!out_dir.IsDec()) {
    out_dir = "DF_0";
  } else {
    out_dir = Form("DF_%i", out_dir.Atoi());
  }
  fout->mkdir(out_dir);
  fout->cd(out_dir);
  for (int i = 0; i < kTrees; i++) {
    if (Trees[i])
      Trees[i]->Write();
  }
  fout->ls();
  fout->Close();

  Printf("AOD written!");
  return 0;
}
