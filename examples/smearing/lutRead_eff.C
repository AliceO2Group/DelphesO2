#include "lutCovm.hh"

TGraph *
lutRead_eff(const char *filename = "lutCovm.dat", double eta = 0.)
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
  lutEntry_t lutTable[nnch][nrad][neta][npt];
  
  // read entries
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
        for (int ipt = 0; ipt < npt; ++ipt) {
          lutFile.read(reinterpret_cast<char *>(&lutTable[inch][irad][ieta][ipt]), sizeof(lutEntry_t));
          // lutTable[inch][irad][ieta][ipt].print();
        }
      }
    }
  }

  lutFile.close();

  // create graph of pt resolution at eta = 0
  auto inch = lutHeader.nchmap.find(0.);
  auto irad = lutHeader.radmap.find(0.);
  auto ieta = lutHeader.etamap.find(eta);
  auto gpt = new TGraph();
  gpt->SetName(filename);
  gpt->SetTitle(filename);
  gpt->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  gpt->GetYaxis()->SetTitle("efficiency (%)");
  for (int ipt = 0; ipt < npt; ++ipt) {
    auto lutEntry = &lutTable[inch][irad][ieta][ipt];
    if (!lutEntry->valid) {
      std::cout << " ipt = " << ipt << " is not valid " << std::endl;
      continue;
    }
    auto cen = lutEntry->pt;
    auto val = lutEntry->eff * 1.e2;
    gpt->SetPoint(gpt->GetN(), cen, val);
  }
  
  return gpt;
  
}
