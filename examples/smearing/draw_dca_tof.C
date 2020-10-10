#include "style.C"
#include "lutRead_dca.C"

void
draw_dca_tof()
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

    auto g1 = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.tof1.dat")).c_str());
    g1->SetLineColor(kRed+1);
    g1->SetLineStyle(kSolid);
    g1->SetLineWidth(3);
    g1->Draw("samel");

    auto g2 = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.tof2.dat")).c_str());
    g2->SetLineColor(kAzure-3);
    g2->SetLineStyle(kSolid);
    g2->SetLineWidth(3);
    g2->Draw("samel");

    auto gdef = lutRead_dca((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.default.dat")).c_str());
    gdef->SetLineColor(kBlack);
    gdef->SetLineStyle(kDashed);
    gdef->SetLineWidth(2);
    gdef->Draw("samel");
    
    c->SaveAs((std::string("draw_dca_tof.") + name[i] + std::string(".png")).c_str());

    cc->cd(i + 1);
    c->DrawClonePad();
  }

  cc->SaveAs("draw_dca_tof.png");
  
}
