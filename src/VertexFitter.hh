/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_VertexFitter_h_
#define _DelphesO2_VertexFitter_h_

#include "DetectorsVertexing/DCAFitterN.h"
#include "classes/DelphesClasses.h"

using O2Track = o2::track::TrackParCov;
  
namespace o2
{
namespace delphes
{

struct Vertex
{
  float x, y, z;
};
  
class VertexFitter {
  
public:
  VertexFitter() = default;
  ~VertexFitter() = default;

  void setup(float bz, bool useAbsDCA = true, bool propagateToVtx = true);
  bool fitVertex(Track &track1, Track &track2, Vertex &vertex);

protected:

  bool convertO2TrackToTrack(const O2Track &o2track, Track &track);
  bool convertTrackToO2Track(const Track &track, O2Track &o2track);

  o2::vertexing::DCAFitterN<2> mFitter;
};
  
}
}

#endif /** _DelphesO2_VertexFitter_h_ **/

