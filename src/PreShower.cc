/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

/// @author: Antonio Uras
/// @email: antonio.uras@cern.ch

/// @author: Marco van Leeuwen
/// @email: marco.van.leeuwen@cern.ch

#include "PreShower.hh"
#include "TDatabasePDG.h"
#include "THnSparse.h"
#include "TRandom.h"
#include "TDatime.h"
#include "TFile.h"
#include "TVector3.h"
#include "TMath.h"
#include "TAxis.h"

namespace o2 {
  namespace delphes {
    
    //==========================================================================================================
    
    bool PreShower::setup() {

      TDatime t;
      gRandom->SetSeed(t.GetDate()+t.GetYear()*t.GetHour()*t.GetMinute()*t.GetSecond()); // NB: gRandom is a global pointer ?
      for (Int_t iPart = 0; iPart < kNPart; iPart++) {
	      mMomMin[iPart] = 0.1;
	      mMomMax[iPart] = 20;
      }

      return kTRUE;
	
    }

    //==========================================================================================================

    bool PreShower::hasPreShower(const Track &track) {

      auto pdg  = std::abs(track.PID);
      auto part = pidmap[pdg];
      return ((TMath::Abs(track.Eta) < mEtaMax) && (track.P > mMomMin[part]));

    }

    //==========================================================================================================

    bool PreShower::isElectron(const Track &track, int multiplicity=1) {

      auto pdg  = std::abs(track.PID);
      auto part = pidmap[pdg];
      if (part == kElectron) {
			// Parametrisation of preshower detector studies without charge sharing
         float eff = 0.8*(1.-exp(-1.6*(track.P-0.05)));
         return (gRandom->Uniform() < eff);
      }
      else {
         const Float_t misTagProb = 0.001;
         return (gRandom->Uniform() < misTagProb);
      }
    }

    //==========================================================================================================
   
  } /** namespace delphes **/

} /** namespace o2 **/
