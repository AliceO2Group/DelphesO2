/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "lutCovm.hh"
#include "fwdRes/fwdRes.C"

DetectorK fat;
void diagonalise(lutEntry_t &lutEntry);
static float etaMaxBarrel = 1.75;

bool usePara = true; // use fwd parameterisation

bool
fatSolve(lutEntry_t &lutEntry, float pt = 0.1, float eta = 0.0, float mass = 0.13957000, int itof = 0, int otof = 0, int q = 1)
{
  lutEntry.valid = false;

  // solve track
  if (q > 1) mass = -mass;
  TrackSol tr(1, pt, eta, q, mass);
  if (!fat.SolveTrack(tr)) return false;
  AliExternalTrackParam *trPtr = (AliExternalTrackParam*)tr.fTrackCmb.At(0);
  if (!trPtr) return false;

  lutEntry.valid = true;
  lutEntry.eff = fat.GetGoodHitProb(0);
  lutEntry.itof = fat.GetGoodHitProb(itof);
  lutEntry.otof = fat.GetGoodHitProb(otof);
  for (int i = 0; i < 15; ++i) lutEntry.covm[i] = trPtr->GetCovariance()[i];
  
  return true;
}

bool
fwdSolve(float *covm, float pt = 0.1, float eta = 0.0, float mass = 0.13957000)
{
  if (fwdRes(covm, pt, eta, mass) < 0) return false;
  return true;
}

bool
fwdPara(lutEntry_t &lutEntry, float pt = 0.1, float eta = 0.0, float mass = 0.13957000, float Bfield = 0.5)
{
  lutEntry.valid = false;

  // parametrised forward response; interpolates between FAT at eta = 1.75 and a fixed parametrisation at eta = 4; only diagonal elements
  if (fabs(eta) < etaMaxBarrel || fabs(eta) > 4)
    return false;

  if (!fatSolve(lutEntry, pt, etaMaxBarrel, mass)) return false;
  float covmbarrel[15] = {0};
  for (int i = 0; i < 15; ++i) {
    covmbarrel[i] = lutEntry.covm[i];
  }

  // parametrisation at eta = 4
  double beta = 1./sqrt(1+mass*mass/pt/pt/cosh(eta)/cosh(eta)); 
  float dca_pos = 2.5e-4/sqrt(3); // 2.5 micron/sqrt(3)
  float r0 = 0.5; // layer 0 radius [cm]
  float r1 = 1.3;
  float r2 = 2.5;
  float x0layer = 0.001; // material budget (rad length) per layer
  double sigma_alpha = 0.0136/beta/pt*sqrt(x0layer*cosh(eta))*(1+0.038*log(x0layer*cosh(eta)));
  double dcaxy_ms = sigma_alpha*r0*sqrt(1+r1*r1/(r2-r0)/(r2-r0));
  double dcaxy2 = dca_pos*dca_pos+dcaxy_ms*dcaxy_ms;

  double dcaz_ms = sigma_alpha*r0*cosh(eta);
  double dcaz2 = dca_pos*dca_pos+dcaz_ms*dcaz_ms;

  float Leta = 2.8/sinh(eta)-0.01*r0; // m
  double relmomres_pos = 10e-6*pt/0.3/Bfield/Leta/Leta*sqrt(720./15.); 

  float relmomres_barrel = sqrt(covmbarrel[14])*pt;
  float Router = 1; // m
  float relmomres_pos_barrel = 10e-6*pt/0.3/Bfield/Router/Router/sqrt(720./15.);
  float relmomres_MS_barrel = sqrt(relmomres_barrel*relmomres_barrel-relmomres_pos_barrel*relmomres_pos_barrel);

  // interpolate MS contrib (rel resolution 0.4 at eta = 4)
  float relmomres_MS_eta4 = 0.4/beta*0.5/Bfield;
  float relmomres_MS = relmomres_MS_eta4*pow(relmomres_MS_eta4/relmomres_MS_barrel,(fabs(eta)-4.)/(4.-etaMaxBarrel));
  float momres_tot = pt*sqrt(relmomres_pos*relmomres_pos + relmomres_MS*relmomres_MS); // total absolute mom reso

  // Fill cov matrix diag
  for (int i = 0; i < 15; ++i)
    lutEntry.covm[i] = 0;

  lutEntry.covm[0] = covmbarrel[0];
  if (dcaxy2 > lutEntry.covm[0]) lutEntry.covm[0] = dcaxy2;
  lutEntry.covm[2] = covmbarrel[2];
  if (dcaz2 > lutEntry.covm[2]) lutEntry.covm[2] = dcaz2;
  lutEntry.covm[5] = covmbarrel[5]; // sigma^2 sin(phi)
  lutEntry.covm[9] = covmbarrel[9]; // sigma^2 tanl
  lutEntry.covm[14] = momres_tot*momres_tot/pt/pt/pt/pt;  // sigma^2 1/pt
  return true;
}

void
lutWrite(const char *filename = "lutCovm.dat", int pdg = 211, float field = 0.2, int itof = 0, int otof = 0)
{

  // output file
  ofstream lutFile(filename, std::ofstream::binary);
  if (!lutFile.is_open()) {
    Printf("Did not manage to open output file!!");
    return;
  }

  // write header
  lutHeader_t lutHeader;
  // pid
  lutHeader.pdg = pdg;
  lutHeader.mass = TDatabasePDG::Instance()->GetParticle(pdg)->Mass();
  const int q = std::abs(TDatabasePDG::Instance()->GetParticle(pdg)->Charge()) / 3;
  if (q <= 0) {
    Printf("Negative or null charge (%f) for pdg code %i. Fix the charge!", TDatabasePDG::Instance()->GetParticle(pdg)->Charge(), pdg);
    return;
  }
  lutHeader.field = field;
  // nch
  lutHeader.nchmap.log   = true;
  lutHeader.nchmap.nbins = 20;
  lutHeader.nchmap.min   = 0.5;
  lutHeader.nchmap.max   = 3.5;
  // radius
  lutHeader.radmap.log   = false;
  lutHeader.radmap.nbins = 1;
  lutHeader.radmap.min   = 0.;
  lutHeader.radmap.max   = 100.;
  // eta
  lutHeader.etamap.log   = false;
  lutHeader.etamap.nbins = 80;
  lutHeader.etamap.min   = -4.;
  lutHeader.etamap.max   =  4.;
  // pt
  lutHeader.ptmap.log    = true;
  lutHeader.ptmap.nbins  = 200;
  lutHeader.ptmap.min    = -2;
  lutHeader.ptmap.max    = 2.;
  lutFile.write(reinterpret_cast<char *>(&lutHeader), sizeof(lutHeader));
  
  // entries
  const int nnch = lutHeader.nchmap.nbins;
  const int nrad = lutHeader.radmap.nbins;
  const int neta = lutHeader.etamap.nbins;
  const int npt = lutHeader.ptmap.nbins;
  lutEntry_t lutEntry;
  
  // write entries
  for (int inch = 0; inch < nnch; ++inch) {
    auto nch = lutHeader.nchmap.eval(inch);
    lutEntry.nch = nch;
    fat.SetdNdEtaCent(nch);
    std::cout << " --- setting FAT dN/deta: " << nch << std::endl;
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
	auto eta = lutHeader.etamap.eval(ieta);
	lutEntry.eta = lutHeader.etamap.eval(ieta);
	for (int ipt = 0; ipt < npt; ++ipt) {
	  lutEntry.pt = lutHeader.ptmap.eval(ipt);
	  lutEntry.valid = true;
	  if (fabs(eta) <= etaMaxBarrel) { // full lever arm ends at etaMaxBarrel
	    //	    printf(" --- fatSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
	    if (!fatSolve(lutEntry, lutEntry.pt, lutEntry.eta, lutHeader.mass, itof, otof, q)) {
	      //	      printf(" --- fatSolve: error \n");
	      lutEntry.valid = false;
              lutEntry.eff = 0.;
	      for (int i = 0; i < 15; ++i)
		lutEntry.covm[i] = 0.;
	    }
	  }
	  else {
	    //	    printf(" --- fwdSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
	    lutEntry.eff = 1.;
            bool retval = true;
            if (usePara) {
	      retval = fwdPara(lutEntry, lutEntry.pt, lutEntry.eta, lutHeader.mass, field);
            }
            else {
	      retval = fwdSolve(lutEntry.covm, lutEntry.pt, lutEntry.eta, lutHeader.mass);
            }
	    if ( !retval) {
	        //	      printf(" --- fwdSolve: error \n");
	        lutEntry.valid = false;
	        for (int i = 0; i < 15; ++i)
			lutEntry.covm[i] = 0.;
	    }
	  }
	  diagonalise(lutEntry);
	  lutFile.write(reinterpret_cast<char *>(&lutEntry), sizeof(lutEntry_t));
	}}}}
  
	  
  lutFile.close();
}

void diagonalise(lutEntry_t &lutEntry)
{
  TMatrixDSym m(5);
  double fcovm[5][5];
  for (int i = 0, k = 0; i < 5; ++i)
    for (int j = 0; j < i + 1; ++j, ++k) {
      fcovm[i][j] = lutEntry.covm[k];
      fcovm[j][i] = lutEntry.covm[k];
    }  
  m.SetMatrixArray((double *)fcovm);
  TMatrixDSymEigen eigen(m);
  // eigenvalues vector
  TVectorD eigenVal = eigen.GetEigenValues();
  for (int i = 0; i < 5; ++i)
    lutEntry.eigval[i] = eigenVal[i];
  // eigenvectors matrix
  TMatrixD eigenVec = eigen.GetEigenVectors();
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      lutEntry.eigvec[i][j] = eigenVec[i][j];
  // inverse eigenvectors matrix
  eigenVec.Invert();
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      lutEntry.eiginv[i][j] = eigenVec[i][j];

}
