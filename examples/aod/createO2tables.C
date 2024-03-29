R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

// std::shuffle
#include <algorithm>
// std::default_random_engine
#include <random>
// std::chrono::system_clock
#include <chrono>

// ROOT includes
#include "TMath.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TRandom3.h"
#include "TDatabasePDG.h"
#include "TH1F.h"

// Delphes includes
#include "ExRootAnalysis/ExRootTreeReader.h"

// O2 includes
#include "DetectorsVertexing/PVertexer.h"
#include "DetectorsVertexing/PVertexerHelpers.h"
#include "Steer/InteractionSampler.h"
#include "CommonDataFormat/BunchFilling.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"
#include "DataFormatsFT0/RecPoints.h"
#include "Framework/DataTypes.h"

// DelphesO2 includes
#include "TrackSmearer.hh"
#include "TOFLayer.hh"
#include "RICHdetector.hh"
#include "ECALdetector.hh"
#include "PhotonConversion.hh"
#include "MIDdetector.hh"
#include "TrackUtils.hh"

#include "createO2tables.h"

// Detector parameters
const double Bz = 0.2; // [T]
// TOF
constexpr double tof_radius = 100.; // [cm] Radius of the TOF detector (used to compute acceptance)
const double tof_length = 200.;     // [cm] Length of the TOF detector (used to compute acceptance)
const double tof_sigmat = 0.02;     // [ns] Resolution of the TOF detector
const double tof_sigmat0 = 0.2;     // [ns] Time spread of the vertex
const char* tof_mismatch_file = "tofMM.root";
// Forward TOF
const double forward_tof_radius = 100.;   // [cm] Radius of the Forward TOF detector (used to compute acceptance)
const double forward_tof_radius_in = 10.; // [cm] Inner radius of the Forward TOF detector (used to compute acceptance)
const double forward_tof_length = 200.;   // [cm] Length of the Forward TOF detector (used to compute acceptance)
const double forward_tof_sigmat = 0.02;   // [ns] Resolution of the Forward TOF detector
const double forward_tof_sigmat0 = 0.2;   // [ns] Time spread of the vertex
// RICH
constexpr double rich_radius = 100.;    // [cm] Radius of the RICH detector (used to compute acceptance)
const double rich_length = 200.;        // [cm] Length of the RICH detector (used to compute acceptance)
const double rich_index = 1.03;         // Refraction index of the RICH detector
const double rich_radiator_length = 2.; // [cm] Radiator length of the RICH detector
const double rich_efficiency = 0.4;     // Efficiency of the RICH detector
const double rich_sigma = 7.e-3;        // [rad] Resolution of the RICH detector
// Forward RICH
const double forward_rich_radius = 100.;        // [cm] Radius of the Forward RICH detector (used to compute acceptance)
const double forward_rich_radius_in = 10.;      // [cm] Inner radius of the Forward RICH detector (used to compute acceptance)
const double forward_rich_length = 200.;        // [cm] Length of the Forward RICH detector (used to compute acceptance)
const double forward_rich_index = 1.0014;       // Refraction index of the Forward RICH detector
const double forward_rich_radiator_length = 95; // [cm] Radiator length of the Forward RICH detector
const double forward_rich_efficiency = 0.2;     // Efficiency of the Forward RICH detector
const double forward_rich_sigma = 1.5e-3;       // [rad] Resolution of the Forward RICH detector
// MID
const char* inputFileAccMuonPID = "muonAccEffPID.root";

// Simulation parameters
constexpr bool do_vertexing = true;  // Vertexing with the O2
constexpr bool enable_nuclei = true; // Nuclei LUTs
constexpr bool enable_ecal = true;   // Enable ECAL filling
constexpr bool debug_qa = false;     // Debug QA histograms
constexpr int tof_mismatch = 0;      // Flag to configure the TOF mismatch running mode: 0 off, 1 create, 2 use

int createO2tables(const char* inputFile = "delphes.root",
                   const char* outputFile = "AODRun5.root",
                   int eventOffset = 0)
{
  if ((inputFile != NULL) && (inputFile[0] == '\0')) {
    Printf("input file is empty, returning");
    return 0;
  }

  // Defining particles to transport
  TDatabasePDG::Instance()->AddParticle("deuteron", "deuteron", 1.8756134, kTRUE, 0.0, 3, "Nucleus", 1000010020);
  TDatabasePDG::Instance()->AddAntiParticle("anti-deuteron", -1000010020);

  TDatabasePDG::Instance()->AddParticle("triton", "triton", 2.8089218, kTRUE, 0.0, 3, "Nucleus", 1000010030);
  TDatabasePDG::Instance()->AddAntiParticle("anti-triton", -1000010030);

  TDatabasePDG::Instance()->AddParticle("helium3", "helium3", 2.80839160743, kTRUE, 0.0, 6, "Nucleus", 1000020030);
  TDatabasePDG::Instance()->AddAntiParticle("anti-helium3", -1000020030);

  if constexpr (do_vertexing) { // Load files for the vertexing
    o2::base::GeometryManager::loadGeometry("./", false);
    o2::base::Propagator::initFieldFromGRP("o2sim_grp.root");
  }

  // Debug histograms
  std::map<const char*, TH1F*> debugHisto;
  std::map<int, TH1F*> debugEffNum;
  std::map<int, TH1F*> debugEffDen;
  std::map<int, TH1F*> debugEffDenPart;
  if constexpr (debug_qa) { // Create histograms for debug QA
    debugHisto["Multiplicity"] = new TH1F("Multiplicity", "Multiplicity", 1000, 0, 5000);
  }

  // Create chain of root trees
  TChain chain("Delphes");
  chain.Add(inputFile);

  // Create object of class ExRootTreeReader
  auto treeReader = new ExRootTreeReader(&chain);
  const auto numberOfEntries = treeReader->GetEntries();

  // Get pointers to branches used in this analysis
  const auto events = treeReader->UseBranch("Event");
  const auto tracks = treeReader->UseBranch("Track");
  const auto particles = treeReader->UseBranch("Particle");

  // smearer
  o2::delphes::TrackSmearer smearer;
  std::map<int, const char*> mapPdgLut;
  mapPdgLut.insert(std::make_pair(11, "lutCovm.el.dat"));
  mapPdgLut.insert(std::make_pair(13, "lutCovm.mu.dat"));
  mapPdgLut.insert(std::make_pair(211, "lutCovm.pi.dat"));
  mapPdgLut.insert(std::make_pair(321, "lutCovm.ka.dat"));
  mapPdgLut.insert(std::make_pair(2212, "lutCovm.pr.dat"));
  if constexpr (enable_nuclei) {
    mapPdgLut.insert(std::make_pair(1000010020, "lutCovm.de.dat"));
    mapPdgLut.insert(std::make_pair(1000010030, "lutCovm.tr.dat"));
    mapPdgLut.insert(std::make_pair(1000020030, "lutCovm.he3.dat"));
  }
  for (auto e : mapPdgLut) {
    if (!smearer.loadTable(e.first, e.second)) {
      Printf("Having issue with loading the LUT %i '%s'", e.first, e.second);
      return 1;
    }
  }

  // TOF layer
  o2::delphes::TOFLayer tof_layer;
  tof_layer.setup(tof_radius, tof_length, tof_sigmat, tof_sigmat0);
  TH1F* hTOFMismatchTemplate = nullptr;
  if constexpr (tof_mismatch == 1) { // Create mode
    hTOFMismatchTemplate = new TH1F("hTOFMismatchTemplate", "", 3000., -5., 25.);
  } else if (tof_mismatch == 2) { // User mode
    TFile f(tof_mismatch_file, "READ");
    if (!f.IsOpen()) {
      Printf("Did not find file for input TOF mismatch distribution");
      return 1;
    }
    f.GetObject("hTOFMismatchTemplate", hTOFMismatchTemplate);
    hTOFMismatchTemplate->SetDirectory(0);
    f.Close();
  }

  // Forward TOF layer
  o2::delphes::TOFLayer forward_tof_layer;
  forward_tof_layer.setup(forward_tof_radius, forward_tof_length, forward_tof_sigmat, forward_tof_sigmat0);
  forward_tof_layer.setType(o2::delphes::TOFLayer::kForward);
  forward_tof_layer.setRadiusIn(forward_tof_radius_in);

  // RICH layer
  o2::delphes::RICHdetector rich_detector;
  rich_detector.setup(rich_radius, rich_length);
  rich_detector.setIndex(rich_index);
  rich_detector.setRadiatorLength(rich_radiator_length);
  rich_detector.setEfficiency(rich_efficiency);
  rich_detector.setSigma(rich_sigma);

  // Forward RICH layer
  o2::delphes::RICHdetector forward_rich_detector;
  forward_rich_detector.setup(forward_rich_radius, forward_rich_length);
  forward_rich_detector.setIndex(forward_rich_index);
  forward_rich_detector.setRadiatorLength(forward_rich_radiator_length);
  forward_rich_detector.setEfficiency(forward_rich_efficiency);
  forward_rich_detector.setSigma(forward_rich_sigma);
  forward_rich_detector.setType(o2::delphes::RICHdetector::kForward);
  forward_rich_detector.setRadiusIn(forward_rich_radius_in);

  // ECAL detector
  o2::delphes::ECALdetector ecal_detector;

  // Photon Conversion Method
  o2::delphes::PhotonConversion photon_conversion;
  TLorentzVector photonConv;


  // MID detector
  o2::delphes::MIDdetector mid_detector;
  const bool isMID = mid_detector.setup(inputFileAccMuonPID);
  if (isMID) {
    Printf("creating MID detector");
  }

  // create output
  auto fout = TFile::Open(outputFile, "RECREATE");
  // Make output Trees
  MakeTreeO2bc();
  MakeTreeO2track();
  MakeTreeO2trackCov();
  MakeTreeO2trackExtra();
  MakeTreeO2ftof();
  MakeTreeO2rich();
  MakeTreeO2ecal();
  MakeTreeO2frich();
  MakeTreeO2photon();
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

  // Define the PVertexer and its utilities
  o2::steer::InteractionSampler irSampler;
  irSampler.setInteractionRate(10000);
  irSampler.init();

  o2::vertexing::PVertexer vertexer;
  vertexer.setValidateWithIR(kFALSE);
  vertexer.setBunchFilling(irSampler.getBunchFilling());
  vertexer.init();

  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) { // Loop over events
    // Adjust start indices for this event in all trees by adding the number of entries of the previous event
    for (auto i = 0; i < kTrees; ++i) {
      eventextra.fStart[i] += eventextra.fNentries[i];
      eventextra.fNentries[i] = 0;
    }

    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    constexpr float multEtaRange = 2.f; // Range in eta to count the charged particles
    float dNdEta = 0.f;                 // Charged particle multiplicity to use in the efficiency evaluation
    TLorentzVector pECAL;               // 4-momentum of photon in ECAL

    for (Int_t iparticle = 0; iparticle < particles->GetEntries(); ++iparticle) { // Loop over particles
      auto particle = (GenParticle*)particles->At(iparticle);

      particle->SetUniqueID(iparticle + fOffsetLabel); // not sure this is needed, to be sure

      mcparticle.fIndexMcCollisions = ientry + eventOffset;
      mcparticle.fPdgCode = particle->PID;
      mcparticle.fStatusCode = particle->Status;
      mcparticle.fFlags = 0;
      if (IsSecondary(particles, iparticle)) {
        mcparticle.fFlags |= o2::aod::mcparticle::enums::ProducedByTransport;
      } else {
        mcparticle.fFlags |= o2::aod::mcparticle::enums::PhysicalPrimary;
      }
      mcparticle.fIndexMcParticles_Mother0 = particle->M1;
      if (mcparticle.fIndexMcParticles_Mother0 > -1)
        mcparticle.fIndexMcParticles_Mother0 += fOffsetLabel;
      mcparticle.fIndexMcParticles_Mother1 = particle->M2;
      if (mcparticle.fIndexMcParticles_Mother1 > -1)
        mcparticle.fIndexMcParticles_Mother1 += fOffsetLabel;
      mcparticle.fIndexMcParticles_Daughter0 = particle->D1;
      if (mcparticle.fIndexMcParticles_Daughter0 > -1)
        mcparticle.fIndexMcParticles_Daughter0 += fOffsetLabel;
      mcparticle.fIndexMcParticles_Daughter1 = particle->D2;
      if (mcparticle.fIndexMcParticles_Daughter1 > -1)
        mcparticle.fIndexMcParticles_Daughter1 += fOffsetLabel;
      mcparticle.fWeight = 1.;

      mcparticle.fPx = particle->Px;
      mcparticle.fPy = particle->Py;
      mcparticle.fPz = particle->Pz;
      mcparticle.fE = particle->E;

      mcparticle.fVx = particle->X * 0.1;
      mcparticle.fVy = particle->Y * 0.1;
      mcparticle.fVz = particle->Z * 0.1;
      mcparticle.fVt = particle->T;

      if (TMath::Abs(particle->Eta) <= multEtaRange && particle->D1 < 0 && particle->D2 < 0 && particle->Charge != 0) {
        dNdEta += 1.f;
      }
      FillTree(kMcParticle);

      // info for the ECAL
      if constexpr (enable_ecal) {
        float posZ, posPhi;
        if (ecal_detector.makeSignal(*particle, pECAL, posZ, posPhi)) { // to be updated 13.09.2021
          ecal.fIndexCollisions = ientry + eventOffset;
          ecal.fIndexMcParticles = TMath::Abs(iparticle + fOffsetLabel);
          ecal.fPx = pECAL.Px();
          ecal.fPy = pECAL.Py();
          ecal.fPz = pECAL.Pz();
          ecal.fE = pECAL.E();
          ecal.fPosZ = posZ;
          ecal.fPosPhi = posPhi;
          FillTree(kA3ECAL);
        }
      }

      // fill debug information

      // info for the PhotonConversion
      TLorentzVector photonConv;

      if (photon_conversion.hasPhotonConversion(*particle)) {
        if (photon_conversion.makeSignal(*particle, photonConv)) {
          photon.fIndexCollisions = ientry + eventOffset;
          photon.fIndexMcParticles = TMath::Abs(iparticle + fOffsetLabel);
          photon.fPx = photonConv.Px();
          photon.fPy = photonConv.Py();
          photon.fPz = photonConv.Pz();
          FillTree(kA3Photon);
        }
      }

      if constexpr (debug_qa) {
        if (!debugEffDenPart[particle->PID]) {
          debugEffDenPart[particle->PID] = new TH1F(Form("denPart%i", particle->PID), Form("denPart%i;#it{p}_{T} (GeV/#it{c})", particle->PID), 1000, 0, 10);
        }
        debugEffDenPart[particle->PID]->Fill(particle->PT);
      }
    }
    dNdEta = 0.5f * dNdEta / multEtaRange;
    if constexpr (debug_qa) {
      debugHisto["Multiplicity"]->Fill(dNdEta);
    }
    fOffsetLabel += particles->GetEntries();

    // For vertexing
    std::vector<TrackAlice3> tracks_for_vertexing;
    std::vector<o2::InteractionRecord> bcData;
    o2::InteractionRecord ir = irSampler.generateCollisionTime(); // Generate IR

    // Tracks used for the T0 evaluation
    std::vector<Track*> tof_tracks;
    std::vector<Track*> ftof_tracks;
    std::vector<std::pair<int, int>> ftof_tracks_indices;
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
      const auto track = (Track*)tracks->At(itrack);
      auto particle = (GenParticle*)track->Particle.GetObject();

      O2Track o2track; // tracks in internal O2 format
      o2::delphes::TrackUtils::convertTrackToO2Track(*track, o2track, true);
      if constexpr (debug_qa) {
        if (!debugEffDen[track->PID]) {
          debugEffDen[track->PID] = new TH1F(Form("den%i", track->PID), Form("den%i;#it{p}_{T} (GeV/#it{c})", track->PID), 1000, 0, 10);
        }
        debugEffDen[track->PID]->Fill(track->PT);
      }
      if (!smearer.smearTrack(o2track, track->PID, dNdEta)) { // Skipping inefficient/not correctly smeared tracks
        continue;
      }
      if constexpr (debug_qa) {
        if (!debugEffNum[track->PID]) {
          debugEffNum[track->PID] = new TH1F(Form("num%i", track->PID), Form("num%i;#it{p}_{T} (GeV/#it{c})", track->PID), 1000, 0, 10);
        }
        debugEffNum[track->PID]->Fill(track->PT);
      }
      o2::delphes::TrackUtils::convertO2TrackToTrack(o2track, *track, true);

      // fill the label tree
      Int_t alabel = particle->GetUniqueID();
      mctracklabel.fIndexMcParticles = TMath::Abs(alabel);
      mctracklabel.fMcMask = 0;
      FillTree(kMcTrackLabel);

      // set track information
      aod_track.fIndexCollisions = ientry + eventOffset;
      aod_track.fX = o2track.getX();
      aod_track.fAlpha = o2track.getAlpha();
      aod_track.fY = o2track.getY();
      aod_track.fZ = o2track.getZ();
      aod_track.fSnp = o2track.getSnp();
      aod_track.fTgl = o2track.getTgl();
      aod_track.fSigned1Pt = o2track.getQ2Pt();

      // Modified covariance matrix
      // First sigmas on the diagonal
      aod_track.fSigmaY = TMath::Sqrt(o2track.getSigmaY2());
      aod_track.fSigmaZ = TMath::Sqrt(o2track.getSigmaZ2());
      aod_track.fSigmaSnp = TMath::Sqrt(o2track.getSigmaSnp2());
      aod_track.fSigmaTgl = TMath::Sqrt(o2track.getSigmaTgl2());
      aod_track.fSigma1Pt = TMath::Sqrt(o2track.getSigma1Pt2());

      aod_track.fRhoZY = (Char_t)(128. * o2track.getSigmaZY() / aod_track.fSigmaZ / aod_track.fSigmaY);
      aod_track.fRhoSnpY = (Char_t)(128. * o2track.getSigmaSnpY() / aod_track.fSigmaSnp / aod_track.fSigmaY);
      aod_track.fRhoSnpZ = (Char_t)(128. * o2track.getSigmaSnpZ() / aod_track.fSigmaSnp / aod_track.fSigmaZ);
      aod_track.fRhoTglY = (Char_t)(128. * o2track.getSigmaTglY() / aod_track.fSigmaTgl / aod_track.fSigmaY);
      aod_track.fRhoTglZ = (Char_t)(128. * o2track.getSigmaTglZ() / aod_track.fSigmaTgl / aod_track.fSigmaZ);
      aod_track.fRhoTglSnp = (Char_t)(128. * o2track.getSigmaTglSnp() / aod_track.fSigmaTgl / aod_track.fSigmaSnp);
      aod_track.fRho1PtY = (Char_t)(128. * o2track.getSigma1PtY() / aod_track.fSigma1Pt / aod_track.fSigmaY);
      aod_track.fRho1PtZ = (Char_t)(128. * o2track.getSigma1PtZ() / aod_track.fSigma1Pt / aod_track.fSigmaZ);
      aod_track.fRho1PtSnp = (Char_t)(128. * o2track.getSigma1PtSnp() / aod_track.fSigma1Pt / aod_track.fSigmaSnp);
      aod_track.fRho1PtTgl = (Char_t)(128. * o2track.getSigma1PtTgl() / aod_track.fSigma1Pt / aod_track.fSigmaTgl);

      //FIXME this needs to be fixed
      aod_track.fITSClusterMap = 3;
      aod_track.fFlags = 4;

      //FIXME this also needs to be fixed
      aod_track.fTrackEtaEMCAL = 0; //track->GetTrackEtaOnEMCal();
      aod_track.fTrackPhiEMCAL = 0; //track->GetTrackPhiOnEMCal();

      aod_track.fLength = track->L * 0.1; // [cm]
      // check if has hit the TOF
      if (tof_layer.hasTOF(*track)) {

        if constexpr (tof_mismatch != 0) {
          const auto L = std::sqrt(track->XOuter * track->XOuter + track->YOuter * track->YOuter + track->ZOuter * track->ZOuter);
          if constexpr (tof_mismatch == 1) { // Created mode: fill output mismatch template
            hTOFMismatchTemplate->Fill(track->TOuter * 1.e9 - L / 299.79246);
          } else if constexpr (tof_mismatch == 2) { // User mode: do some random mismatch
            auto lutEntry = smearer.getLUTEntry(track->PID, dNdEta, 0., o2track.getEta(), 1. / o2track.getQ2Pt());
            if (lutEntry && lutEntry->valid) {  // Check that LUT entry is valid
              if constexpr (tof_radius < 50.) { // Inner TOF
                if (gRandom->Uniform() < (1.f - lutEntry->itof)) {
                  track->TOuter = (hTOFMismatchTemplate->GetRandom() + L / 299.79246) * 1.e-9;
                }
              } else { // Outer TOF
                if (gRandom->Uniform() < (1.f - lutEntry->otof)) {
                  track->TOuter = (hTOFMismatchTemplate->GetRandom() + L / 299.79246) * 1.e-9;
                }
              }
            }
          }
        }

        aod_track.fTOFChi2 = 1.f;                     // Negative if TOF is not available
        aod_track.fTOFSignal = track->TOuter * 1.e12; // [ps]
        aod_track.fTrackTime = track->TOuter * 1.e9;  // [ns]
        aod_track.fTrackTimeRes = 200 * 1.e9;         // [ns]
        aod_track.fTOFExpMom = track->P * 0.029979246;
        // if primary push to TOF tracks
        if (fabs(aod_track.fY) < 3. * aod_track.fSigmaY && fabs(aod_track.fZ) < 3. * aod_track.fSigmaZ)
          tof_tracks.push_back(track);
      } else {
        aod_track.fTOFChi2 = -1.f;
        aod_track.fTOFSignal = -999.f;
        aod_track.fTrackTime = -999.f;
        aod_track.fTrackTimeRes = 2000 * 1.e9;
        aod_track.fTOFExpMom = -999.f;
      }

      // check if has hit on RICH
      if (rich_detector.hasRICH(*track)) {
        const auto measurement = rich_detector.getMeasuredAngle(*track);
        rich.fIndexCollisions = ientry + eventOffset;
        rich.fIndexTracks = fTrackCounter; // Index in the Track table
        rich.fRICHSignal = measurement.first;
        rich.fRICHSignalError = measurement.second;
        std::array<float, 5> deltaangle, nsigma;
        rich_detector.makePID(*track, deltaangle, nsigma);
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

      // check if has hit on the forward RICH
      if (forward_rich_detector.hasRICH(*track)) {
        const auto measurement = forward_rich_detector.getMeasuredAngle(*track);
        frich.fIndexCollisions = ientry + eventOffset;
        frich.fIndexTracks = fTrackCounter; // Index in the Track table
        frich.fRICHSignal = measurement.first;
        frich.fRICHSignalError = measurement.second;
        std::array<float, 5> deltaangle, nsigma;
        forward_rich_detector.makePID(*track, deltaangle, nsigma);
        frich.fRICHDeltaEl = deltaangle[0];
        frich.fRICHDeltaMu = deltaangle[1];
        frich.fRICHDeltaPi = deltaangle[2];
        frich.fRICHDeltaKa = deltaangle[3];
        frich.fRICHDeltaPr = deltaangle[4];
        frich.fRICHNsigmaEl = nsigma[0];
        frich.fRICHNsigmaMu = nsigma[1];
        frich.fRICHNsigmaPi = nsigma[2];
        frich.fRICHNsigmaKa = nsigma[3];
        frich.fRICHNsigmaPr = nsigma[4];
        FillTree(kFRICH);
      }

      // check if has Forward TOF
      if (forward_tof_layer.hasTOF(*track)) {
        ftof_tracks.push_back(track);
        ftof_tracks_indices.push_back(std::pair<int, int>{ientry + eventOffset, fTrackCounter});
      }


      // check if it is within the acceptance of the MID
      if (isMID) {
        if (mid_detector.hasMID(*track)) {
          mid.fIndexCollisions = ientry + eventOffset;
          mid.fIndexTracks = fTrackCounter; // Index in the Track table
          mid.fMIDIsMuon = mid_detector.isMuon(*track, multiplicity);
          FillTree(kMID);
        }
      }
      if constexpr (do_vertexing) {
        const float t = (ir.bc2ns() + gRandom->Gaus(0., 100.)) * 1e-3;
        tracks_for_vertexing.push_back(TrackAlice3{o2track, t, 100.f * 1e-3, TMath::Abs(alabel)});
      }
      FillTree(kTracks);
      FillTree(kTracksCov);
      FillTree(kTracksExtra);
      fTrackCounter++;
      // fill histograms
    }

    // Filling the fTOF tree after computing its T0
    std::array<float, 2> ftzero;

    forward_tof_layer.eventTime(ftof_tracks, ftzero);
    for (unsigned int i = 0; i < ftof_tracks.size(); i++) {
      auto track = ftof_tracks[i];
      ftof.fIndexCollisions = ftof_tracks_indices[i].first;
      ftof.fIndexTracks = ftof_tracks_indices[i].second; // Index in the Track table

      ftof.fFTOFLength = track->L * 0.1;        // [cm]
      ftof.fFTOFSignal = track->TOuter * 1.e12; // [ps]

      std::array<float, 5> deltat, nsigma;
      forward_tof_layer.makePID(*track, deltat, nsigma);
      ftof.fFTOFDeltaEl = deltat[0];
      ftof.fFTOFDeltaMu = deltat[1];
      ftof.fFTOFDeltaPi = deltat[2];
      ftof.fFTOFDeltaKa = deltat[3];
      ftof.fFTOFDeltaPr = deltat[4];
      ftof.fFTOFNsigmaEl = nsigma[0];
      ftof.fFTOFNsigmaMu = nsigma[1];
      ftof.fFTOFNsigmaPi = nsigma[2];
      ftof.fFTOFNsigmaKa = nsigma[3];
      ftof.fFTOFNsigmaPr = nsigma[4];
      FillTree(kFTOF);
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
    if (!tof_layer.eventTime(tof_tracks, tzero) && tof_tracks.size() > 0) {
      Printf("Issue when evaluating the start time");
      return 1;
    }

    // fill collision information
    collision.fIndexBCs = ientry + eventOffset;
    bc.fGlobalBC = ientry + eventOffset;
    if constexpr (do_vertexing) { // Performing vertexing
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
      const int n_vertices = vertexer.process(tracks_for_vertexing,
                                              idxVec,
                                              gsl::span<o2::InteractionRecord>{bcData},
                                              vertices,
                                              vertexTrackIDs,
                                              v2tRefs,
                                              gsl::span<const o2::MCCompLabel>{lblTracks},
                                              lblVtx);
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
        int index = 0;
        int hm = 0;
        for (int i = 0; i < n_vertices; i++) {
          //in case of multiple vertices select the vertex with the higher multiplicities
          if (vertices[i].getNContributors() > hm) {
            hm = vertices[i].getNContributors();
            index = i;
          }
        }
        collision.fPosX = vertices[index].getX();
        collision.fPosY = vertices[index].getY();
        collision.fPosZ = vertices[index].getZ();
        collision.fCovXX = vertices[index].getSigmaX2();
        collision.fCovXY = vertices[index].getSigmaXY();
        collision.fCovXZ = vertices[index].getSigmaXZ();
        collision.fCovYY = vertices[index].getSigmaY2();
        collision.fCovYZ = vertices[index].getSigmaYZ();
        collision.fCovZZ = vertices[index].getSigmaZ2();
        collision.fFlags = 0;
        collision.fChi2 = vertices[index].getChi2();
        collision.fN = vertices[index].getNContributors();
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
  const TObjArray* out_tag = out_dir.Tokenize(".");
  out_dir = out_tag->GetEntries() > 1 ? out_tag->At(1)->GetName() : "";
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
  fout->cd();
  for (auto e : debugHisto) {
    e.second->Write();
  }
  for (auto e : debugEffNum) {
    e.second->Write();
  }
  for (auto e : debugEffDen) {
    e.second->Write();
  }
  for (auto e : debugEffDenPart) {
    e.second->Write();
  }
  fout->ls();
  fout->Close();

  Printf("AOD written!");
  if constexpr (tof_mismatch == 1) {
    Printf("Writing the template for TOF mismatch");
    hTOFMismatchTemplate->SaveAs(Form("tof_mismatch_template_%s.root", out_dir.Data()));
  }
  return 0;
}
