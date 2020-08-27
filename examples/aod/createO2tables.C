R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

double Bz = 0.2;
double tof_radius = 100.; // [cm]
double tof_length = 200.; // [cm]
double tof_sigmat = 0.02; // [ns]

void
createO2tables(const char *inputFile = "delphes.root",
	       const char *outputFile = "AODRun5.root",
	       int eventOffset = 0)
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
  
  struct {
    int fRunNumber = -1;         /// Run number
    ULong64_t fGlobalBC = 0u;    /// Unique bunch crossing id. Contains period, orbit and bunch crossing numbers
    ULong64_t fTriggerMask = 0u; /// Trigger class mask
  } bc; //! structure to keep trigger-related info
 
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
  

  auto fout = TFile::Open(outputFile, "RECREATE");
   
  TTree* tBC = new TTree("O2bc", "BC info"); 
  tBC->Branch("fRunNumber", &bc.fRunNumber, "fRunNumber/I");
  tBC->Branch("fGlobalBC", &bc.fGlobalBC, "fGlobalBC/l");
  tBC->Branch("fTriggerMask", &bc.fTriggerMask, "fTriggerMask/l");

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

  struct {
    // Track label to find the corresponding MC particle
    UInt_t fLabel = 0;       /// Track label
    UShort_t fLabelMask = 0; /// Bit mask to indicate detector mismatches (bit ON means mismatch)
                           /// Bit 0-6: mismatch at ITS layer
                           /// Bit 7-9: # of TPC mismatches in the ranges 0, 1, 2-3, 4-7, 8-15, 16-31, 32-63, >64
                           /// Bit 10: TRD, bit 11: TOF, bit 15: negative label sign
  } mctracklabel; //! Track labels
  
  TTree* tLabels = new TTree("O2mctracklabel", "MC track labels");
  tLabels->Branch("fLabel", &mctracklabel.fLabel, "fLabel/i");
  tLabels->Branch("fLabelMask", &mctracklabel.fLabelMask, "fLabelMask/s");

  UInt_t mTrackX =  0xFFFFFFFF;
  UInt_t mTrackAlpha = 0xFFFFFFFF;
  UInt_t mtrackSnp = 0xFFFFFFFF;
  UInt_t mTrackTgl = 0xFFFFFFFF;
  UInt_t mTrack1Pt = 0xFFFFFFFF; // Including the momentun at the inner wall of TPC
  UInt_t mTrackCovDiag = 0xFFFFFFFF; // Including the chi2
  UInt_t mTrackCovOffDiag = 0xFFFFFFFF;
  UInt_t mTrackSignal = 0xFFFFFFFF; // PID signals and track length
  
  int fOffsetLabel = 0;
  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);

    // loop over particles
    for (Int_t iparticle = 0; iparticle < particles->GetEntries(); ++iparticle) {
      auto particle = (GenParticle *)particles->At(iparticle);
      
      particle->SetUniqueID(iparticle + fOffsetLabel); // not sure this is needed, to be sure
      
      mcparticle.fMcCollisionsID = ientry + eventOffset;
      mcparticle.fPdgCode = particle->PID;
      mcparticle.fStatusCode = particle->Status;
      mcparticle.fFlags = 0;
      mcparticle.fMother0 = particle->M1;
      if (mcparticle.fMother0 > -1) mcparticle.fMother0 += fOffsetLabel;
      mcparticle.fMother1 = particle->M2;
      if (mcparticle.fMother1 > -1) mcparticle.fMother1 += fOffsetLabel;
      mcparticle.fDaughter0 = particle->D1;
      if (mcparticle.fDaughter0 > -1) mcparticle.fDaughter0 += fOffsetLabel;
      mcparticle.fDaughter1 = particle->D2;
      if (mcparticle.fDaughter1 > -1) mcparticle.fDaughter1 += fOffsetLabel;
      mcparticle.fWeight = 1.;

      mcparticle.fPx = particle->Px;
      mcparticle.fPy = particle->Py;
      mcparticle.fPz = particle->Pz;
      mcparticle.fE  = particle->E;

      mcparticle.fVx = particle->X;
      mcparticle.fVy = particle->Y;
      mcparticle.fVz = particle->Z;
      mcparticle.fVt = particle->T;
      
      tKinematics->Fill();

    }
    fOffsetLabel += particles->GetEntries();
    
    // loop over tracks
    std::vector<Track *> tof_tracks;
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();

      // fill the label tree
      Int_t alabel = particle->GetUniqueID();
      mctracklabel.fLabel = TMath::Abs(alabel);
      mctracklabel.fLabelMask = 0;
      tLabels->Fill();
      
      O2Track o2track; // tracks in internal O2 format
      o2::delphes::TrackUtils::convertTrackToO2Track(*track, o2track, true);
      smearer.smearTrack(o2track, track->PID);     
      o2::delphes::TrackUtils::convertO2TrackToTrack(o2track, *track, true);
      
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

      mytracks.fRhoZY = (Char_t)(128.*o2track.getSigmaZY()/mytracks.fSigmaZ/mytracks.fSigmaY);
      mytracks.fRhoSnpY = (Char_t)(128.*o2track.getSigmaSnpY()/mytracks.fSigmaSnp/mytracks.fSigmaY);
      mytracks.fRhoSnpZ = (Char_t)(128.*o2track.getSigmaSnpZ()/mytracks.fSigmaSnp/mytracks.fSigmaZ);
      mytracks.fRhoTglY = (Char_t)(128.*o2track.getSigmaTglY()/mytracks.fSigmaTgl/mytracks.fSigmaY);
      mytracks.fRhoTglZ = (Char_t)(128.*o2track.getSigmaTglZ()/mytracks.fSigmaTgl/mytracks.fSigmaZ);
      mytracks.fRhoTglSnp = (Char_t)(128.*o2track.getSigmaTglSnp()/mytracks.fSigmaTgl/mytracks.fSigmaSnp);
      mytracks.fRho1PtY = (Char_t)(128.*o2track.getSigma1PtY()/mytracks.fSigma1Pt/mytracks.fSigmaY);
      mytracks.fRho1PtZ = (Char_t)(128.*o2track.getSigma1PtZ()/mytracks.fSigma1Pt/mytracks.fSigmaZ);
      mytracks.fRho1PtSnp = (Char_t)(128.*o2track.getSigma1PtSnp()/mytracks.fSigma1Pt/mytracks.fSigmaSnp);
      mytracks.fRho1PtTgl = (Char_t)(128.*o2track.getSigma1PtTgl()/mytracks.fSigma1Pt/mytracks.fSigmaTgl);

      //FIXME this needs to be fixed
      mytracks.fITSClusterMap = 3;
      mytracks.fFlags = 4;

      // check if has hit the TOF
      if (toflayer.hasTOF(*track)) {
	tof_tracks.push_back(track);
	mytracks.fLength = track->L * 0.1; // [cm]
	mytracks.fTOFSignal = track->TOuter * 1.e12; // [ps]
	mytracks.fTOFExpMom = track->P * 0.029979246;
      }
      else {
	mytracks.fLength = -999.f;
	mytracks.fTOFSignal = -999.f;
	mytracks.fTOFExpMom = -999.f;
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
    collision.fPosX = 0.;
    collision.fPosY = 0.;
    collision.fPosZ = 0.;
    collision.fCovXX = 0.01;
    collision.fCovXY = 0.01;
    collision.fCovXZ = 0.01;
    collision.fCovYY = 0.01;
    collision.fCovYZ = 0.01;
    collision.fCovZZ = 0.01;
    collision.fChi2 = 0.01;
    collision.fCollisionTime = tzero[0] * 1.e3; // [ps]
    collision.fCollisionTimeRes = tzero[1] * 1.e3; // [ps]
    tEvents->Fill();
    tBC->Fill();
    
  }
  
  fTracks->Write();
  tEvents->Write();
  tBC->Write();
  tKinematics->Write();
  tLabels->Write();
  fout->Close();
  
}
