#include "HistoManager.h"
#include "TROOT.h"
#include "TSystem.h"

ClassImp(HistoManager)

//_______________________________________________________________
HistoManager::HistoManager(const char* dirname,const char* fname,Bool_t LOAD,const char* prefix)
{
  //
  fNHistos = 0;
  fDirName = dirname;
  SetFileName(fname);
  SetFile(0);
  if (LOAD && !fDefName.IsNull()) {
    int nh = Load(fname,dirname);
    Printf("HistoManager::Load was requested: got %d histos from %s/%s",nh,fname,dirname);
    if (prefix && prefix[0]!=0) AddPrefix(prefix);
  }
  //
}

//_______________________________________________________________
HistoManager::~HistoManager()
{
  Delete();
}

//_______________________________________________________________
HistoManager* HistoManager::CreateClone(const char* prefix)
{
  //
  HistoManager* hm = (HistoManager*)this->Clone();
  hm->AddPrefix(prefix);
  TH1* histo;
  for (int i=0;i<GetLast()+1;i++) {
    TObject* obj = UncheckedAt(i);
    if (!obj) continue;
    if (obj->InheritsFrom("TH1")) {
      ((TH1*)obj)->SetDirectory(0);
    }
  }
  hm->fNHistos = fNHistos;
  hm->SetFileName(fDefName.Data());
  hm->SetDirName(fDirName.Data());
  return hm;
  //
}

//_______________________________________________________________
Int_t HistoManager::AddHisto(TH1* histo,Int_t at)
{
  // Add new histo either to next free slot (at<0) or to requested position
  if (at<0) at = fNHistos;
  AddAtAndExpand(histo,at);
  histo->SetDirectory(0);
  histo->SetUniqueID(at+1);
  return fNHistos++;
  //
}

//_______________________________________________________________
Int_t HistoManager::AddGraph(TGraph* gr,Int_t at)
{
  // Add new histo either to next free slot (at<0) or to requested position
  if (at<0) at = fNHistos;
  AddAtAndExpand(gr,at);
  //histo->SetDirectory(0);
  gr->SetUniqueID(at+1);
  return fNHistos++;
  //
}

//_______________________________________________________________
void HistoManager::Compress()
{
  TObjArray::Compress();
  TObject* histo;
  for (int i=0;i<GetLast()+1;i++) if ((histo=At(i))) histo->SetUniqueID(i+1);
}

//_______________________________________________________________
void HistoManager::Write(TFile* file)
{
  // Write all histograms to file
  if (!fNHistos) return;
  Bool_t localfile = kFALSE;
  TH1* histo=0;
  TFile *lfile=0;
  const char* str=0;
  if (file) lfile = file;
  else {
    // Check if the file is not already open
    TFile *tmpF = (TFile*)gROOT->GetListOfFiles()->FindObject(fDefName.Data());
    if (tmpF && tmpF->IsOpen()) {
      TString opt = tmpF->GetOption(); opt.ToLower();
      if (!opt.Contains("read")) {
	lfile = tmpF;
	tmpF->cd();
      }
    }
  }
  TString pwd = gDirectory->GetPath();
  if (!lfile) { // have to open
    str = fDefName.Data();
    if (!str || !str[0] || str[0] == ' ') fDefName = "histoman";
    if (!fDefName.Contains(".root")) fDefName += ".root";
    lfile = TFile::Open(fDefName.Data(),"UPDATE");
    fDefName = str;
    localfile = kTRUE;
  }
  //
  lfile->cd();
  // Create directory (if necessary)
  str = fDirName.Data();
  if (str && str[0] && str[0] != ' ') {
    if (!lfile->Get(str)) lfile->mkdir(str);
    lfile->cd(str);
  }
  Printf("Writing histogrames to: %s%s",lfile->GetPath(),str);
  for (int i=0;i<GetLast()+1;i++) {
    TObject* obj = UncheckedAt(i);
    if (!obj) continue;
    TH1* histo = dynamic_cast<TH1*>(obj);
    TDirectory* dr = 0;
    if (histo) {
      dr = histo->GetDirectory();
      histo->SetDirectory(0);
    }
    obj->Write(0,TObject::kOverwrite);
    if (dr && histo) histo->SetDirectory(dr);
  }
  if (localfile) {lfile->Close(); delete lfile;}
  TDirectory* oldDir = ((TDirectory *)gROOT->GetListOfFiles()->FindObject(pwd.Data()));
  if (oldDir) oldDir->cd();
  //
}

//_______________________________________________________________
void  HistoManager::Clear(Option_t*) 
{
  int nent = GetLast()+1;
  for (int i=0;i<nent;i++) {
    TObject* hh = UncheckedAt(i);
    if (!hh) continue;
    RemoveAt(i);
    fNHistos--;
    //
  }
}

//_______________________________________________________________
void HistoManager::Delete(Option_t*) 
{
  int nent = GetLast()+1;
  for (int i=0;i<nent;i++) {
    TObject* hh = UncheckedAt(i);
    if (!hh) continue;
    RemoveAt(i);
    delete hh;
    //
  }
  fNHistos = 0;
}

//_______________________________________________________________
void  HistoManager::Print(Option_t* option) const
{
  int nent = GetLast()+1;
  for (int i=0;i<nent;i++) {
    TObject* hh = UncheckedAt(i);
    if (!hh) continue;
    Printf("At position #%d",i);
    hh->Print(option);
    //
  }
  Printf("\nTotal number of defined Histograms: %d",fNHistos);
  Printf("\nCurrent output path: %s/%s",fDefName.Data(),fDirName.Data());
}

//_______________________________________________________________
void  HistoManager::AddPrefix(const char* pref)
{
  TString prfs = pref;
  if (prfs.IsNull()) return;
  int nent = GetLast()+1;
  for (int i=0;i<nent;i++) {
    TObject* hh = UncheckedAt(i);
    if (!hh) continue;
    prfs = pref;
    prfs += hh->GetName();
    if (hh->InheritsFrom("TNamed")) ((TNamed*)hh)->SetName(prfs.Data());
    //    prfs = pref;
    //    prfs += hh->GetTitle();
    //    hh->SetTitle(prfs.Data());
  }
}

//_______________________________________________________________
void  HistoManager::AddHistos(HistoManager* hm,Double_t c1)
{
  int nent = GetLast()+1;
  int nent1 = hm->GetLast()+1;
  if (nent!=nent1) {Error("AddHistos","HistoManagers have different content: %d vs %d",nent,nent1);return;}
  for (int i=0;i<nent;i++) {
    TH1* hh1 = GetHisto(i);
    TH1* hh2 = hm->GetHisto(i);
    if (!hh1 || !hh2) continue;
    hh1->Add(hh2,c1);
  }
}

//_______________________________________________________________
void  HistoManager::DivideHistos(HistoManager* hm)
{
  int nent = GetLast()+1;
  int nent1 = hm->GetLast()+1;
  if (nent!=nent1) {Error("DivideHistos","HistoManagers have different content: %d vs %d",nent,nent1);return;}
  for (int i=0;i<nent;i++) {
    TH1* hh1 = GetHisto(i);
    TH1* hh2 = hm->GetHisto(i);
    if (!hh1 || !hh2) continue;
    hh1->Divide(hh2);
  }
}

//_______________________________________________________________
void  HistoManager::MultiplyHistos(HistoManager* hm)
{
  int nent = GetLast()+1;
  int nent1 = hm->GetLast()+1;
  if (nent!=nent1) {Error("MultiplyHistos","HistoManagers have different content: %d vs %d",nent,nent1);return;}
  for (int i=0;i<nent;i++) {
    TH1* hh1 = GetHisto(i);
    TH1* hh2 = hm->GetHisto(i);
    if (!hh1 || !hh2) continue;
    hh1->Multiply(hh2);
  }
}

//_______________________________________________________________
void  HistoManager::ScaleHistos(Double_t c1)
{
  int nent = GetLast()+1;
  for (int i=0;i<nent;i++) {
    TH1* hh1 = GetHisto(i);
    if (hh1) hh1->Scale(c1);
  }
}

//_______________________________________________________________
void  HistoManager::Sumw2()
{
  int nent = GetLast()+1;
  for (int i=0;i<nent;i++) {
    TH1* hh1 = dynamic_cast<TH1*>(UncheckedAt(i));
    if (hh1) hh1->Sumw2();
  }
}

//_______________________________________________________________
void HistoManager::SetFile(TFile* file)
{
  if (file) fDefName = file->GetName();
}

//_______________________________________________________________
void HistoManager::DelHisto(Int_t at)
{
  TH1* hist = GetHisto(at);
  if (hist) {
    RemoveAt(at);
    delete hist;
  }
}

//_______________________________________________________________
void HistoManager::Purify(Bool_t emptyToo)
{
  // remove empty slots, optionally removing empty histos too
  int last = GetLast()+1;
  if (emptyToo)
    for (int i=0;i<last;i++) {
      TH1* hist = GetHisto(i);
      if (!hist) continue;
      if (hist->GetEntries()<1) { DelHisto(i); fNHistos--;}
    }
  Compress();
  //
}

//_____________________________________________________________________________
void HistoManager::SetFileName(const char *name)  
{fDefName = name; gSystem->ExpandPathName(fDefName);}

//_____________________________________________________________________________
void HistoManager::Reset()
{
  int last = GetLast()+1;
  for (int i=0;i<last;i++) {
    TH1* hist = GetHisto(i);
    if (!hist) continue;
    hist->Reset();
  }
}

//_____________________________________________________________________________
Int_t HistoManager::Load(const char* fname,const char* dirname)
{
  TString flpath = fname;
  gSystem->ExpandPathName(flpath);
  TFile* file = TFile::Open(flpath.Data());
  if (!file) {Printf("Error: no file %s",fname); return 0;}
  if (dirname && dirname[0] && dirname[0] != ' ') {
    if (!file->Get(dirname)) {
      Printf("Error: no %s directory in file %s",dirname,fname);
      file->Close(); delete file;
      return 0;
    }
    else file->cd(dirname);
  }
  //
  int count = 0;
  TList* lst = gDirectory->GetListOfKeys();
  TIter next(lst);
  TObject* obj;
  while ((obj=next())) {
    if (FindObject(obj->GetName())) continue; // already added
    TObject* hst = gDirectory->Get(obj->GetName());
    int ID = hst->GetUniqueID();
    TH1* h = dynamic_cast<TH1*>(hst);
    if (h) {
      AddHisto(h, ID-1);
      count++;
      continue;
    }
    TGraph* gr = dynamic_cast<TGraph*>(hst);
    if (gr) {
      AddGraph(gr, ID-1);
      count++;
      continue;
    }
  }
  file->Close(); 
  delete file;
  return count;
}

//_____________________________________________________________________________
void HistoManager::SetColor(Color_t tcolor)
{
  int last = GetLast()+1;
  for (int i=0;i<last;i++) {
    TH1* hist = GetHisto(i);
    if (!hist) continue;
    hist->SetLineColor(tcolor);
    hist->SetMarkerColor(tcolor);
  }
}

//_____________________________________________________________________________
void HistoManager::SetMarkerStyle(Style_t mstyle,Size_t msize)
{
  int last = GetLast()+1;
  for (int i=0;i<last;i++) {
    TH1* hist = GetHisto(i);
    if (!hist) continue;
    hist->SetMarkerStyle(mstyle);
    hist->SetMarkerSize(msize);
  }
}

//_____________________________________________________________________________
void HistoManager::SetMarkerSize(Size_t msize)
{
  int last = GetLast()+1;
  for (int i=0;i<last;i++) {
    TH1* hist = GetHisto(i);
    if (!hist) continue;
    hist->SetMarkerSize(msize);
  }
}

