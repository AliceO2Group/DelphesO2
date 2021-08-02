/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

/// @author: Antonio Uras
/// @email: antonio.uras@cern.ch

/// @author: Marco van Leeuwen
/// @email: marco.van.leeuwen@cern.ch

#ifndef _DelphesO2_PreShower_h_
#define _DelphesO2_PreShower_h_

#include "classes/DelphesClasses.h"
#include "THnSparse.h"
#include "TFile.h"

#include <map>
using namespace std;

namespace o2 {
  namespace delphes {

    class PreShower {
  
    public:
      PreShower() = default;
      ~PreShower() = default;
  
      enum { kElectron, kMuon, kPion, kKaon, kProton, kNPart }; // primary particles with a non-zero muon PID probability
      
      bool setup();
      bool hasPreShower(const Track &track);
      bool isElectron(const Track &track, int multiplicity);

    protected:

      const double mEtaMax = 1.75;
      double mMomMin[kNPart];
      double mMomMax[kNPart];
      const char *partLabel[kNPart] = {"electron","muon","pion","kaon","proton"};
      std::map<int, int> pidmap = { {11, kElectron}, {13, kMuon}, {211, kPion}, {321, kKaon}, {2212, kProton} };
  
    };
  
  } /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_MIDLayer_h_ **/

