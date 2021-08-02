#ifndef HISTOMANAGER_H
#define HISTOMANAGER_H

#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"
#include "TProfile.h"
#include "TFile.h"
#include "TObjArray.h"
class TROOT;
class TSystem;

class HistoManager 
: public TObjArray
{
  //#ifdef USE_USING
  using TCollection::Print;
  using TCollection::Write;
  //#endif
 public:
  HistoManager(const char* dirname="",const char* fname="histoman.root",Bool_t LOAD=kFALSE,const char* prefix="");
  ~HistoManager();
  HistoManager* CreateClone(const char* prefix);
  //
  Int_t   GetNHistos()             const {return fNHistos;}
  TGraph* GetGraph(Int_t id)       const {return id<=GetLast() ? dynamic_cast<TGraph*>(UncheckedAt(id)) : 0;}
  TH1*    GetHisto(Int_t id)       const {return id<=GetLast() ? dynamic_cast<TH1*>(UncheckedAt(id)) : 0;}
  TH1*    GetHisto(char* name)     const {return dynamic_cast<TH1*>( FindObject(name) );}
  TH1F*   GetHisto1F(Int_t id)     const {return dynamic_cast<TH1F*>( UncheckedAt(id) );}
  TH2F*   GetHisto2F(Int_t id)     const {return dynamic_cast<TH2F*>( UncheckedAt(id) );}
  TProfile* GetHistoP(Int_t id)    const {return dynamic_cast<TProfile*>( UncheckedAt(id) );}
  Int_t   AddHisto(TH1* histo,Int_t at=-1);
  Int_t   AddGraph(TGraph* gr,Int_t at=-1);
  void    DelHisto(Int_t at);
  void    SetFile(TFile* file);
  void    SetFileName(const char* fname);
  char*   GetFileName()            const {return (char*) fDefName.Data();}
  void    SetDirName(const char* name)   {fDirName = name;}
  char*   GetDirName()             const {return (char*) fDirName.Data();}
  //
  void    Reset();
  void    Write(TFile* file=0); 
  Int_t   Write(const char* flname, int =0, int =0) {SetFileName(flname);Write();return 0;}
  void    AddPrefix(const char* pref);
  void    AddHistos(HistoManager* hm, Double_t c1 = 1);
  void    DivideHistos(HistoManager* hm);
  void    MultiplyHistos(HistoManager* hm);
  void    ScaleHistos(Double_t c1 = 1);
  void    SetColor(Color_t tcolor = 1);
  void    SetMarkerStyle(Style_t mstyle = 1, Size_t msize = 1);
  void    SetMarkerSize(Size_t msize = 1);
  void    Sumw2();
  Int_t   Load(const char* fname,const char* dirname="");
  //
  void    Purify(Bool_t emptyToo=kFALSE);
  void    Print(Option_t* option="") const;
  void    Clear(Option_t* option="");
  void    Delete(Option_t* option="");
  virtual void  Compress();
  //
 private:
  Int_t   fNHistos;               // Number of Histogrames defined
  TString fDefName;               // Default file name
  TString fDirName;               // Directory name in the output file
  //
  ClassDef(HistoManager,0)        // NA60 Histograms manager
};






#endif
