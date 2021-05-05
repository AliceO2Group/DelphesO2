/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

/// @author: Antonio Uras
/// @email: antonio.uras@cern.ch

#include "MIDdetector.hh"
#include "TDatabasePDG.h"
#include "THnSparse.h"
#include "TRandom.h"
#include "TDatime.h"
#include "TFile.h"
#include "TVector3.h"

namespace o2 {
  namespace delphes {
    
    //==========================================================================================================
    
    bool MIDdetector::setup(const Char_t *nameInputFile = "muonAccEffPID.root") {

      TDatime t;
      gRandom->SetSeed(t.GetDate()+t.GetYear()*t.GetHour()*t.GetMinute()*t.GetSecond());
      
      mFileAccEffMuonPID = new TFile(nameInputFile);
      if (!mFileAccEffMuonPID) {
	printf("File %s not found\n",nameInputFile);
	return kFALSE;
      }
      if (!(mFileAccEffMuonPID->IsOpen())) {
	printf("File %s not open\n",nameInputFile);
	return kFALSE;
      }

      for (Int_t iPart=kMuon; iPart<kNPart; iPart++) {
	mAccEffMuonPID[iPart] = (THnSparse*) mFileAccEffMuonPID->Get(Form("mAccEffMuonPID_%s",partLabel[iPart]));
	if (!mAccEffMuonPID[iPart]) {
	  printf("Object %s not found, quitting\n",Form("mAccEffMuonPID_%s",partLabel[iPart]));
	  return kFALSE;
	}
      }

      printf("Setup of MIDdetector successfully completed\n");
      return kTRUE;
	
    }

    //==========================================================================================================

    bool MIDdetector::hasMID(const Track &track) {

      TVector3 v(track.XOuter, track.YOuter, track.ZOuter);
      return (TMath::Abs(v.Eta()) < mEtaMax);

    }

    //==========================================================================================================

    bool MIDdetector::isMuon(const Track &track, int multiplicity=1) {

      auto pdg  = std::abs(track.PID);
      auto part = pidmap[pdg];
      if (part == kElectron) return kFALSE;

      auto particle = (GenParticle*) track.Particle.GetObject();

      Double_t var[4] = {track.P, track.Eta, particle->Z, double(multiplicity)};
      Double_t probMuonPID = mAccEffMuonPID[part]->GetBinContent(mAccEffMuonPID[part]->GetBin(var));
      return (gRandom->Uniform() < probMuonPID);

    }

    //==========================================================================================================
  
  
  } /** namespace delphes **/

} /** namespace o2 **/
