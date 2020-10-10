#include "style.C"
#include "lutRead_dca.C"

void
draw_dca()
{

  style();

  std::vector<std::string> name = {"el", "pi", "ka", "pr"};
  //  std::vector<std::string> title = {"electron", "pion", "kaon", "proton"};
  std::vector<std::string> title = {"e", "#pi", "K", "p"};

  TLatex latex;
  latex.SetTextAlign(33);
  
  auto cc = new TCanvas("cc", "cc", 800, 800);
  cc->Divide(2, 2);
  
  for (int i = 0; i < 4; ++i) {
  
    auto c = new TCanvas((std::string("c") + name[i]).c_str(),
			 (std::string("c") + name[i]).c_str(),
			 800, 800);
    c->SetLogx();
    c->SetLogy();
    c->DrawFrame(1.e-2, 0.1, 100., 10000., ";#it{p}_{T} (GeV/#it{c});pointing resolution (#mum)");
    latex.DrawLatexNDC(0.9, 0.9, title[i].c_str());

    auto g2a = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".2kG.20cm.dat")).c_str());
    g2a->SetLineColor(kRed+1);
    g2a->SetLineStyle(kDashed);
    g2a->SetLineWidth(3);
    g2a->Draw("samel");
    
    auto g2b = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".2kG.100cm.dat")).c_str());
    g2b->SetLineColor(kRed+1);
    g2b->SetLineStyle(kSolid);
    g2b->SetLineWidth(3);
    g2b->Draw("samel");

    auto g5a = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.dat")).c_str());
    g5a->SetLineColor(kAzure-3);
    g5a->SetLineStyle(kDashed);
    g5a->SetLineWidth(3);
    g5a->Draw("samel");

    auto g5b = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".5kG.100cm.dat")).c_str());
    g5b->SetLineColor(kAzure-3);
    g5b->SetLineStyle(kSolid);
    g5b->SetLineWidth(3);
    g5b->Draw("samel");
    
    cc->cd(i + 1);
    c->DrawClonePad();
  }
			 
}
