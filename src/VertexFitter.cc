/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "VertexFitter.hh"
#include "TrackUtils.hh"

namespace o2
{
namespace delphes
{

/*****************************************************************/

void
VertexFitter::setup(float bz, bool useAbsDCA, bool propagateToVtx)
{
  mFitter.setBz(bz * 10.); // [kG]
  mFitter.setUseAbsDCA(useAbsDCA); // to use abs distance minimization
  mFitter.setPropagateToPCA(propagateToVtx); // to propagate tracks to closest distance to the found vertex
  mFitter.setMaxDistance2ToMerge(0.01);
}

/*****************************************************************/

bool
VertexFitter::fitVertex(O2Track& o2track1, O2Track& o2track2, Vertex& vertex)
{
  int nv = mFitter.process(o2track1, o2track2);
  if (!nv) return false; // teoretically, one can find up to 2 vertices
  const auto& fittedVertex = mFitter.getPCACandidate(0);
  vertex.x = fittedVertex[0];
  vertex.y = fittedVertex[1];
  vertex.z = fittedVertex[2];
  return true;
}

/*****************************************************************/

bool
VertexFitter::fitVertex(Track& track1, Track& track2, Vertex& vertex)
{

  O2Track o2track1, o2track2; // tracks in internal O2 format
  TrackUtils::convertTrackToO2Track(track1, o2track1, true);
  TrackUtils::convertTrackToO2Track(track2, o2track2, true);
  if (!fitVertex(o2track1, o2track2, vertex)) return false;
  TrackUtils::convertO2TrackToTrack(mFitter.getTrack(0), track1, false); // convert back tracks propagated
  TrackUtils::convertO2TrackToTrack(mFitter.getTrack(1), track2, false); // to vertex
  vertex.x *= 10.;
  vertex.y *= 10.;
  vertex.z *= 10.;
  return true;

#if 0
  O2Track o2track1, o2track2; // tracks in internal O2 format
  TrackUtils::convertTrackToO2Track(track1, o2track1, true);
  TrackUtils::convertTrackToO2Track(track2, o2track2, true);
  int nv = mFitter.process(o2track1, o2track2);
  if (!nv) return false; // teoretically, one can find up to 2 vertices
  
  TrackUtils::convertO2TrackToTrack(mFitter.getTrack(0), track1, false); // convert back tracks propagated
  TrackUtils::convertO2TrackToTrack(mFitter.getTrack(1), track2, false); // to vertex
  const auto& fittedVertex = mFitter.getPCACandidate(0);
  vertex.x = 10. * fittedVertex[0];
  vertex.y = 10. * fittedVertex[1];
  vertex.z = 10. * fittedVertex[2];
  return true;
#endif
}

/*****************************************************************/
  
} /** namespace delphes **/
} /** namespace o2 **/

