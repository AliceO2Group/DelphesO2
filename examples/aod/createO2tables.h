struct {
  // Event data
  Int_t fBCsID = 0u;       /// Index to BC table
  // Primary vertex position
  Float_t  fPosX = -999.f;       /// Primary vertex x coordinate
  Float_t  fPosY = -999.f;       /// Primary vertex y coordinate
  Float_t  fPosZ = -999.f;       /// Primary vertex z coordinate
  // Primary vertex covariance matrix
  Float_t  fCovXX = 999.f;    /// cov[0]
  Float_t  fCovXY = 0.f;      /// cov[1]
  Float_t  fCovXZ = 0.f;      /// cov[2]
  Float_t  fCovYY = 999.f;    /// cov[3]
  Float_t  fCovYZ = 0.f;      /// cov[4]
  Float_t  fCovZZ = 999.f;    /// cov[5]
  // Quality parameters
  Float_t  fChi2 = 999.f;             /// Chi2 of the vertex
  UInt_t   fN = 0u;                /// Number of contributors

  // The calculation of event time certainly will be modified in Run3
  // The prototype below can be switched on request
  Float_t fCollisionTime = -999.f;    /// Event time (t0) obtained with different methods (best, T0, T0-TOF, ...)
  Float_t fCollisionTimeRes = -999.f; /// Resolution on the event time (t0) obtained with different methods (best, T0, T0-TOF, ...)
  UChar_t fCollisionTimeMask = 0u;    /// Mask with the method used to compute the event time (0x1=T0-TOF,0x2=T0A,0x3=TOC) for each momentum bins

} collision; //! structure to keep the primary vertex (avoid name conflicts)

TTree* MakeTreeO2collision()
{
  TTree* tEvents = new TTree("O2collision", "Collision tree");
  tEvents->Branch("fBCsID", &collision.fBCsID, "fBCsID/I");
  tEvents->Branch("fPosX", &collision.fPosX, "fPosX/F");
  tEvents->Branch("fPosY", &collision.fPosY, "fPosY/F");
  tEvents->Branch("fPosZ", &collision.fPosZ, "fPosZ/F");
  tEvents->Branch("fCovXX", &collision.fCovXX, "fCovXX/F");
  tEvents->Branch("fCovXY", &collision.fCovXY, "fCovXY/F");
  tEvents->Branch("fCovXZ", &collision.fCovXZ, "fCovXZ/F");
  tEvents->Branch("fCovYY", &collision.fCovYY, "fCovYY/F");
  tEvents->Branch("fCovYZ", &collision.fCovYZ, "fCovYZ/F");
  tEvents->Branch("fCovZZ", &collision.fCovZZ, "fCovZZ/F");
  tEvents->Branch("fChi2", &collision.fChi2, "fChi2/F");
  tEvents->Branch("fNumContrib", &collision.fN, "fNumContrib/i");
  tEvents->Branch("fCollisionTime", &collision.fCollisionTime, "fCollisionTime/F");
  tEvents->Branch("fCollisionTimeRes", &collision.fCollisionTimeRes, "fCollisionTimeRes/F");
  tEvents->Branch("fCollisionTimeMask", &collision.fCollisionTimeMask, "fCollisionTimeMask/b");
  return tEvents;
}

void ConnectTreeO2collision(TTree *tEvents)
{
  tEvents->SetBranchAddress("fBCsID", &collision.fBCsID);
  tEvents->SetBranchAddress("fPosX", &collision.fPosX);
  tEvents->SetBranchAddress("fPosY", &collision.fPosY);
  tEvents->SetBranchAddress("fPosZ", &collision.fPosZ);
  tEvents->SetBranchAddress("fCovXX", &collision.fCovXX);
  tEvents->SetBranchAddress("fCovXY", &collision.fCovXY);
  tEvents->SetBranchAddress("fCovXZ", &collision.fCovXZ);
  tEvents->SetBranchAddress("fCovYY", &collision.fCovYY);
  tEvents->SetBranchAddress("fCovYZ", &collision.fCovYZ);
  tEvents->SetBranchAddress("fCovZZ", &collision.fCovZZ);
  tEvents->SetBranchAddress("fChi2", &collision.fChi2);
  tEvents->SetBranchAddress("fNumContrib", &collision.fN);
  tEvents->SetBranchAddress("fCollisionTime", &collision.fCollisionTime);
  tEvents->SetBranchAddress("fCollisionTimeRes", &collision.fCollisionTimeRes);
  tEvents->SetBranchAddress("fCollisionTimeMask", &collision.fCollisionTimeMask);
}

struct {
  // MC collision
  Int_t fBCsID = 0u;       /// Index to BC table
  Short_t fGeneratorsID = 0u; /// Generator ID used for the MC
  Float_t fPosX = -999.f;  /// Primary vertex x coordinate from MC
  Float_t fPosY = -999.f;  /// Primary vertex y coordinate from MC
  Float_t fPosZ = -999.f;  /// Primary vertex z coordinate from MC
  Float_t fT = -999.f;  /// Time of the collision from MC
  Float_t fWeight = -999.f;  /// Weight from MC
  // Generation details (HepMC3 in the future)
  Float_t fImpactParameter = -999.f; /// Impact parameter from MC
} mccollision;  //! MC collisions = vertices

TTree* MakeTreeO2mccollision()
{
  TTree* tMCvtx = new TTree("O2mccollision", "MC Collision tree");
  tMCvtx->Branch("fBCsID", &mccollision.fBCsID, "fBCsID/I");
  tMCvtx->Branch("fGeneratorsID", &mccollision.fGeneratorsID, "fGeneratorsID/S");
  tMCvtx->Branch("fPosX", &mccollision.fPosX, "fPosX/F");
  tMCvtx->Branch("fPosY", &mccollision.fPosY, "fPosY/F");
  tMCvtx->Branch("fPosZ", &mccollision.fPosZ, "fPosZ/F");
  tMCvtx->Branch("fT", &mccollision.fT, "fT/F");
  tMCvtx->Branch("fWeight", &mccollision.fWeight, "fWeight/F");
  tMCvtx->Branch("fImpactParameter", &mccollision.fImpactParameter, "fImpactParameter/F");
  return tMCvtx;
}
struct {
  int fRunNumber = -1;         /// Run number
  ULong64_t fGlobalBC = 0u;    /// Unique bunch crossing id. Contains period, orbit and bunch crossing numbers
  ULong64_t fTriggerMask = 0u; /// Trigger class mask
} bc; //! structure to keep trigger-related info

TTree* MakeTreeO2bc()
{
  TTree* tBC = new TTree("O2bc", "BC info");
  tBC->Branch("fRunNumber", &bc.fRunNumber, "fRunNumber/I");
  tBC->Branch("fGlobalBC", &bc.fGlobalBC, "fGlobalBC/l");
  tBC->Branch("fTriggerMask", &bc.fTriggerMask, "fTriggerMask/l");
  return tBC;
}

struct {
  // Track data
  Int_t   fCollisionsID = -1;    /// The index of the collision vertex in the TF, to which the track is attached

  uint8_t fTrackType = 0;       // Type of track: global, ITS standalone, tracklet, ...

  // In case we need connection to TOF clusters, activate next lines
  // Int_t   fTOFclsIndex;     /// The index of the associated TOF cluster
  // Int_t   fNTOFcls;         /// The number of TOF clusters

  // Coordinate system parameters
  Float_t fX = -999.f;     /// X coordinate for the point of parametrisation
  Float_t fAlpha = -999.f; /// Local <--> global coor.system rotation angle

  // Track parameters
  Float_t fY = -999.f;          /// fP[0] local Y-coordinate of a track (cm)
  Float_t fZ = -999.f;          /// fP[1] local Z-coordinate of a track (cm)
  Float_t fSnp = -999.f;        /// fP[2] local sine of the track momentum azimuthal angle
  Float_t fTgl = -999.f;        /// fP[3] tangent of the track momentum dip angle
  Float_t fSigned1Pt = -999.f;  /// fP[4] 1/pt (1/(GeV/c))

  // "Covariance matrix"
  // The diagonal elements represent the errors = Sqrt(C[i,i])
  // The off-diagonal elements are the correlations = C[i,j]/Sqrt(C[i,i])/Sqrt(C[j,j])
  // The off-diagonal elements are multiplied by 128 (7bits) and packed in Char_t
  Float_t fSigmaY      = -999.f; /// Sqrt(fC[0])
  Float_t fSigmaZ      = -999.f; /// Sqrt(fC[2])
  Float_t fSigmaSnp    = -999.f; /// Sqrt(fC[5])
  Float_t fSigmaTgl    = -999.f; /// Sqrt(fC[9])
  Float_t fSigma1Pt    = -999.f; /// Sqrt(fC[14])
  Char_t fRhoZY        = 0;      /// 128*fC[1]/SigmaZ/SigmaY
  Char_t fRhoSnpY      = 0;      /// 128*fC[3]/SigmaSnp/SigmaY
  Char_t fRhoSnpZ      = 0;      /// 128*fC[4]/SigmaSnp/SigmaZ
  Char_t fRhoTglY      = 0;      /// 128*fC[6]/SigmaTgl/SigmaY
  Char_t fRhoTglZ      = 0;      /// 128*fC[7]/SigmaTgl/SigmaZ
  Char_t fRhoTglSnp    = 0;      /// 128*fC[8]/SigmaTgl/SigmaSnp
  Char_t fRho1PtY      = 0;      /// 128*fC[10]/Sigma1Pt/SigmaY
  Char_t fRho1PtZ      = 0;      /// 128*fC[11]/Sigma1Pt/SigmaZ
  Char_t fRho1PtSnp    = 0;      /// 128*fC[12]/Sigma1Pt/SigmaSnp
  Char_t fRho1PtTgl    = 0;      /// 128*fC[13]/Sigma1Pt/SigmaTgl

  // Additional track parameters
  Float_t fTPCinnerP = -999.f; /// Full momentum at the inner wall of TPC for dE/dx PID

  // Track quality parameters
  ULong64_t fFlags = 0u;       /// Reconstruction status flags

  // Clusters and tracklets
  UChar_t fITSClusterMap = 0u;   /// ITS map of clusters, one bit per a layer
  UChar_t fTPCNClsFindable = 0u; /// number of clusters that could be assigned in the TPC
  Char_t fTPCNClsFindableMinusFound = 0;       /// difference between foundable and found clusters
  Char_t fTPCNClsFindableMinusCrossedRows = 0; ///  difference between foundable clsuters and crossed rows
  UChar_t fTPCNClsShared = 0u;   /// Number of shared clusters
  UChar_t fTRDPattern = 0u;   /// Bit 0-5 if tracklet from TRD layer used for this track

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
} mytracks;                      //! structure to keep track information

TTree* MakeTreeO2track()
{
  TTree* fTracks = new TTree("O2track", "Barrel tracks");
  fTracks->Branch("fCollisionsID", &mytracks.fCollisionsID, "fCollisionsID/I");
  fTracks->Branch("fTrackType", &mytracks.fTrackType, "fTrackType/b");
  //    fTracks->Branch("fTOFclsIndex", &mytracks.fTOFclsIndex, "fTOFclsIndex/I");
  //    fTracks->Branch("fNTOFcls", &mytracks.fNTOFcls, "fNTOFcls/I");
  fTracks->Branch("fX", &mytracks.fX, "fX/F");
  fTracks->Branch("fAlpha", &mytracks.fAlpha, "fAlpha/F");
  fTracks->Branch("fY", &mytracks.fY, "fY/F");
  fTracks->Branch("fZ", &mytracks.fZ, "fZ/F");
  fTracks->Branch("fSnp", &mytracks.fSnp, "fSnp/F");
  fTracks->Branch("fTgl", &mytracks.fTgl, "fTgl/F");
  fTracks->Branch("fSigned1Pt", &mytracks.fSigned1Pt, "fSigned1Pt/F");
  // Modified covariance matrix
  fTracks->Branch("fSigmaY", &mytracks.fSigmaY, "fSigmaY/F");
  fTracks->Branch("fSigmaZ", &mytracks.fSigmaZ, "fSigmaZ/F");
  fTracks->Branch("fSigmaSnp", &mytracks.fSigmaSnp, "fSigmaSnp/F");
  fTracks->Branch("fSigmaTgl", &mytracks.fSigmaTgl, "fSigmaTgl/F");
  fTracks->Branch("fSigma1Pt", &mytracks.fSigma1Pt, "fSigma1Pt/F");
  fTracks->Branch("fRhoZY", &mytracks.fRhoZY, "fRhoZY/B");
  fTracks->Branch("fRhoSnpY", &mytracks.fRhoSnpY, "fRhoSnpY/B");
  fTracks->Branch("fRhoSnpZ", &mytracks.fRhoSnpZ, "fRhoSnpZ/B");
  fTracks->Branch("fRhoTglY", &mytracks.fRhoTglY, "fRhoTglY/B");
  fTracks->Branch("fRhoTglZ", &mytracks.fRhoTglZ, "fRhoTglZ/B");
  fTracks->Branch("fRhoTglSnp", &mytracks.fRhoTglSnp, "fRhoTglSnp/B");
  fTracks->Branch("fRho1PtY", &mytracks.fRho1PtY, "fRho1PtY/B");
  fTracks->Branch("fRho1PtZ", &mytracks.fRho1PtZ, "fRho1PtZ/B");
  fTracks->Branch("fRho1PtSnp", &mytracks.fRho1PtSnp, "fRho1PtSnp/B");
  fTracks->Branch("fRho1PtTgl", &mytracks.fRho1PtTgl, "fRho1PtTgl/B");
  //
  fTracks->Branch("fTPCInnerParam", &mytracks.fTPCinnerP, "fTPCInnerParam/F");
  fTracks->Branch("fFlags", &mytracks.fFlags, "fFlags/l");
  fTracks->Branch("fITSClusterMap", &mytracks.fITSClusterMap, "fITSClusterMap/b");
  fTracks->Branch("fTPCNClsFindable", &mytracks.fTPCNClsFindable, "fTPCNClsFindable/b");
  fTracks->Branch("fTPCNClsFindableMinusFound",&mytracks.fTPCNClsFindableMinusFound, "fTPCNClsFindableMinusFound/B");
  fTracks->Branch("fTPCNClsFindableMinusCrossedRows", &mytracks.fTPCNClsFindableMinusCrossedRows, "fTPCNClsFindableMinusCrossedRows/B");
  fTracks->Branch("fTPCNClsShared", &mytracks.fTPCNClsShared, "fTPCNClsShared/b");
  fTracks->Branch("fTRDPattern", &mytracks.fTRDPattern, "fTRDPattern/b");
  fTracks->Branch("fITSChi2NCl", &mytracks.fITSChi2NCl, "fITSChi2NCl/F");
  fTracks->Branch("fTPCChi2NCl", &mytracks.fTPCChi2NCl, "fTPCChi2NCl/F");
  fTracks->Branch("fTRDChi2", &mytracks.fTRDChi2, "fTRDChi2/F");
  fTracks->Branch("fTOFChi2", &mytracks.fTOFChi2, "fTOFChi2/F");
  fTracks->Branch("fTPCSignal", &mytracks.fTPCSignal, "fTPCSignal/F");
  fTracks->Branch("fTRDSignal", &mytracks.fTRDSignal, "fTRDSignal/F");
  fTracks->Branch("fTOFSignal", &mytracks.fTOFSignal, "fTOFSignal/F");
  fTracks->Branch("fLength", &mytracks.fLength, "fLength/F");
  fTracks->Branch("fTOFExpMom", &mytracks.fTOFExpMom, "fTOFExpMom/F");
  return fTracks;
}

void ConnectTreeO2track(TTree *fTracks)
{
  fTracks->SetBranchAddress("fCollisionsID", &mytracks.fCollisionsID);
  fTracks->SetBranchAddress("fTrackType", &mytracks.fTrackType);
  //    fTracks->SetBranchAddress("fTOFclsIndex", &mytracks.fTOFclsIndex);
  //    fTracks->SetBranchAddress("fNTOFcls", &mytracks.fNTOFcls);
  fTracks->SetBranchAddress("fX", &mytracks.fX);
  fTracks->SetBranchAddress("fAlpha", &mytracks.fAlpha);
  fTracks->SetBranchAddress("fY", &mytracks.fY);
  fTracks->SetBranchAddress("fZ", &mytracks.fZ);
  fTracks->SetBranchAddress("fSnp", &mytracks.fSnp);
  fTracks->SetBranchAddress("fTgl", &mytracks.fTgl);
  fTracks->SetBranchAddress("fSigned1Pt", &mytracks.fSigned1Pt);
  // Modified covariance matrix
  fTracks->SetBranchAddress("fSigmaY", &mytracks.fSigmaY);
  fTracks->SetBranchAddress("fSigmaZ", &mytracks.fSigmaZ);
  fTracks->SetBranchAddress("fSigmaSnp", &mytracks.fSigmaSnp);
  fTracks->SetBranchAddress("fSigmaTgl", &mytracks.fSigmaTgl);
  fTracks->SetBranchAddress("fSigma1Pt", &mytracks.fSigma1Pt);
  fTracks->SetBranchAddress("fRhoZY", &mytracks.fRhoZY);
  fTracks->SetBranchAddress("fRhoSnpY", &mytracks.fRhoSnpY);
  fTracks->SetBranchAddress("fRhoSnpZ", &mytracks.fRhoSnpZ);
  fTracks->SetBranchAddress("fRhoTglY", &mytracks.fRhoTglY);
  fTracks->SetBranchAddress("fRhoTglZ", &mytracks.fRhoTglZ);
  fTracks->SetBranchAddress("fRhoTglSnp", &mytracks.fRhoTglSnp);
  fTracks->SetBranchAddress("fRho1PtY", &mytracks.fRho1PtY);
  fTracks->SetBranchAddress("fRho1PtZ", &mytracks.fRho1PtZ);
  fTracks->SetBranchAddress("fRho1PtSnp", &mytracks.fRho1PtSnp);
  fTracks->SetBranchAddress("fRho1PtTgl", &mytracks.fRho1PtTgl);
  //
  fTracks->SetBranchAddress("fTPCInnerParam", &mytracks.fTPCinnerP);
  fTracks->SetBranchAddress("fFlags", &mytracks.fFlags);
  fTracks->SetBranchAddress("fITSClusterMap", &mytracks.fITSClusterMap);
  fTracks->SetBranchAddress("fTPCNClsFindable", &mytracks.fTPCNClsFindable);
  fTracks->SetBranchAddress("fTPCNClsFindableMinusFound",&mytracks.fTPCNClsFindableMinusFound);
  fTracks->SetBranchAddress("fTPCNClsFindableMinusCrossedRows", &mytracks.fTPCNClsFindableMinusCrossedRows);
  fTracks->SetBranchAddress("fTPCNClsShared", &mytracks.fTPCNClsShared);
  fTracks->SetBranchAddress("fTRDPattern", &mytracks.fTRDPattern);
  fTracks->SetBranchAddress("fITSChi2NCl", &mytracks.fITSChi2NCl);
  fTracks->SetBranchAddress("fTPCChi2NCl", &mytracks.fTPCChi2NCl);
  fTracks->SetBranchAddress("fTRDChi2", &mytracks.fTRDChi2);
  fTracks->SetBranchAddress("fTOFChi2", &mytracks.fTOFChi2);
  fTracks->SetBranchAddress("fTPCSignal", &mytracks.fTPCSignal);
  fTracks->SetBranchAddress("fTRDSignal", &mytracks.fTRDSignal);
  fTracks->SetBranchAddress("fTOFSignal", &mytracks.fTOFSignal);
  fTracks->SetBranchAddress("fLength", &mytracks.fLength);
  fTracks->SetBranchAddress("fTOFExpMom", &mytracks.fTOFExpMom);
}

struct {
  // MC particle

  Int_t   fMcCollisionsID = -1;    /// The index of the MC collision vertex

  // MC information (modified version of TParticle
  Int_t fPdgCode    = -99999; /// PDG code of the particle
  Int_t fStatusCode = -99999; /// generation status code
  uint8_t fFlags    = 0;     /// See enum MCParticleFlags
  Int_t fMother0    = 0; /// Indices of the mother particles
  Int_t fMother1    = 0;
  Int_t fDaughter0  = 0; /// Indices of the daughter particles
  Int_t fDaughter1  = 0;
  Float_t fWeight   = 1;     /// particle weight from the generator or ML

  Float_t fPx = -999.f; /// x component of momentum
  Float_t fPy = -999.f; /// y component of momentum
  Float_t fPz = -999.f; /// z component of momentum
  Float_t fE  = -999.f; /// Energy (covers the case of resonances, no need for calculated mass)

  Float_t fVx = -999.f; /// x of production vertex
  Float_t fVy = -999.f; /// y of production vertex
  Float_t fVz = -999.f; /// z of production vertex
  Float_t fVt = -999.f; /// t of production vertex
  // We do not use the polarisation so far
} mcparticle;  //! MC particles from the kinematics tree

TTree* MakeTreeO2mcparticle()
{
  TTree* tKinematics = new TTree("O2mcparticle", "Kinematics");
  tKinematics->Branch("fMcCollisionsID", &mcparticle.fMcCollisionsID, "fMcCollisionsID/I");
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
  return tKinematics;
}

struct {
  // Track label to find the corresponding MC particle
  UInt_t fLabel = 0;       /// Track label
  UShort_t fLabelMask = 0; /// Bit mask to indicate detector mismatches (bit ON means mismatch)
                           /// Bit 0-6: mismatch at ITS layer
                           /// Bit 7-9: # of TPC mismatches in the ranges 0, 1, 2-3, 4-7, 8-15, 16-31, 32-63, >64
                           /// Bit 10: TRD, bit 11: TOF, bit 15: negative label sign
} mctracklabel; //! Track labels

TTree *MakeTreeO2mctracklabel()
{
  TTree* tLabels = new TTree("O2mctracklabel", "MC track labels");
  tLabels->Branch("fLabel", &mctracklabel.fLabel, "fLabel/i");
  tLabels->Branch("fLabelMask", &mctracklabel.fLabelMask, "fLabelMask/s");
  return tLabels;
}
struct {
  // MC collision label
  UInt_t fLabel = 0;       /// Collision label
  UShort_t fLabelMask = 0; /// Bit mask to indicate collision mismatches (bit ON means mismatch)
                             /// bit 15: negative label sign
} mccollisionlabel; //! Collision labels

TTree* MakeTreeO2mccollisionlabel()
{
  TTree* tCollisionLabels = new TTree("O2mccollisionlabel", "MC collision labels");
  tCollisionLabels->Branch("fLabel", &mccollisionlabel.fLabel, "fLabel/i");
  tCollisionLabels->Branch("fLabelMask", &mccollisionlabel.fLabelMask, "fLabelMask/s");
  return tCollisionLabels;
}
