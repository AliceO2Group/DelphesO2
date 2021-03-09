#include "TTree.h"

enum TreeIndex { // Index of the output trees
  kEvents = 0,
  kEventsExtra,
  kTracks,
  kTracksCov,
  kTracksExtra,
  kCalo,
  kCaloTrigger,
  kMuon,
  kMuonCls,
  kZdc,
  kFV0A,
  kFV0C,
  kFT0,
  kFDD,
  kV0s,
  kCascades,
  kTOF,
  kMcParticle,
  kMcCollision,
  kMcTrackLabel,
  kMcCaloLabel,
  kMcCollisionLabel,
  kBC,
  kRun2BCInfo,
  kHMPID,
  kRICH,
  kTrees
};

const int fBasketSizeEvents = 1000000;  // Maximum basket size of the trees for events
const int fBasketSizeTracks = 10000000; // Maximum basket size of the trees for tracks

const TString TreeName[kTrees] = {"O2collision", "DbgEventExtra", "O2track", "O2trackcov", "O2trackextra", "O2calo", "O2calotrigger", "O2muon", "O2muoncluster", "O2zdc", "O2fv0a", "O2fv0c", "O2ft0", "O2fdd", "O2v0", "O2cascade", "O2tof", "O2mcparticle", "O2mccollision", "O2mctracklabel", "O2mccalolabel", "O2mccollisionlabel", "O2bc", "O2run2bcinfo", "O2hmpid", "O2rich"};
const TString TreeTitle[kTrees] = {"Collision tree", "Collision extra", "Barrel tracks Parameters", "Barrel tracks Covariance", "Barrel tracks Extra", "Calorimeter cells", "Calorimeter triggers", "MUON tracks", "MUON clusters", "ZDC", "FV0A", "FV0C", "FT0", "FDD", "V0s", "Cascades", "TOF hits", "Kinematics", "MC collisions", "MC track labels", "MC calo labels", "MC collision labels", "BC info", "Run 2 BC Info", "HMPID info", "RICH info"};

TTree* Trees[kTrees] = {nullptr}; // Array of created TTrees
TTree* CreateTree(TreeIndex t)
{
  TTree* tree = new TTree(TreeName[t], TreeTitle[t]);
  tree->SetAutoFlush(0);
  Trees[t] = tree;
  return tree;
}

struct {
  // Event data
  Int_t fIndexBCs = 0u; /// Index to BC table
  // Primary vertex position
  Float_t fPosX = -999.f; /// Primary vertex x coordinate
  Float_t fPosY = -999.f; /// Primary vertex y coordinate
  Float_t fPosZ = -999.f; /// Primary vertex z coordinate
  // Primary vertex covariance matrix
  Float_t fCovXX = 999.f; /// cov[0]
  Float_t fCovXY = 0.f;   /// cov[1]
  Float_t fCovXZ = 0.f;   /// cov[2]
  Float_t fCovYY = 999.f; /// cov[3]
  Float_t fCovYZ = 0.f;   /// cov[4]
  Float_t fCovZZ = 999.f; /// cov[5]
  // Quality parameters
  UShort_t fFlags = 0;   /// Vertex type
  Float_t fChi2 = 999.f; /// Chi2 of the vertex
  UShort_t fN = 0u;      /// Number of contributors

  // The calculation of event time certainly will be modified in Run3
  // The prototype below can be switched on request
  Float_t fCollisionTime = -999.f;    /// Event time (t0) obtained with different methods (best, T0, T0-TOF, ...)
  Float_t fCollisionTimeRes = -999.f; /// Resolution on the event time (t0) obtained with different methods (best, T0, T0-TOF, ...)
  UChar_t fCollisionTimeMask = 0u;    /// Mask with the method used to compute the event time (0x1=T0-TOF,0x2=T0A,0x3=TOC) for each momentum bins

} collision; //! structure to keep the primary vertex (avoid name conflicts)

void MakeTreeO2collision()
{
  TTree* tEvents = CreateTree(kEvents);
  tEvents->Branch("fIndexBCs", &collision.fIndexBCs, "fIndexBCs/I");
  tEvents->Branch("fPosX", &collision.fPosX, "fPosX/F");
  tEvents->Branch("fPosY", &collision.fPosY, "fPosY/F");
  tEvents->Branch("fPosZ", &collision.fPosZ, "fPosZ/F");
  tEvents->Branch("fCovXX", &collision.fCovXX, "fCovXX/F");
  tEvents->Branch("fCovXY", &collision.fCovXY, "fCovXY/F");
  tEvents->Branch("fCovXZ", &collision.fCovXZ, "fCovXZ/F");
  tEvents->Branch("fCovYY", &collision.fCovYY, "fCovYY/F");
  tEvents->Branch("fCovYZ", &collision.fCovYZ, "fCovYZ/F");
  tEvents->Branch("fCovZZ", &collision.fCovZZ, "fCovZZ/F");
  tEvents->Branch("fFlags", &collision.fFlags, "fFlags/s");
  tEvents->Branch("fChi2", &collision.fChi2, "fChi2/F");
  tEvents->Branch("fNumContrib", &collision.fN, "fNumContrib/s");
  tEvents->Branch("fCollisionTime", &collision.fCollisionTime, "fCollisionTime/F");
  tEvents->Branch("fCollisionTimeRes", &collision.fCollisionTimeRes, "fCollisionTimeRes/F");
  tEvents->Branch("fCollisionTimeMask", &collision.fCollisionTimeMask, "fCollisionTimeMask/b");
  tEvents->SetBasketSize("*", fBasketSizeEvents);
}

struct {
  // Start indices and numbers of elements for data in the other trees matching this vertex.
  // Needed for random access of collision-related data, allowing skipping data discarded by the user
  Int_t fStart[kTrees] = {0};    /// Start entry indices for data in the other trees matching this vertex
  Int_t fNentries[kTrees] = {0}; /// Numbers of entries for data in the other trees matching this vertex
} eventextra;                    //! structure for benchmarking information

void MakeTreeO2collisionExtra()
{
  TTree* tEventsExtra = CreateTree(kEventsExtra);
  TString sstart = TString::Format("fStart[%d]/I", kTrees);
  TString sentries = TString::Format("fNentries[%d]/I", kTrees);
  tEventsExtra->Branch("fStart", eventextra.fStart, sstart.Data());
  tEventsExtra->Branch("fNentries", eventextra.fNentries, sentries.Data());
  tEventsExtra->SetBasketSize("*", fBasketSizeEvents);
}

struct {
  // MC collision
  Int_t fIndexBCs = 0u;       /// Index to BC table
  Short_t fGeneratorsID = 0u; /// Generator ID used for the MC
  Float_t fPosX = -999.f;     /// Primary vertex x coordinate from MC
  Float_t fPosY = -999.f;     /// Primary vertex y coordinate from MC
  Float_t fPosZ = -999.f;     /// Primary vertex z coordinate from MC
  Float_t fT = -999.f;        /// Time of the collision from MC
  Float_t fWeight = -999.f;   /// Weight from MC
  // Generation details (HepMC3 in the future)
  Float_t fImpactParameter = -999.f; /// Impact parameter from MC
} mccollision;                       //! MC collisions = vertices

void MakeTreeO2mccollision()
{
  TTree* tMCvtx = CreateTree(kMcCollision);
  tMCvtx->Branch("fIndexBCs", &mccollision.fIndexBCs, "fIndexBCs/I");
  tMCvtx->Branch("fGeneratorsID", &mccollision.fGeneratorsID, "fGeneratorsID/S");
  tMCvtx->Branch("fPosX", &mccollision.fPosX, "fPosX/F");
  tMCvtx->Branch("fPosY", &mccollision.fPosY, "fPosY/F");
  tMCvtx->Branch("fPosZ", &mccollision.fPosZ, "fPosZ/F");
  tMCvtx->Branch("fT", &mccollision.fT, "fT/F");
  tMCvtx->Branch("fWeight", &mccollision.fWeight, "fWeight/F");
  tMCvtx->Branch("fImpactParameter", &mccollision.fImpactParameter, "fImpactParameter/F");
  tMCvtx->SetBasketSize("*", fBasketSizeEvents);
}

struct {
  int fRunNumber = -1;         /// Run number
  ULong64_t fGlobalBC = 0u;    /// Unique bunch crossing id. Contains period, orbit and bunch crossing numbers
  ULong64_t fTriggerMask = 0u; /// Trigger class mask
} bc;                          //! structure to keep trigger-related info

void MakeTreeO2bc()
{
  TTree* tBC = CreateTree(kBC);
  tBC->Branch("fRunNumber", &bc.fRunNumber, "fRunNumber/I");
  tBC->Branch("fGlobalBC", &bc.fGlobalBC, "fGlobalBC/l");
  tBC->Branch("fTriggerMask", &bc.fTriggerMask, "fTriggerMask/l");
  tBC->SetBasketSize("*", fBasketSizeEvents);
}

struct {
  // Track data

  Int_t fIndexCollisions = -1; /// The index of the collision vertex in the TF, to which the track is attached

  uint8_t fTrackType = 0; // Type of track: global, ITS standalone, tracklet, ...

  // In case we need connection to TOF clusters, activate next lines
  // Int_t   fTOFclsIndex;     /// The index of the associated TOF cluster
  // Int_t   fNTOFcls;         /// The number of TOF clusters

  // Coordinate system parameters
  Float_t fX = -999.f;     /// X coordinate for the point of parametrisation
  Float_t fAlpha = -999.f; /// Local <--> global coor.system rotation angle

  // Track parameters
  Float_t fY = -999.f;         /// fP[0] local Y-coordinate of a track (cm)
  Float_t fZ = -999.f;         /// fP[1] local Z-coordinate of a track (cm)
  Float_t fSnp = -999.f;       /// fP[2] local sine of the track momentum azimuthal angle
  Float_t fTgl = -999.f;       /// fP[3] tangent of the track momentum dip angle
  Float_t fSigned1Pt = -999.f; /// fP[4] 1/pt (1/(GeV/c))

  // "Covariance matrix"
  // The diagonal elements represent the errors = Sqrt(C[i,i])
  // The off-diagonal elements are the correlations = C[i,j]/Sqrt(C[i,i])/Sqrt(C[j,j])
  // The off-diagonal elements are multiplied by 128 (7bits) and packed in Char_t
  Float_t fSigmaY = -999.f;   /// Sqrt(fC[0])
  Float_t fSigmaZ = -999.f;   /// Sqrt(fC[2])
  Float_t fSigmaSnp = -999.f; /// Sqrt(fC[5])
  Float_t fSigmaTgl = -999.f; /// Sqrt(fC[9])
  Float_t fSigma1Pt = -999.f; /// Sqrt(fC[14])
  Char_t fRhoZY = 0;          /// 128*fC[1]/SigmaZ/SigmaY
  Char_t fRhoSnpY = 0;        /// 128*fC[3]/SigmaSnp/SigmaY
  Char_t fRhoSnpZ = 0;        /// 128*fC[4]/SigmaSnp/SigmaZ
  Char_t fRhoTglY = 0;        /// 128*fC[6]/SigmaTgl/SigmaY
  Char_t fRhoTglZ = 0;        /// 128*fC[7]/SigmaTgl/SigmaZ
  Char_t fRhoTglSnp = 0;      /// 128*fC[8]/SigmaTgl/SigmaSnp
  Char_t fRho1PtY = 0;        /// 128*fC[10]/Sigma1Pt/SigmaY
  Char_t fRho1PtZ = 0;        /// 128*fC[11]/Sigma1Pt/SigmaZ
  Char_t fRho1PtSnp = 0;      /// 128*fC[12]/Sigma1Pt/SigmaSnp
  Char_t fRho1PtTgl = 0;      /// 128*fC[13]/Sigma1Pt/SigmaTgl

  // Additional track parameters
  Float_t fTPCinnerP = -999.f; /// Full momentum at the inner wall of TPC for dE/dx PID

  // Track quality parameters
  UInt_t fFlags = 0u; /// Reconstruction status flags

  // Clusters and tracklets
  UChar_t fITSClusterMap = 0u;                 /// ITS map of clusters, one bit per a layer
  UChar_t fTPCNClsFindable = 0u;               /// number of clusters that could be assigned in the TPC
  Char_t fTPCNClsFindableMinusFound = 0;       /// difference between foundable and found clusters
  Char_t fTPCNClsFindableMinusCrossedRows = 0; ///  difference between foundable clsuters and crossed rows
  UChar_t fTPCNClsShared = 0u;                 /// Number of shared clusters
  UChar_t fTRDPattern = 0u;                    /// Bit 0-5 if tracklet from TRD layer used for this track

  // Chi2
  Float_t fITSChi2NCl = -999.f; /// chi2/Ncl ITS
  Float_t fTPCChi2NCl = -999.f; /// chi2/Ncl TPC
  Float_t fTRDChi2 = -999.f;    /// chi2 TRD match (?)
  Float_t fTOFChi2 = -999.f;    /// chi2 TOF match (?)

  // PID
  Float_t fTPCSignal = -999.f; /// dE/dX TPC
  Float_t fTRDSignal = -999.f; /// dE/dX TRD
  Float_t fTOFSignal = -999.f; /// TOFsignal
  Float_t fLength = -999.f;    /// Int.Lenght @ TOF
  Float_t fTOFExpMom = -999.f; /// TOF Expected momentum based on the expected time of pions

  // Track extrapolation to EMCAL surface
  Float_t fTrackEtaEMCAL = -999.f; /// Track eta at the EMCAL surface
  Float_t fTrackPhiEMCAL = -999.f; /// Track phi at the EMCAL surface
} mytracks;                        //! structure to keep track information

void MakeTreeO2track()
{
  TTree* tTracks = CreateTree(kTracks);
  tTracks->Branch("fIndexCollisions", &mytracks.fIndexCollisions, "fIndexCollisions/I");
  tTracks->Branch("fTrackType", &mytracks.fTrackType, "fTrackType/b");
  tTracks->Branch("fX", &mytracks.fX, "fX/F");
  tTracks->Branch("fAlpha", &mytracks.fAlpha, "fAlpha/F");
  tTracks->Branch("fY", &mytracks.fY, "fY/F");
  tTracks->Branch("fZ", &mytracks.fZ, "fZ/F");
  tTracks->Branch("fSnp", &mytracks.fSnp, "fSnp/F");
  tTracks->Branch("fTgl", &mytracks.fTgl, "fTgl/F");
  tTracks->Branch("fSigned1Pt", &mytracks.fSigned1Pt, "fSigned1Pt/F");
  tTracks->SetBasketSize("*", fBasketSizeTracks);
}

void MakeTreeO2trackCov()
{
  TTree* tTracksCov = CreateTree(kTracksCov);
  // Modified covariance matrix
  tTracksCov->Branch("fSigmaY", &mytracks.fSigmaY, "fSigmaY/F");
  tTracksCov->Branch("fSigmaZ", &mytracks.fSigmaZ, "fSigmaZ/F");
  tTracksCov->Branch("fSigmaSnp", &mytracks.fSigmaSnp, "fSigmaSnp/F");
  tTracksCov->Branch("fSigmaTgl", &mytracks.fSigmaTgl, "fSigmaTgl/F");
  tTracksCov->Branch("fSigma1Pt", &mytracks.fSigma1Pt, "fSigma1Pt/F");
  tTracksCov->Branch("fRhoZY", &mytracks.fRhoZY, "fRhoZY/B");
  tTracksCov->Branch("fRhoSnpY", &mytracks.fRhoSnpY, "fRhoSnpY/B");
  tTracksCov->Branch("fRhoSnpZ", &mytracks.fRhoSnpZ, "fRhoSnpZ/B");
  tTracksCov->Branch("fRhoTglY", &mytracks.fRhoTglY, "fRhoTglY/B");
  tTracksCov->Branch("fRhoTglZ", &mytracks.fRhoTglZ, "fRhoTglZ/B");
  tTracksCov->Branch("fRhoTglSnp", &mytracks.fRhoTglSnp, "fRhoTglSnp/B");
  tTracksCov->Branch("fRho1PtY", &mytracks.fRho1PtY, "fRho1PtY/B");
  tTracksCov->Branch("fRho1PtZ", &mytracks.fRho1PtZ, "fRho1PtZ/B");
  tTracksCov->Branch("fRho1PtSnp", &mytracks.fRho1PtSnp, "fRho1PtSnp/B");
  tTracksCov->Branch("fRho1PtTgl", &mytracks.fRho1PtTgl, "fRho1PtTgl/B");
  tTracksCov->SetBasketSize("*", fBasketSizeTracks);
}

void MakeTreeO2trackExtra()
{
  TTree* tTracksExtra = CreateTree(kTracksExtra);
  //Extra
  tTracksExtra->Branch("fTPCInnerParam", &mytracks.fTPCinnerP, "fTPCInnerParam/F");
  tTracksExtra->Branch("fFlags", &mytracks.fFlags, "fFlags/i");
  tTracksExtra->Branch("fITSClusterMap", &mytracks.fITSClusterMap, "fITSClusterMap/b");
  tTracksExtra->Branch("fTPCNClsFindable", &mytracks.fTPCNClsFindable, "fTPCNClsFindable/b");
  tTracksExtra->Branch("fTPCNClsFindableMinusFound", &mytracks.fTPCNClsFindableMinusFound, "fTPCNClsFindableMinusFound/B");
  tTracksExtra->Branch("fTPCNClsFindableMinusCrossedRows", &mytracks.fTPCNClsFindableMinusCrossedRows, "fTPCNClsFindableMinusCrossedRows/B");
  tTracksExtra->Branch("fTPCNClsShared", &mytracks.fTPCNClsShared, "fTPCNClsShared/b");
  tTracksExtra->Branch("fTRDPattern", &mytracks.fTRDPattern, "fTRDPattern/b");
  tTracksExtra->Branch("fITSChi2NCl", &mytracks.fITSChi2NCl, "fITSChi2NCl/F");
  tTracksExtra->Branch("fTPCChi2NCl", &mytracks.fTPCChi2NCl, "fTPCChi2NCl/F");
  tTracksExtra->Branch("fTRDChi2", &mytracks.fTRDChi2, "fTRDChi2/F");
  tTracksExtra->Branch("fTOFChi2", &mytracks.fTOFChi2, "fTOFChi2/F");
  tTracksExtra->Branch("fTPCSignal", &mytracks.fTPCSignal, "fTPCSignal/F");
  tTracksExtra->Branch("fTRDSignal", &mytracks.fTRDSignal, "fTRDSignal/F");
  tTracksExtra->Branch("fTOFSignal", &mytracks.fTOFSignal, "fTOFSignal/F");
  tTracksExtra->Branch("fLength", &mytracks.fLength, "fLength/F");
  tTracksExtra->Branch("fTOFExpMom", &mytracks.fTOFExpMom, "fTOFExpMom/F");
  tTracksExtra->Branch("fTrackEtaEMCAL", &mytracks.fTrackEtaEMCAL, "fTrackEtaEMCAL/F");
  tTracksExtra->Branch("fTrackPhiEMCAL", &mytracks.fTrackPhiEMCAL, "fTrackPhiEMCAL/F");
  tTracksExtra->SetBasketSize("*", fBasketSizeTracks);
}

struct {
  // RICH data
  Int_t fIndexCollisions = -1; /// Collision ID
  Int_t fIndexTracks = -1;     /// Track ID

  Float_t fRICHSignal = -999.f;      /// RICH signal
  Float_t fRICHSignalError = -999.f; /// RICH signal error
  Float_t fRICHDeltaEl = -999.f;     /// Delta for El
  Float_t fRICHDeltaMu = -999.f;     /// Delta for Mu
  Float_t fRICHDeltaPi = -999.f;     /// Delta for Pi
  Float_t fRICHDeltaKa = -999.f;     /// Delta for Ka
  Float_t fRICHDeltaPr = -999.f;     /// Delta for Pr
  Float_t fRICHNsigmaEl = -999.f;    /// Nsigma for El
  Float_t fRICHNsigmaMu = -999.f;    /// Nsigma for Mu
  Float_t fRICHNsigmaPi = -999.f;    /// Nsigma for Pi
  Float_t fRICHNsigmaKa = -999.f;    /// Nsigma for Ka
  Float_t fRICHNsigmaPr = -999.f;    /// Nsigma for Pr
} rich;                              //! structure to keep RICH info

void MakeTreeO2rich()
{
  TTree* t = CreateTree(kRICH);
  t->Branch("fIndexCollisions", &rich.fIndexCollisions, "fIndexCollisions/I");
  t->Branch("fIndexTracks", &rich.fIndexTracks, "fIndexTracks/I");
  t->Branch("fRICHSignal", &rich.fRICHSignal, "fRICHSignal/F");
  t->Branch("fRICHSignalError", &rich.fRICHSignalError, "fRICHSignalError/F");
  t->Branch("fRICHDeltaEl", &rich.fRICHDeltaEl, "fRICHDeltaEl/F");
  t->Branch("fRICHDeltaMu", &rich.fRICHDeltaMu, "fRICHDeltaMu/F");
  t->Branch("fRICHDeltaPi", &rich.fRICHDeltaPi, "fRICHDeltaPi/F");
  t->Branch("fRICHDeltaKa", &rich.fRICHDeltaKa, "fRICHDeltaKa/F");
  t->Branch("fRICHDeltaPr", &rich.fRICHDeltaPr, "fRICHDeltaPr/F");
  t->Branch("fRICHNsigmaEl", &rich.fRICHNsigmaEl, "fRICHNsigmaEl/F");
  t->Branch("fRICHNsigmaMu", &rich.fRICHNsigmaMu, "fRICHNsigmaMu/F");
  t->Branch("fRICHNsigmaPi", &rich.fRICHNsigmaPi, "fRICHNsigmaPi/F");
  t->Branch("fRICHNsigmaKa", &rich.fRICHNsigmaKa, "fRICHNsigmaKa/F");
  t->Branch("fRICHNsigmaPr", &rich.fRICHNsigmaPr, "fRICHNsigmaPr/F");
  t->SetBasketSize("*", fBasketSizeTracks);
}

struct {
  // MC particle

  Int_t fIndexMcCollisions = -1; /// The index of the MC collision vertex

  // MC information (modified version of TParticle
  Int_t fPdgCode = -99999;    /// PDG code of the particle
  Int_t fStatusCode = -99999; /// generation status code
  uint8_t fFlags = 0;         /// See enum MCParticleFlags
  Int_t fMother0 = 0;         /// Indices of the mother particles
  Int_t fMother1 = 0;
  Int_t fDaughter0 = 0; /// Indices of the daughter particles
  Int_t fDaughter1 = 0;
  Float_t fWeight = 1; /// particle weight from the generator or ML

  Float_t fPx = -999.f; /// x component of momentum
  Float_t fPy = -999.f; /// y component of momentum
  Float_t fPz = -999.f; /// z component of momentum
  Float_t fE = -999.f;  /// Energy (covers the case of resonances, no need for calculated mass)

  Float_t fVx = -999.f; /// x of production vertex
  Float_t fVy = -999.f; /// y of production vertex
  Float_t fVz = -999.f; /// z of production vertex
  Float_t fVt = -999.f; /// t of production vertex
  // We do not use the polarisation so far
} mcparticle; //! MC particles from the kinematics tree

void MakeTreeO2mcparticle()
{
  TTree* tKinematics = CreateTree(kMcParticle);
  tKinematics->Branch("fIndexMcCollisions", &mcparticle.fIndexMcCollisions, "fIndexMcCollisions/I");
  tKinematics->Branch("fPdgCode", &mcparticle.fPdgCode, "fPdgCode/I");
  tKinematics->Branch("fStatusCode", &mcparticle.fStatusCode, "fStatusCode/I");
  tKinematics->Branch("fFlags", &mcparticle.fFlags, "fFlags/b");
  tKinematics->Branch("fMother0", &mcparticle.fMother0, "fMother0/I");
  tKinematics->Branch("fMother1", &mcparticle.fMother1, "fMother1/I");
  tKinematics->Branch("fDaughter0", &mcparticle.fDaughter0, "fDaughter0/I");
  tKinematics->Branch("fDaughter1", &mcparticle.fDaughter1, "fDaughter1/I");
  tKinematics->Branch("fWeight", &mcparticle.fWeight, "fWeight/F");
  tKinematics->Branch("fPx", &mcparticle.fPx, "fPx/F");
  tKinematics->Branch("fPy", &mcparticle.fPy, "fPy/F");
  tKinematics->Branch("fPz", &mcparticle.fPz, "fPz/F");
  tKinematics->Branch("fE", &mcparticle.fE, "fE/F");
  tKinematics->Branch("fVx", &mcparticle.fVx, "fVx/F");
  tKinematics->Branch("fVy", &mcparticle.fVy, "fVy/F");
  tKinematics->Branch("fVz", &mcparticle.fVz, "fVz/F");
  tKinematics->Branch("fVt", &mcparticle.fVt, "fVt/F");
  tKinematics->SetBasketSize("*", fBasketSizeTracks);
}

struct {
  // Track label to find the corresponding MC particle
  Int_t fIndexMcParticles = 0; /// Track label
  UShort_t fMcMask = 0;        /// Bit mask to indicate detector mismatches (bit ON means mismatch)
                               /// Bit 0-6: mismatch at ITS layer
                               /// Bit 7-9: # of TPC mismatches in the ranges 0, 1, 2-3, 4-7, 8-15, 16-31, 32-63, >64
                               /// Bit 10: TRD, bit 11: TOF, bit 15: negative label sign
} mctracklabel;                //! Track labels

void MakeTreeO2mctracklabel()
{
  TTree* tLabels = CreateTree(kMcTrackLabel);
  tLabels->Branch("fIndexMcParticles", &mctracklabel.fIndexMcParticles, "fIndexMcParticles/I");
  tLabels->Branch("fMcMask", &mctracklabel.fMcMask, "fMcMask/s");
  tLabels->SetBasketSize("*", fBasketSizeTracks);
}

struct {
  // MC collision label
  Int_t fIndexMcCollisions = 0; /// Collision label
  UShort_t fMcMask = 0;         /// Bit mask to indicate collision mismatches (bit ON means mismatch)
                                /// bit 15: negative label sign
} mccollisionlabel;             //! Collision labels

void MakeTreeO2mccollisionlabel()
{
  TTree* tCollisionLabels = CreateTree(kMcCollisionLabel);
  tCollisionLabels->Branch("fIndexMcCollisions", &mccollisionlabel.fIndexMcCollisions, "fIndexMcCollisions/I");
  tCollisionLabels->Branch("fMcMask", &mccollisionlabel.fMcMask, "fMcMask/s");
  tCollisionLabels->SetBasketSize("*", fBasketSizeEvents);
}

void FillTree(TreeIndex t)
{
  Trees[t]->Fill();
  eventextra.fNentries[t]++;
}
