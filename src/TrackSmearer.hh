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
  lutEntry_t *getLUTEntry(int pdg, float nch, float radius, float eta, float pt);

  void smearTrack(O2Track &o2track, lutEntry_t *lutEntry);
  bool smearTrack(O2Track &o2track, int pid);
  bool smearTrack(Track &track, bool atDCA = true);

  int getIndexPDG(int pdg) {
    switch(abs(pdg)) {
    case 11: return 0;
    case 13: return 1;
    case 211: return 2;
    case 321: return 3;
    case 2212: return 4;
    default: return 2;
    };
  };
  
protected:

  lutHeader_t *mLUTHeader[5] = {nullptr};
  lutEntry_t *****mLUTEntry[5] = {nullptr};
  
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_TrackSmearer_h_ **/

