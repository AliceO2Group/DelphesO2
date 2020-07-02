R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

bool smear = true;
bool nsigma = true;
double Bz = 0.2;

void
createO2tables(const char *inputFile = "delphes.root",
    const char *outputFile = "AODRun5.root")
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

    // Covariance matrix
    Float_t fCYY = -999.f;       /// fC[0]
    Float_t fCZY = -999.f;       /// fC[1]
    Float_t fCZZ = -999.f;       /// fC[2]
    Float_t fCSnpY = -999.f;     /// fC[3]
    Float_t fCSnpZ = -999.f;     /// fC[4]
    Float_t fCSnpSnp = -999.f;   /// fC[5]
    Float_t fCTglY = -999.f;     /// fC[6]
    Float_t fCTglZ = -999.f;     /// fC[7]
    Float_t fCTglSnp = -999.f;   /// fC[8]
    Float_t fCTglTgl = -999.f;   /// fC[9]
    Float_t fC1PtY = -999.f;     /// fC[10]
    Float_t fC1PtZ = -999.f;     /// fC[11]
    Float_t fC1PtSnp = -999.f;   /// fC[12]
    Float_t fC1PtTgl = -999.f;   /// fC[13]
    Float_t fC1Pt21Pt2 = -999.f; /// fC[14]

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
    UChar_t fTRDTOFPattern = 0u;   /// Bit 0-5 if TRD layers used. Bit 6 for TOF

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
  } mytracks;                      //! structure to keep track information

  struct {
    Int_t fBCsID = 0u;       /// Index to BC table
    Float_t  fPosX = -999.f;       /// Primary vertex x coordinate
    Float_t  fPosY = -999.f;       /// Primary vertex y coordinate
    Float_t  fPosZ = -999.f;       /// Primary vertex z coordinate
    Float_t  fCovXX = 999.f;    /// cov[0]
    Float_t  fCovXY = 0.f;      /// cov[1]
    Float_t  fCovXZ = 0.f;      /// cov[2]
    Float_t  fCovYY = 999.f;    /// cov[3]
    Float_t  fCovYZ = 0.f;      /// cov[4]
    Float_t  fCovZZ = 999.f;    /// cov[5]
    Float_t  fChi2 = 999.f;             /// Chi2 of the vertex
    UInt_t   fN = 0u;                /// Number of contributors
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
  fTracks->Branch("fCYY", &mytracks.fCYY, "fCYY/F");
  fTracks->Branch("fCZY", &mytracks.fCZY, "fCZY/F");
  fTracks->Branch("fCZZ", &mytracks.fCZZ, "fCZZ/F");
  fTracks->Branch("fCSnpY", &mytracks.fCSnpY, "fCSnpY/F");
  fTracks->Branch("fCSnpZ", &mytracks.fCSnpZ, "fCSnpZ/F");
  fTracks->Branch("fCSnpSnp", &mytracks.fCSnpSnp, "fCSnpSnp/F");
  fTracks->Branch("fCTglY", &mytracks.fCTglY, "fCTglY/F");
  fTracks->Branch("fCTglZ", &mytracks.fCTglZ, "fCTglZ/F");
  fTracks->Branch("fCTglSnp", &mytracks.fCTglSnp, "fCTglSnp/F");
  fTracks->Branch("fCTglTgl", &mytracks.fCTglTgl, "fCTglTgl/F");
  fTracks->Branch("fC1PtY", &mytracks.fC1PtY, "fC1PtY/F");
  fTracks->Branch("fC1PtZ", &mytracks.fC1PtZ, "fC1PtZ/F");
  fTracks->Branch("fC1PtSnp", &mytracks.fC1PtSnp, "fC1PtSnp/F");
  fTracks->Branch("fC1PtTgl", &mytracks.fC1PtTgl, "fC1PtTgl/F");
  fTracks->Branch("fC1Pt21Pt2", &mytracks.fC1Pt21Pt2, "fC1Pt21Pt2/F");
  fTracks->Branch("fTPCInnerParam", &mytracks.fTPCinnerP, "fTPCInnerParam/F");
  fTracks->Branch("fFlags", &mytracks.fFlags, "fFlags/l");
  fTracks->Branch("fITSClusterMap", &mytracks.fITSClusterMap, "fITSClusterMap/b");
  fTracks->Branch("fTPCNClsFindable", &mytracks.fTPCNClsFindable, "fTPCNClsFindable/b");
  fTracks->Branch("fTPCNClsFindableMinusFound",&mytracks.fTPCNClsFindableMinusFound, "fTPCNClsFindableMinusFound/B");
  fTracks->Branch("fTPCNClsFindableMinusCrossedRows", &mytracks.fTPCNClsFindableMinusCrossedRows, "fTPCNClsFindableMinusCrossedRows/B");
  fTracks->Branch("fTPCNClsShared", &mytracks.fTPCNClsShared, "fTPCNClsShared/b");
  fTracks->Branch("fTRDTOFPattern", &mytracks.fTRDTOFPattern, "fTRDTOFPattern/b");
  fTracks->Branch("fITSChi2NCl", &mytracks.fITSChi2NCl, "fITSChi2NCl/F");
  fTracks->Branch("fTPCChi2NCl", &mytracks.fTPCChi2NCl, "fTPCChi2NCl/F");
  fTracks->Branch("fTRDChi2", &mytracks.fTRDChi2, "fTRDChi2/F");
  fTracks->Branch("fTOFChi2", &mytracks.fTOFChi2, "fTOFChi2/F");
  fTracks->Branch("fTPCSignal", &mytracks.fTPCSignal, "fTPCSignal/F");
  fTracks->Branch("fTRDSignal", &mytracks.fTRDSignal, "fTRDSignal/F");
  fTracks->Branch("fTOFSignal", &mytracks.fTOFSignal, "fTOFSignal/F");
  fTracks->Branch("fLength", &mytracks.fLength, "fLength/F");

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

  for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry) {
    
    // Load selected branches with data from specified event
    treeReader->ReadEntry(ientry);
    collision.fBCsID = ientry;
    bc.fGlobalBC = ientry;
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
    tEvents->Fill();
    tBC->Fill();
    // loop over tracks
    for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack) {

      // get track and corresponding particle
      auto track = (Track *)tracks->At(itrack);
      auto particle = (GenParticle *)track->Particle.GetObject();
      // smear track if requested
      //if (smear)
      //	if (!smearer.smearTrack(*track)) continue;
      
      O2Track o2track; // tracks in internal O2 format
      o2::delphes::TrackUtils::convertTrackToO2Track(*track, o2track, true);
      smearer.smearTrack(o2track, track->PID);     
      mytracks.fCollisionsID = ientry;
      mytracks.fX = o2track.getX();
      mytracks.fAlpha = o2track.getAlpha();
      mytracks.fY = o2track.getY();
      mytracks.fZ = o2track.getZ();
      mytracks.fSnp = o2track.getSnp();
      mytracks.fTgl = o2track.getTgl();
      mytracks.fSigned1Pt = o2track.getQ2Pt(); //is this correct

      mytracks.fCYY= o2track.getSigmaY2();
      mytracks.fCZY = o2track.getSigmaZY();
      mytracks.fCZZ = o2track.getSigmaZ2();
      mytracks.fCSnpY = o2track.getSigmaSnpY();
      mytracks.fCSnpZ = o2track.getSigmaSnpZ();
      mytracks.fCSnpSnp = o2track.getSigmaSnp2();
      mytracks.fCTglY = o2track.getSigmaTglY();
      mytracks.fCTglZ = o2track.getSigmaTglZ();
      mytracks.fCTglSnp = o2track.getSigmaTglSnp();
      mytracks.fCTglTgl = o2track.getSigmaTgl2();
      mytracks.fC1PtY = o2track.getSigma1PtY();
      mytracks.fC1PtZ = o2track.getSigma1PtZ();
      mytracks.fC1PtSnp = o2track.getSigma1PtSnp();
      mytracks.fC1PtTgl = o2track.getSigma1PtTgl();
      mytracks.fC1Pt21Pt2 = o2track.getSigma1Pt2();
      mytracks.fITSClusterMap = 3;
      mytracks.fFlags = 4;


      fTracks->Fill();
      // fill histograms
    }
  }
  fTracks->Write();
  tEvents->Write();
  tBC->Write();
  fout->Close();
  
}
