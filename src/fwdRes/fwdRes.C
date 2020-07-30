#include "AliExternalTrackParam.h"
#include "TMath.h"
#include "TH2F.h"

const int NPlanes = 9;
float ZPlane[NPlanes] = {20.,   40.,  60.,  80., 100., 120., 140., 160., 180.};
float X2X0[NPlanes]   = {0.005, 0.005, 0.005, 0.005, 0.005, 0.005, 0.005, 0.005, 0.005};
float Res[NPlanes]    = {5e-4, 5e-4, 5e-4, 5e-4, 5e-4, 5e-4, 5e-4, 5e-4, 5e-4};
float Bz = 5.;

bool propagateToZ(AliExternalTrackParam& tr, float z, float bz);
float fwdRes(float *covm, float pt, float eta, float mass=0.14);

TH2F* hptres = 0;

void fwdRes() {
  float ptMin=0.2, ptMax = 10.;
  float etaMin=1., etaMax = 4.;
  int nPtBin = 50, nEtaBin = 50;
  hptres = new TH2F("etapt","#sigma_{1/p_{T}} #times p_{T}", nEtaBin, etaMin, etaMax, nPtBin, ptMin, ptMax);
  hptres->SetXTitle("#eta");
  hptres->SetYTitle("p_{T}");
  hptres->SetZTitle("#sigma_{1/p_{T}} #times p_{T}");
  for (int ieta=0;ieta<nEtaBin;ieta++) {
    float eta = hptres->GetXaxis()->GetBinCenter(ieta+1);
    for (int ipt=0;ipt<nPtBin;ipt++) {
      float pt = hptres->GetYaxis()->GetBinCenter(ipt+1);
      float ptres = fwdRes(nullptr, pt, eta);
      if (ptres>0) {
        hptres->SetBinContent(ieta+1, ipt+1, ptres);
      }
    }
  }
}

float fwdRes(float *covm, float pt, float eta, float mass)
{
  if (TMath::Abs(eta)<1e-6 || pt<1e-3) {
    return -1;
  }
  float tgl = 1./TMath::Tan(2*TMath::ATan(TMath::Exp(-eta)));
  double par[5] = {0.,0.,0.,tgl,1./pt};
  double cov[15] = {
    0.01,
    0.0, 0.01,
    0.0, 0.0, 0.01,
    0.0, 0.0, 0.0, 0.01,
    0.0, 0.0, 0.0, 0.0, 1000.
  };
  //  cov[14] = cov[14]/(pt*pt);
  AliExternalTrackParam trc(0, 0, par, cov);
  if (!propagateToZ(trc, ZPlane[NPlanes-1], Bz)) {
    return -1;
  }
  trc.ResetCovariance(1000);
  trc.CheckCovariance();
  //  trc.Print();
  
  for (int i=NPlanes;i--;) {
    if (!propagateToZ(trc, ZPlane[i], Bz)) {
      return -1;
    }
    double meas[2] = {trc.GetY(), trc.GetZ()};
    double zres = Res[i]*trc.GetTgl();
    double err[3] = {Res[i]*Res[i], 0, zres*zres};                                                  
    if (!trc.Update(meas, err) || !trc.CorrectForMeanMaterial(X2X0[i],0., mass, true)) {
      return -1;
    }
    //    trc.Print();
  }
  if (!propagateToZ(trc, 0., Bz)) {
    return -1;
  }

  if (covm) for (int i = 0; i < 15; ++i)
    covm[i] = trc.GetCovariance()[i];
  
  
  return TMath::Sqrt(trc.GetSigma1Pt2())*pt;
}



bool propagateToZ(AliExternalTrackParam& tr, float z, float bz)
{
  const double EPS = 1e-4;
  AliExternalTrackParam trc(tr);
  if (TMath::Abs(tr.GetTgl())<1e-6) {
    return false;
  }
  // bracket using straight line approximation
  float dz = z - trc.GetZ();
  while(TMath::Abs(dz)>EPS) {
    float dx = dz / trc.GetTgl();
    if (!trc.PropagateParamOnlyTo(trc.GetX()+dx, bz)) return kFALSE;
    dz = z - trc.GetZ();
  }
  return tr.PropagateTo(trc.GetX(), bz);
}
