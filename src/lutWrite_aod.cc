/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

/// @author: Nicolo' Jacazio
/// @email: nicolo.jacazio@cern.ch

#include "TCanvas.h"
#include "TDatabasePDG.h"
#include "TFile.h"
#include "TMatrixD.h"
#include "TMatrixDSymEigen.h"
#include "TProfile2D.h"
#include "TProfile3D.h"
#include "TVectorD.h"
#include "fwdRes/fwdRes.C"
#include "lutCovm.hh"
#include <Riostream.h>

void diagonalise(lutEntry_t& lutEntry);

bool fwdSolve(float* covm, float pt = 0.1, float eta = 0.0,
    float mass = 0.13957000)
{
  if (fwdRes(covm, pt, eta, mass) < 0)
    return false;
  return true;
}

void lutWrite_aod(const char* filename = "/tmp/lutCovm.pi.aod.dat",
    int pdg = 211,
    float field = 0.2, int layer = 0, int what = 0,
    int efftype = 0,
    const char* infilename = "/tmp/AnalysisResults_LUT.root",
    float minPt = 0.f,
    float maxPt = 80.f,
    float minEta = -4.f,
    float maxEta = 4.f)
{

  std::map<int, std::string> partname{ { 11, "electron" }, { 13, "muon" }, { 211, "pion" }, { 321, "kaon" }, { 2212, "proton" } };
  const std::string dn = "alice3-lutmaker-" + partname[pdg];

  // Get the input from the analysis results
  TFile f(infilename);
  if (!f.IsOpen()) {
    Printf("Did not find %s", infilename);
    return;
  }
  // f.ls();
  TDirectory* d = nullptr;
  f.GetObject(dn.c_str(), d);
  if (!d) {
    Printf("Did not find %s", dn.c_str());
    f.ls();
    return;
  }
  // d->ls();
  std::map<std::string, TH1F*> h{ { "eta", nullptr }, { "pt", nullptr } };
  std::map<std::string, TProfile2D*> m{ { "CovMat_cYY", nullptr },
    { "CovMat_cZY", nullptr },
    { "CovMat_cZZ", nullptr },
    { "CovMat_cSnpY", nullptr },
    { "CovMat_cSnpZ", nullptr },
    { "CovMat_cSnpSnp", nullptr },
    { "CovMat_cTglY", nullptr },
    { "CovMat_cTglZ", nullptr },
    { "CovMat_cTglSnp", nullptr },
    { "CovMat_cTglTgl", nullptr },
    { "CovMat_c1PtY", nullptr },
    { "CovMat_c1PtZ", nullptr },
    { "CovMat_c1PtSnp", nullptr },
    { "CovMat_c1PtTgl", nullptr },
    { "CovMat_c1Pt21Pt2", nullptr },
    { "Efficiency", nullptr } };

  struct binning {
    int n = 0;
    float min = 0;
    float max = 0;
    bool log = false;
  };

  binning histo_eta_bins;
  binning histo_pt_bins;
  for (auto const& i : h) {

    auto setBinning = [&h, &i](binning& b) {
      const auto j = h[i.first];
      if (b.n == 0) {
        Printf("Setting bin for %s", i.first.c_str());
        b.n = j->GetXaxis()->GetNbins();
        b.min = j->GetXaxis()->GetBinLowEdge(1);
        b.max = j->GetXaxis()->GetBinUpEdge(b.n);
      }
      if (std::abs(j->GetXaxis()->GetBinWidth(1) - j->GetXaxis()->GetBinWidth(j->GetNbinsX())) > 0.0001f) {
        b.log = true;
      }
    };

    h[i.first] = ((TH1F*)d->Get(i.first.c_str()));
    h[i.first]->SetDirectory(0);
    if (i.first == "eta") {
      setBinning(histo_eta_bins);
    } else if (i.first == "pt") {
      setBinning(histo_pt_bins);
    }
  }

  for (auto const& i : m) {
    auto checkBinning = [&m, &i, histo_eta_bins, histo_pt_bins]() {
      const auto j = m[i.first];
      const char* n = i.first.c_str();
      // X
      const TAxis* x = j->GetXaxis();
      if (histo_pt_bins.n != x->GetNbins()) {
        Printf("Different number of bins on X for %s: %i vs %i", n, histo_pt_bins.n, x->GetNbins());
        return false;
      }
      if (std::abs(histo_pt_bins.min - x->GetBinLowEdge(1)) > 0.0001f) {
        Printf("Different starting on X for %s: %f vs %f, diff. is %f", n, histo_pt_bins.min, x->GetBinLowEdge(1), histo_pt_bins.min - x->GetBinLowEdge(1));
        return false;
      }
      if (std::abs(histo_pt_bins.max - x->GetBinUpEdge(x->GetNbins())) > 0.0001f) {
        Printf("Different ending on X for %s: %f vs %f, diff. is %f", n, histo_pt_bins.max, x->GetBinUpEdge(x->GetNbins()), histo_pt_bins.max - x->GetBinUpEdge(x->GetNbins()));
        return false;
      }
      // Y
      const TAxis* y = j->GetYaxis();
      if (histo_eta_bins.n != y->GetNbins()) {
        Printf("Different number of bins on Y for %s: %i vs %i", n, histo_eta_bins.n, y->GetNbins());
        return false;
      }
      if (std::abs(histo_eta_bins.min - y->GetBinLowEdge(1)) > 0.0001f) {
        Printf("Different starting on Y for %s: %f vs %f, diff. is %f", n, histo_eta_bins.min, y->GetBinLowEdge(1), histo_eta_bins.min - y->GetBinLowEdge(1));
        return false;
      }
      if (std::abs(histo_eta_bins.max - y->GetBinUpEdge(y->GetNbins())) > 0.0001f) {
        Printf("Different ending on Y for %s: %f vs %f, diff. is %f", n, histo_eta_bins.max, y->GetBinUpEdge(y->GetNbins()), histo_eta_bins.max - y->GetBinUpEdge(y->GetNbins()));
        return false;
      }
      return true;
    };
    // m[i.first] = (TProfile3D*)d->Get(i.first.c_str());
    // m[i.first] = ((TProfile3D*)d->Get(i.first.c_str()))->Project3DProfile("yx");
    m[i.first] = ((TProfile2D*)d->Get(i.first.c_str()));
    m[i.first]->SetDirectory(0);
    if (!checkBinning()) {
      Printf("Something went wrong, stopping");
      return;
    }
  }

  f.Close();

  // output file
  ofstream lutFile(filename, std::ofstream::binary);

  // write header
  lutHeader_t lutHeader;
  // pid
  lutHeader.pdg = pdg;
  lutHeader.mass = TDatabasePDG::Instance()->GetParticle(pdg)->Mass();
  lutHeader.field = field;
  // nch
  lutHeader.nchmap.log = true;
  lutHeader.nchmap.nbins = 1;
  lutHeader.nchmap.min = 0.;
  lutHeader.nchmap.max = 4.;
  // radius
  lutHeader.radmap.log = false;
  lutHeader.radmap.nbins = 1;
  lutHeader.radmap.min = 0.;
  lutHeader.radmap.max = 100.;
  // eta
  lutHeader.etamap.log = false;
  lutHeader.etamap.nbins = histo_eta_bins.n;
  lutHeader.etamap.min = histo_eta_bins.min;
  lutHeader.etamap.max = histo_eta_bins.max;
  Printf("LUT eta: %i, [%f, %f]", lutHeader.etamap.nbins, lutHeader.etamap.min, lutHeader.etamap.max);
  // pt
  lutHeader.ptmap.log = histo_pt_bins.log;
  lutHeader.ptmap.nbins = histo_pt_bins.n;
  lutHeader.ptmap.min = histo_pt_bins.log ? std::log10(histo_pt_bins.min) : histo_pt_bins.min;
  lutHeader.ptmap.max = histo_pt_bins.log ? std::log10(histo_pt_bins.max) : histo_pt_bins.max;
  Printf("LUT pt: %i, [%f, %f]%s", lutHeader.ptmap.nbins, lutHeader.ptmap.min, lutHeader.ptmap.max, lutHeader.ptmap.log ? " LOG AXIS!" : "");
  lutFile.write(reinterpret_cast<char*>(&lutHeader), sizeof(lutHeader));
  // entries
  const int nnch = lutHeader.nchmap.nbins;
  const int nrad = lutHeader.radmap.nbins;
  const int neta = lutHeader.etamap.nbins;
  const int npt = lutHeader.ptmap.nbins;
  lutEntry_t lutEntry;

  auto resetCovM = [&lutEntry]() {
    lutEntry.valid = false;
    for (int i = 0; i < 15; ++i) {
      lutEntry.covm[i] = 0.;
    }
  };

  TH1F* hptcalls = (TH1F*)h["pt"]->Clone("hptcalls");
  hptcalls->Reset();

  TH1F* hetacalls = (TH1F*)h["eta"]->Clone("hetacalls");
  hetacalls->Reset();

  // write entries
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
        lutEntry.eta = lutHeader.etamap.eval(ieta);
        hetacalls->Fill(lutEntry.eta);
        const int bin_eta = h["eta"]->FindBin(lutEntry.eta);
        if (ieta == 0 && bin_eta != 1) {
          Printf("First eta is not the first bin");
          return;
        }
        if (lutEntry.eta < minEta || lutEntry.eta > maxEta) {
          continue;
        }
        for (int ipt = 0; ipt < npt; ++ipt) {
          lutEntry.pt = lutHeader.ptmap.eval(ipt);
          hptcalls->Fill(lutEntry.pt);
          const int bin_pt = h["pt"]->FindBin(lutEntry.pt);
          if (ipt == 0 && bin_pt != 1) {
            Printf("First pt is not the first bin");
            return;
          }
          if (lutEntry.pt < minPt || lutEntry.pt > maxPt) {
            continue;
          }
          if (bin_eta <= 0 || bin_eta > h["eta"]->GetNbinsX()) {
            resetCovM();
          } else if (h["eta"]->GetBinContent(bin_eta) <= 0.f) {
            resetCovM();
          } else if (bin_pt <= 0 || bin_pt > h["pt"]->GetNbinsX()) {
            resetCovM();
          } else if (h["pt"]->GetBinContent(bin_pt) <= 0.f) {
            resetCovM();
          } else {
            if (fabs(lutEntry.eta) < .3) { // Barrel
              // const int bin = m["Efficiency"]->FindBin(lutEntry.pt, lutEntry.eta, 3.14);
              const int bin = m["Efficiency"]->FindBin(lutEntry.pt, lutEntry.eta);
              lutEntry.eff = m["Efficiency"]->GetBinContent(bin);
              lutEntry.covm[0] = m["CovMat_cYY"]->GetBinContent(bin);
              lutEntry.covm[1] = m["CovMat_cZY"]->GetBinContent(bin);
              lutEntry.covm[2] = m["CovMat_cZZ"]->GetBinContent(bin);
              lutEntry.covm[3] = m["CovMat_cSnpY"]->GetBinContent(bin);
              lutEntry.covm[4] = m["CovMat_cSnpZ"]->GetBinContent(bin);
              lutEntry.covm[5] = m["CovMat_cSnpSnp"]->GetBinContent(bin);
              lutEntry.covm[6] = m["CovMat_cTglY"]->GetBinContent(bin);
              lutEntry.covm[7] = m["CovMat_cTglZ"]->GetBinContent(bin);
              lutEntry.covm[8] = m["CovMat_cTglSnp"]->GetBinContent(bin);
              lutEntry.covm[9] = m["CovMat_cTglTgl"]->GetBinContent(bin);
              lutEntry.covm[10] = m["CovMat_c1PtY"]->GetBinContent(bin);
              lutEntry.covm[11] = m["CovMat_c1PtZ"]->GetBinContent(bin);
              lutEntry.covm[12] = m["CovMat_c1PtSnp"]->GetBinContent(bin);
              lutEntry.covm[13] = m["CovMat_c1PtTgl"]->GetBinContent(bin);
              lutEntry.covm[14] = m["CovMat_c1Pt21Pt2"]->GetBinContent(bin);

              lutEntry.valid = true;
            } else {
              lutEntry.eff = 1.;
              resetCovM();
              // // printf(" --- fwdSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
              // if (!fwdSolve(lutEntry.covm, lutEntry.pt, lutEntry.eta, lutHeader.mass)) {
              //   // printf(" --- fwdSolve: error \n");
              // }
            }
          }
          diagonalise(lutEntry);
          if (lutEntry.valid) {
            Printf("Writing valid entry at pT %f and eta %f:", lutEntry.pt, lutEntry.eta);
            lutEntry.print();
          }
          lutFile.write(reinterpret_cast<char*>(&lutEntry), sizeof(lutEntry_t));
        }
      }
    }
  }

  lutFile.close();
  TCanvas* can = new TCanvas();
  can->Divide(2);
  can->cd(1);
  hptcalls->Scale(1. / hetacalls->GetNbinsX());
  hptcalls->Draw("HIST");
  can->cd(2);
  hetacalls->Draw("HIST");
}

void diagonalise(lutEntry_t& lutEntry)
{
  // Printf(" --- diagonalise: pt = %f, eta = %f", lutEntry.pt, lutEntry.eta);
  TMatrixDSym m(5);
  double fcovm[5][5];
  for (int i = 0, k = 0; i < 5; ++i)
    for (int j = 0; j < i + 1; ++j, ++k) {
      fcovm[i][j] = lutEntry.covm[k];
      fcovm[j][i] = lutEntry.covm[k];
    }
  m.SetMatrixArray((double*)fcovm);
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
