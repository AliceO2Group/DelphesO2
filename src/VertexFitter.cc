/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "VertexFitter.hh"

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
}

/*****************************************************************/

bool
VertexFitter::convertO2TrackToTrack(const O2Track& o2track, Track& track)
{
  track.PT = o2track.getPt();
  track.Eta = o2track.getEta();
  track.Phi = o2track.getPhi();
  //
  auto xyz = o2track.getXYZGlo();
  track.X = 10. * xyz.X();
  track.Y = 10. * xyz.Y();
  track.Z = 10. * xyz.Z(); 

  track.Charge = TMath::Nint( o2track.getSign() );
  return true;
}

/*****************************************************************/

bool
VertexFitter::convertTrackToO2Track(const Track& track, O2Track& o2track)
{
  const float errYZ = 1e-2, errSlp = 1e-3, errQPT = 2e-2;
  std::array<float, 15> covm = {
    errYZ * errYZ,
    0., errYZ * errYZ,
    0, 0., errSlp * errSlp,
    0., 0., 0., errSlp * errSlp,
    0., 0., 0., 0., errQPT * errQPT};
  //
  float s, c, x;
  std::array<float, 5> params;
  o2::utils::sincosf(track.Phi, s, c);
  o2::utils::rotateZInv(0.1 * track.Xd, 0.1 * track.Yd, x, params[0], s, c);
  params[1] = 0.1 * track.Zd;
  params[2] = 0.; // since alpha = phi
  auto theta = 2.*TMath::ATan( TMath::Exp(-track.Eta) );  
  params[3] = 1. / TMath::Tan(theta);
  params[4] = track.Charge / track.PT;
  
  covm[14] = errQPT * errQPT * params[4] * params[4];
  
  new (&o2track) O2Track(x, track.Phi, params, covm);

  return true;
}

/*****************************************************************/

bool
VertexFitter::fitVertex(Track& track1, Track& track2, Vertex& vertex)
{
  O2Track o2track1, o2track2; // tracks in internal O2 format
  convertTrackToO2Track(track1, o2track1);
  convertTrackToO2Track(track2, o2track2);
  int nv = mFitter.process(o2track1, o2track2);
  if (!nv) return false; // teoretically, one can find up to 2 vertices
  
  convertO2TrackToTrack(mFitter.getTrack(0), track1); // convert back tracks propagated
  convertO2TrackToTrack(mFitter.getTrack(1), track2); // to vertex
  const auto& fittedVertex = mFitter.getPCACandidate(0);
  vertex.x = 10. * fittedVertex[0];
  vertex.y = 10. * fittedVertex[1];
  vertex.z = 10. * fittedVertex[2];
  return true;
}

/*****************************************************************/

}
}
