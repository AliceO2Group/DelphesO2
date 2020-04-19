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
  double x, y, z;
};
  
class VertexFitter {
  
public:
  VertexFitter() = default;
  ~VertexFitter() = default;

  void setup(float bz, bool useAbsDCA = true, bool propagateToVtx = true);
  bool fitVertex(Track &track1, Track &track2, Vertex &vertex);

protected:

  o2::vertexing::DCAFitterN<2> mFitter;
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_VertexFitter_h_ **/

