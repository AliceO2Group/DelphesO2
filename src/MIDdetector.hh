/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

/// @author: Antonio Uras
/// @email: antonio.uras@cern.ch

#ifndef _DelphesO2_MIDdetector_h_
#define _DelphesO2_MIDdetector_h_

#include "classes/DelphesClasses.h"
#include "THnSparse.h"
#include "TFile.h"

#include <map>
using namespace std;

namespace o2 {
  namespace delphes {

    class MIDdetector {
  
    public:
      MIDdetector() = default;
      ~MIDdetector() = default;
  
      enum { kElectron, kMuon, kPion, kKaon, kProton, kNPart }; // primary particles with a non-zero muon PID probability
      
      void setup(const Char_t *nameInputFile);
      bool hasMID(const Track &track);
      bool isMuon(const Track &track, int multiplicity);

    protected:

      TFile *mFileAccEffMuonPID;
      THnSparse *mAccEffMuonPID[kNPart];
      const double mEtaMax = 1.6;
      const char *partLabel[kNPart] = {"electron","muon","pion","kaon","proton"};
      std::map<int, int> pidmap = { {11, kElectron}, {13, kMuon}, {211, kPion}, {321, kKaon}, {2212, kProton} };
  
    };
  
  } /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_MIDLayer_h_ **/

