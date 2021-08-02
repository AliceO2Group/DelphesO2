#include "lutCovm.hh"

void lutRead_allres_vspt(const char *filename = "lutCovm.dat",
		  TGraph *dca_xy = 0, TGraph *dca_z = 0, TGraph *sinp = 0, TGraph *tanl = 0, TGraph* ptres = 0, double eta = 0.)
{
  
  // input file
  ifstream lutFile(filename, std::ofstream::binary);

  // read header
  lutHeader_t lutHeader;
  lutFile.read(reinterpret_cast<char *>(&lutHeader), sizeof(lutHeader));
  lutHeader.print();

  // entries
  const int nnch = lutHeader.nchmap.nbins;
  const int nrad = lutHeader.radmap.nbins;
  const int neta = lutHeader.etamap.nbins;
  const int npt = lutHeader.ptmap.nbins;

  auto nchbin = lutHeader.nchmap.find(0.);
  auto radbin = lutHeader.radmap.find(0.);
  auto etabin = lutHeader.etamap.find(eta);

  lutEntry_t lutTable[npt];
  
  // read entries
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
        for (int ipt = 0; ipt < npt; ++ipt) {
	  if (inch==nchbin && irad==radbin && ieta == etabin) {
	    lutFile.read(reinterpret_cast<char *>(&lutTable[ipt]), sizeof(lutEntry_t));
	    //lutTable[ipt].print();
	  }
	  else {
	    lutEntry_t dummy;
	    lutFile.read(reinterpret_cast<char *>(&dummy), sizeof(lutEntry_t));
	  }
        }
      }
    }
  }

  lutFile.close();

  if (ptres) {
    ptres->SetName(filename);
    ptres->SetTitle(filename);
    ptres->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    ptres->GetYaxis()->SetTitle("momentum resolution (%)");
  }
  
  if (dca_xy) {
    dca_xy->SetName(filename);
    dca_xy->SetTitle(filename);
    dca_xy->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    dca_xy->GetYaxis()->SetTitle("#sigma(d_{xy}) (#mum)");
  }

  if (dca_z) {
    dca_z->SetName(filename);
    dca_z->SetTitle(filename);
    dca_z->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    dca_z->GetYaxis()->SetTitle("#sigma(d_{z}) (#mum)");
  }
  
  if (sinp) {
    sinp->SetName(filename);
    sinp->SetTitle(filename);
    sinp->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    sinp->GetYaxis()->SetTitle("#sigma(sin(#phi))");
  }

  if (tanl) {
    tanl->SetName(filename);
    tanl->SetTitle(filename);
    tanl->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    tanl->GetYaxis()->SetTitle("#sigma(tam(#lambda))");
  }
  for (int ipt = 0; ipt < npt; ++ipt) {
    auto lutEntry = &lutTable[ipt];
    if (!lutEntry->valid) continue;
    auto cen = lutEntry->pt;
    if (ptres) {
      auto val = sqrt(lutEntry->covm[14]) * lutEntry->pt * 100.;
      ptres->SetPoint(ptres->GetN(), cen, val);
    }
    if (dca_xy) {
      auto val = sqrt(lutEntry->covm[0]) * 1e4;
      dca_xy->SetPoint(dca_xy->GetN(), cen, val);
    }
    if (dca_z) {
      auto val = sqrt(lutEntry->covm[2]) * 1e4;
      dca_z->SetPoint(dca_z->GetN(), cen, val);
    }
    if (sinp) {
      auto val = sqrt(lutEntry->covm[5]);
      sinp->SetPoint(sinp->GetN(), cen, val);
    }
    if (tanl) {
      auto val = sqrt(lutEntry->covm[9]);
      tanl->SetPoint(tanl->GetN(), cen, val);
    }
  }
}
