/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_TrackUtils_h_
#define _DelphesO2_TrackUtils_h_

#include "ReconstructionDataFormats/Track.h"
#include "classes/DelphesClasses.h"

using O2Track = o2::track::TrackParCov;
  
namespace o2
{
namespace delphes
{

class TrackUtils {
  
public:

  static void makeO2Track(O2Track &o2track, std::array<float, 3> xyz, std::array<float, 3> ptetaphi, int charge);
  static void convertO2TrackToTrack(const O2Track &o2track, Track &track, bool atDCA);

  static void convertTrackToO2Track(const Track &track, O2Track &o2track, bool atDCA);
  static void convertGenParticleToO2Track(const GenParticle &particle, O2Track &o2track);

  static bool propagateToDCA(O2Track &o2track, std::array<float, 3> xyz, float Bz);
  
protected:
  
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_TrackUtils_h_ **/

