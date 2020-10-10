#include "style.C"
#include "lutRead_eff.C"

void
draw_dNdeta()
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
    c->DrawFrame(1.e-2, 0., 1., 110., ";#it{p}_{T} (GeV/#it{c});efficiency (%)");
    latex.DrawLatexNDC(0.9, 0.25, title[i].c_str());

    auto g10 = lutRead_eff((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.dNdeta10.dat")).c_str());
    g10->SetLineColor(kAzure-3);
    g10->SetLineStyle(kSolid);
    g10->SetLineWidth(3);
    g10->Draw("samel");
    
    auto g100 = lutRead_eff((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.dNdeta100.dat")).c_str());
    g100->SetLineColor(kGreen+2);
    g100->SetLineStyle(kSolid);
    g100->SetLineWidth(3);
    g100->Draw("samel");
    
    auto g1000 = lutRead_eff((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.dNdeta1000.dat")).c_str());
    g1000->SetLineColor(kRed+1);
    g1000->SetLineStyle(kSolid);
    g1000->SetLineWidth(3);
    g1000->Draw("samel");
    
    auto gdef = lutRead_eff((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.default.dat")).c_str());
    gdef->SetLineColor(kBlack);
    gdef->SetLineStyle(kDashed);
    gdef->SetLineWidth(2);
    gdef->Draw("samel");
    
    c->SaveAs((std::string("draw_dNdeta.") + name[i] + std::string(".png")).c_str());

    cc->cd(i + 1);
    c->DrawClonePad();
  }

  cc->SaveAs("draw_dNdeta.png");
  
}
