/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_TrackSmearer_h_
#define _DelphesO2_TrackSmearer_h_

#include "ReconstructionDataFormats/Track.h"
#include "classes/DelphesClasses.h"
#include "lutCovm.hh"

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
  void loadTable(const char *filename);
  lutEntry_t &getLUTEntry(float nch, float radius, float eta, float pt);

  void smearTrack(O2Track &o2track, lutEntry_t &lutEntry);
  void smearTrack(Track &track, bool atDCA = true);
  
protected:

  lutHeader_t mLUTHeader;
  lutEntry_t mLUTEntry[1][1][100][100];
  
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_TrackSmearer_h_ **/

