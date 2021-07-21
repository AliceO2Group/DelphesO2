/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_TrackSmearer_h_
#define _DelphesO2_TrackSmearer_h_

#include "ReconstructionDataFormats/Track.h"
#include "classes/DelphesClasses.h"
#include "lutCovm.hh"
#include <map>

using O2Track = o2::track::TrackParCov;

namespace o2
{
namespace delphes
{

class TrackSmearer {
  
public:
  TrackSmearer() = default;
  ~TrackSmearer() = default;

  /** LUT methods **/
  bool loadTable(int pdg, const char *filename);
  void useEfficiency(bool val) { mUseEfficiency = val; };
  lutHeader_t *getLUTHeader(int pdg) { return mLUTHeader[getIndexPDG(pdg)]; };
  lutEntry_t *getLUTEntry(int pdg, float nch, float radius, float eta, float pt);

  bool smearTrack(O2Track &o2track, lutEntry_t *lutEntry);
  bool smearTrack(O2Track &o2track, int pid, float nch);
  bool smearTrack(Track &track, bool atDCA = true);

  int getIndexPDG(int pdg) {
    switch(abs(pdg)) {
    case 11: return 0; // Electron
    case 13: return 1; // Muon
    case 211: return 2; // Pion
    case 321: return 3; // Kaon
    case 2212: return 4; // Proton
    case 1000010020: return 5; // Deuteron
    case 1000020030: return 6; // Helium3
    default: return 2; // Default: pion
    };
  };

  void setdNdEta(float val) { mdNdEta = val; };
  
protected:
  static constexpr unsigned int nLUTs = 7;
  lutHeader_t *mLUTHeader[nLUTs] = {nullptr};
  lutEntry_t *****mLUTEntry[nLUTs] = {nullptr};
  bool mUseEfficiency = true;
  float mdNdEta =  1600.;
  
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_TrackSmearer_h_ **/

