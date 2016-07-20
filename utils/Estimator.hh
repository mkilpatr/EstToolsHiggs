#ifndef ESTTOOLS_ESTIMATOR_HH_
#define ESTTOOLS_ESTIMATOR_HH_

#if !defined(__CINT__) || defined(__MAKECINT__)

#include "EstHelper.hh"

using namespace std;
#endif

namespace EstTools{

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class IEstimator {
public:
  IEstimator() : fout_(nullptr) {}
  IEstimator(TString outputdir, TString outputfile):
    outputdir_(outputdir),
    outputfile_(outputfile),
    fout_(nullptr)
  {
    gSystem->mkdir(outputdir, true);
    SetStyle();
  }

  virtual ~IEstimator() {
//////    TH1::AddDirectory(kFALSE); // Detach the histograms from the file: problematic
    if (fout_) fout_->Close();
  }

  void setHeader(const TString& header) {
    header_ = header;
  }

  void setSaveHists(bool saveHists = true) {
    saveHists_ = saveHists;
  }

  void setPostfix(const TString& postfix) {
    postfix_ = postfix;
  }

  void setSelection(const TString& selection, const TString& postfix, const TString& header) {
    selection_ = selection;
    postfix_ = postfix;
    header_ = header;
  }

  void setSelection(const Category& cat){
    selection_ = cat.cut;
    postfix_ = cat.name;
    header_ = cat.label;
  }

  void resetSelection() {
    selection_ = "";
    postfix_ = "";
    header_ = "";
  }

  void setConfig(const BaseConfig& config) {
    this->config = config;
  }

public:
  void savePlot(TCanvas *c, TString fn){
    c->SaveAs(outputdir_+"/"+fn+"."+config.plotFormat);
  }

  void saveHist(const TH1 *h, TString name = ""){
    if (!fout_)
      fout_ = new TFile(outputdir_ + "/" + outputfile_, "RECREATE");
    fout_->cd();
    TH1* hnew = (TH1*) h->Clone();
    hnew->Write(name, TObject::kOverwrite);
  }

public:
  BaseConfig config;

  TString outputdir_;
  TString outputfile_;
  TFile   *fout_;
  TString header_;
  TString postfix_;
  TString selection_;
  bool    saveHists_ = false;

};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class BaseEstimator : public IEstimator {
public:
  BaseEstimator() {}
  BaseEstimator(TString outputdir, TString outputfile = "EstOutput.root") :
    IEstimator(outputdir, outputfile) {}
  BaseEstimator(const BaseConfig &config) :
    IEstimator(config.outputdir, "output.root") { setConfig(config); }

  virtual ~BaseEstimator() {}

  template<typename T>
  void printVec(const std::vector<T>& vec, const TString title="", bool printPercents = false) const{
    if (title!="")
      cout << title << endl;
    int ibin = 0;
    for (const auto &cat_name : config.categories){
      const auto & cat = config.catMaps.at(cat_name);
      cout << setw(30) << cat.name << "\t ";
      for (const auto &b : cat.bin.cuts){
        const auto &quantity = vec.at(ibin++);
        cout << fixed << setprecision(2) << setw(10) << quantity << "\t ";
        if (printPercents && std::is_same<T, Quantity>::value)
          cout << " (" << toString(quantity.error/quantity.value*100, 0, true) << "%) ";
      }
      cout << endl;
    }
    cout << endl;
  }

  virtual void calcYields(){
    vector<TString> snames;
    for (const auto &s : config.samples) snames.push_back(s.first);
    doYieldsCalc(snames);
  }

  virtual void calcYieldsExcludes(const vector<TString> &excludes){
    vector<TString> snames;
    for (const auto &s : config.samples) {
      if ( std::find(excludes.begin(), excludes.end(), s.first) != excludes.end() ) continue;
      snames.push_back(s.first);
    }
    doYieldsCalc(snames);
  }

  virtual void doYieldsCalc(const vector<TString> &sample_names, int nBootstrapping = 0){
    // calculate yields for the samples in snames
    // use SR categories if no CR categories are defined OR sample name ends with "-sr"
    // otherwise use CR categories
    for (auto &sname : sample_names){
      cout << "\nCalc yields for sample " << sname << endl;
      yields[sname] = vector<Quantity>();
      const auto &sample = config.samples.at(sname);
      auto catMaps = (config.crCatMaps.empty() || sname.EndsWith("-sr")) ? config.catMaps : config.crCatMaps;
      auto srCatMaps = config.catMaps;
      for (auto &cat_name : config.categories){
        const auto & cat = catMaps.at(cat_name);
        auto cut = config.sel + " && " + cat.cut;
        auto v = getYieldVectorWrapper(sample.tree, sample.wgtvar, cut + sample.sel, cat.bin, nBootstrapping);
        if (v.size()<srCatMaps.at(cat_name).bin.nbins){
          // !! FIXME : if cr bin numbers < sr: repeat the last bin
          vector<Quantity> vv(v);
          for (unsigned ibin=v.size(); ibin<srCatMaps.at(cat_name).bin.nbins; ++ibin){
            vv.push_back(v.back());
          }
          yields[sname].insert(yields[sname].end(), vv.begin(), vv.end());
        }else{
          yields[sname].insert(yields[sname].end(), v.begin(), v.end());
        }
      }
    }
  }

  virtual vector<Quantity> getYieldVectorWrapper(TTree *intree, TString wgtvar, TString sel, const BinInfo &bin, int nBootstrapping=0){
    if (nBootstrapping==0){
      return getYieldVector(intree, wgtvar, sel, bin);
    }else{
      throw std::invalid_argument("BaseEstimator::getYieldVectorWrapper: Bootstrapping not implemented!");
    }
  }

  void sumYields(vector<TString> list, TString sum_name){
    // sum yields from samples in the list, and store as "sum_name"
    assert(list.size() <= yields.size());
    yields[sum_name] = vector<Quantity>(config.nbins());
    for (const auto &s : list){
      yields[sum_name] = yields[sum_name] + yields.at(s);
    }
  }

  void printYields() const{
    cout << "Yields" << endl;
    for (const auto &p : yields){
      const auto &vec = p.second;
      cout << p.first << ": " << fixed << setprecision(2) << Quantity::sum(vec) << endl;
      printVec(vec);
    }
  }

  void printYieldsTable(vector<TString> columns) const{
    // print yields in tabular format (friendly for pasting into spreadsheets)

    unsigned ibin = 0;

    cout << "Yields" << "\t";
    for (const auto &c : columns){
      cout << c << "\t";
    }
    cout << endl;

    for (auto &cat_name : config.categories){
      const auto &cat = config.catMaps.at(cat_name);
      cout << cat.name << endl;
      for (auto &bin : cat.bin.plotnames){
        cout << bin << "\t";
        for (const auto &c : columns){
          cout << fixed << setprecision(2) << yields.at(c).at(ibin) << "\t ";
        }
        cout << endl;
        ++ibin;
      }
    }

  }

  TH1* getHistogram(const BinInfo& var_info, TString sample, const Category& category){
    // get a histogram of the given sample in the given category

    TString plotvar = var_info.var;
    TString title = ";"
        + var_info.label + (var_info.unit=="" ? "" : " ["+var_info.unit + "]") +";"
        + "Events";

    auto cut = config.sel + " && " + category.cut + TString(selection_=="" ? "" : " && "+selection_);

    auto samp = config.samples.at(sample);
    auto hname = filterString(plotvar) + "_" + sample + "_" + category.name + "_" + postfix_;
    auto hist = getHist(samp.tree, plotvar, samp.wgtvar, cut + samp.sel, hname, title, var_info.plotbins);
    prepHists({hist});
    if (saveHists_) saveHist(hist);

    return hist;
  }

  void plotComp(const BinInfo& var_info, const vector<TString> comp_samples, const vector<TString> comp_categories, bool comp_in_samples = true, bool isNormalized = true){
    // plot distribution in *var_info.var* for all given samples and categories
    // and compare them between either *samples* or *categories* (in the ratio plot)

    vector<TH1*> hists;
    vector<TH1*> ratioHists;
    auto leg = initLegend();

    TString plotvar = var_info.var;
    TString title = ";"
        + var_info.label + (var_info.unit=="" ? "" : " ["+var_info.unit + "]") +";"
        + (isNormalized ? "Normalized (to 1.) Events" : "Events");

    for (unsigned isamp=0; isamp<comp_samples.size(); ++isamp){
      TH1* hRef = nullptr;
      auto sname = comp_samples.at(isamp);
      auto sample = config.samples.at(sname);
      for (unsigned icat=0; icat<comp_categories.size(); ++icat){
        auto cat = config.catMaps.at(comp_categories.at(icat));
        auto hname = filterString(plotvar) + "_" + sname + "_" + cat.name + "_" + postfix_;
        auto cut = config.sel + sample.sel + " && " + cat.cut + TString(selection_=="" ? "" : " && "+selection_);
        auto htmp = getHist(sample.tree, plotvar, sample.wgtvar, cut, hname, title, var_info.plotbins);
        htmp->SetLineStyle(icat+1);
        prepHists({htmp}, isNormalized);
        if (saveHists_) saveHist(htmp);
        hists.push_back(htmp);

        // add legend
        addLegendEntry(leg, htmp, sample.label + " " + cat.label);

        // make ratio histograms : compare between categories
        if (!comp_in_samples){
          if (icat==0)
            hRef = htmp;
          else{
            auto rhist = makeRatioHists(htmp, hRef);
            ratioHists.push_back(rhist);
            if (saveHists_) saveHist(rhist);
          }
        }

      }
    }

    // make ratio histograms : compare between samples
    if (comp_in_samples){
      for (unsigned icat=0; icat<comp_categories.size(); ++icat){
        TH1* hRef = nullptr;
        for (unsigned isamp=0; isamp<comp_samples.size(); ++isamp){
          if (isamp==0)
            hRef = hists.at(icat);
          else{
            auto htmp = hists.at(isamp*comp_categories.size()+icat);
            auto rhist = makeRatioHists(htmp, hRef);
            ratioHists.push_back(rhist);
            if (saveHists_) saveHist(rhist);
          }
        }
      }
    }

    TString RYtitle = comp_in_samples ?
        "#frac{dN(" + (comp_samples.size()==2 ? config.samples.at(comp_samples.back()).label : "...") + ")}{dN(" + config.samples.at(comp_samples.front()).label +")}" :
        "#frac{dN(" + (comp_categories.size()==2 ? config.catMaps.at(comp_categories.back()).label : "...") + ")}{dN(" + config.catMaps.at(comp_categories.front()).label +")}";
    auto c = drawCompAndRatio(hists, ratioHists, leg, RYtitle);
//    drawHeader(header_);
    TString plotname = "comp_"+filterString(plotvar)+"_btw_"+(comp_in_samples?"samples":"categories")+"__"+postfix_;
    c->SetTitle(plotname);
    savePlot(c, plotname);

  }

  void plotEfficiencyComp(const BinInfo& num, const BinInfo& denom, const vector<TString> comp_samples = {"mc", "data"}, TString ytitle="Efficiency", TString ratioYtitle = "Data/MC"){
    // plot the ratio histogram of num.var/denom.var
    // compare the differences between different samples listed in comp_samples

    vector<TH1*> hists;
    vector<TH1*> ratioHists;
    auto leg = initLegend();

    TString num_var = num.var;
    TString denom_var = denom.var;
    TString title = ";"
        + num.label + (num.unit=="" ? "" : " ["+num.unit + "]") +";"
        + ytitle;

    TH1D* hRef = nullptr;
    for (unsigned isamp=0; isamp<comp_samples.size(); ++isamp){
      auto sname = comp_samples.at(isamp);
      auto sample = config.samples.at(sname);
      auto hname = num_var + "_over_" + denom_var + "_" + sname + "_" + postfix_;
      auto cut = config.sel + TString(selection_=="" ? "" : " && "+selection_);
      auto hnum = getHist(sample.tree, num_var, sample.wgtvar, cut + sample.sel, hname, title, num.plotbins);
      auto hdenom = getHist(sample.tree, denom_var, sample.wgtvar, cut + sample.sel, hname+"_denom", title, num.plotbins);
      prepHists({hnum, hdenom});
      hnum->Divide(hnum, hdenom, 1, 1, "B");
      if (comp_samples.size()==2){
        if(isamp==0) { hnum->SetLineColor(kOrange); hnum->SetMarkerColor(kOrange);}
        else if (isamp==1) { hnum->SetLineColor(kBlack); hnum->SetMarkerColor(kBlack);}
      }
      if (saveHists_) saveHist(hnum);
      hists.push_back(hnum);

      // add legend
      addLegendEntry(leg, hnum, sample.label);

      // make ratio histograms
      if (isamp==0)
        hRef = hnum;
      else{
        auto rhist = makeRatioHists(hnum, hRef);
        ratioHists.push_back(rhist);
        if (saveHists_) saveHist(rhist);
      }
    }

    auto c = drawCompAndRatio(hists, ratioHists, leg, ratioYtitle);
//    drawHeader(header_);
    TString plotname = "comp_"+num_var+"_over_"+denom_var+"_btw_"+"samples"+"__"+postfix_;
    c->SetTitle(plotname);
    savePlot(c, plotname);

  }

  void plotStack(const BinInfo& var_info, const vector<TString> mc_samples, const vector<TString> signal_samples, const Category& category, bool norm_to_bkg = false, double sigScale = 1){
    // make stack plots with the given cateogory selection
    // possible to scale signals to total bkg, or scale signal by a const ratio

    vector<TH1*> mchists;
    vector<TH1*> sighists;
    auto leg = initLegend();

    TString plotvar = var_info.var;
    TString title = ";"
        + var_info.label + (var_info.unit=="" ? "" : " ["+var_info.unit + "]") +";"
        + "Events";
    TString RYTitle = "N_{obs}/N_{exp}";

    auto cut = config.sel + " && " + category.cut + TString(selection_=="" ? "" : " && "+selection_);

    TH1 *hbkgtotal = nullptr;
    for (const auto &sname : mc_samples){
      auto sample = config.samples.at(sname);
      auto hname = filterString(plotvar) + "_" + sname + "_" + category.name + "_" + postfix_;
      auto hist = getHist(sample.tree, plotvar, sample.wgtvar, cut + sample.sel, hname, title, var_info.plotbins);
      prepHists({hist}, false, true, true);
      if (saveHists_) saveHist(hist);
      mchists.push_back(hist);

      addLegendEntry(leg, hist, sample.label, "F", 0.04);

      if(!hbkgtotal)
        hbkgtotal = (TH1*)hist->Clone("bkgtotal");
      else
        hbkgtotal->Add(hist);
    }

    double totalbkg = hbkgtotal->Integral(1, hbkgtotal->GetNbinsX()+1);

    for (const auto &sname : signal_samples){
      auto sample = config.samples.at(sname);
      auto hname = filterString(plotvar) + "_" + sname + "_" + category.name + "_" + postfix_;
      auto hist = getHist(sample.tree, plotvar, sample.wgtvar, cut + sample.sel, hname, title, var_info.plotbins);
      prepHists({hist});
      if (saveHists_) saveHist(hist);
      sighists.push_back(hist);

      addLegendEntry(leg, hist, sample.label, "L", 0.04);

      if (norm_to_bkg)
        hist->Scale(totalbkg/hist->Integral(1, hist->GetNbinsX()+1));
      else
        hist->Scale(sigScale);
    }

    TCanvas *c = drawStack(mchists, sighists, false, leg);
    TString plotname = filterString(plotvar)+"_stack_"+category.name+"__"+postfix_;
    c->SetTitle(plotname);
    savePlot(c, plotname);

  }



  TH1* plotDataMC(const BinInfo& var_info, const vector<TString> mc_samples, TString data_sample, const Category& category, bool norm_to_data = false, TString norm_cut = "", bool plotlog = false, std::function<void(TCanvas*)> *plotextra = nullptr){
    // make DataMC plots with the given cateogory selection
    // possible to normalize MC to Data with a different selection (set by *norm_cut*)

    vector<TH1*> mchists;
    auto leg = initLegend();

    TString plotvar = var_info.var;
    TString title = ";"
        + var_info.label + (var_info.unit=="" ? "" : " ["+var_info.unit + "]") +";"
        + "Events";
    TString RYTitle = "N_{obs}/N_{exp}";

    auto cut = config.sel + " && " + category.cut + TString(selection_=="" ? "" : " && "+selection_);

    TH1 *hdata = nullptr;
    Sample d_sample;
    if (data_sample!=""){
      d_sample = config.samples.at(data_sample);
      auto hname = filterString(plotvar) + "_" + data_sample + "_" + category.name + "_" + postfix_;
      hdata = getHist(d_sample.tree, plotvar, d_sample.wgtvar, cut + d_sample.sel, hname, title, var_info.plotbins);
      prepHists({hdata});
      if (saveHists_) saveHist(hdata);
      addLegendEntry(leg, hdata, d_sample.label);
    }

    for (auto &sname : mc_samples){
      auto sample = config.samples.at(sname);
      auto hname = filterString(plotvar) + "_" + sname + "_" + category.name + "_" + postfix_;
      auto hist = getHist(sample.tree, plotvar, sample.wgtvar, cut + sample.sel, hname, title, var_info.plotbins);
      prepHists({hist});
      if (saveHists_) saveHist(hist);
      mchists.push_back(hist);
      hist->SetFillColor(hist->GetLineColor()); hist->SetFillStyle(1001); hist->SetLineColor(kBlack);

      addLegendEntry(leg, hist, sample.label, "F");
    }

    if (hdata){
      if (norm_to_data){
        double sf = 1;
        // calc sf
        if (norm_cut==""){
          auto hsum = (TH1*)mchists.front()->Clone("hsum");
          for (unsigned i=1; i<mchists.size(); ++i){
            hsum->Add(mchists.at(i));
          }
          sf = hdata->Integral(1, hsum->GetNbinsX()+1) / hsum->Integral(1, hsum->GetNbinsX()+1);
        }else {
          auto data_inc = getYields(d_sample.tree, d_sample.wgtvar, norm_cut + d_sample.sel);
          vector<Quantity> mc_quantities;
          for (auto &sname : mc_samples){
            auto sample = config.samples.at(sname);
            mc_quantities.push_back(getYields(sample.tree, sample.wgtvar, norm_cut + sample.sel));
          }
          auto mc_inc = Quantity::sum(mc_quantities);
          sf = (data_inc/mc_inc).value;
        }
        // scale the mc hists
        for (auto *hist : mchists) hist->Scale(sf);
      }
    }

    TCanvas *c = nullptr;
    if (hdata){
      c = drawStackAndRatio(mchists, hdata, leg, plotlog);
    }else{
      c = drawStack(mchists, {}, plotlog, leg);
    }

    if (plotextra) (*plotextra)(c);

    TString plotname = filterString(plotvar)+"_DataMC_"+category.name+"__"+postfix_;
    c->SetTitle(plotname);
    savePlot(c, plotname);

    if (hdata)
      return makeRatioHists(hdata, sumHists(mchists, "bkgtotal"));
    else
      return nullptr;
  }

  void plotDataMC(const vector<TString> mc_samples, TString data_sample, bool norm_to_data = false, TString norm_cut = "", bool plotlog = false, std::function<void(TCanvas*)> *plotextra = nullptr){
    // make Data/MC plots for all categories
    for (auto category : config.categories){
      const auto &cat = config.catMaps.at(category);
      plotDataMC(cat.bin, mc_samples, data_sample, cat, norm_to_data, norm_cut, plotlog, plotextra);
    }
  }

  void printSummary(const vector<vector<Quantity>> &bkgs, const vector<Quantity> &data){
    auto totalbkg = bkgs.front();
    for (unsigned ibkg=1; ibkg<bkgs.size(); ++ibkg) totalbkg = totalbkg + bkgs.at(ibkg);

    cout << setw(30) << "category" << "\t " << setw(30) << "pred." << setw(30) << "obs." << endl;
    unsigned ibin=0;
    for (const auto &cat_name : config.categories){
      Quantity pred(0, 0);
      Quantity obs(0, 0);
      const auto & cat = config.catMaps.at(cat_name);
      for (const auto &b : cat.bin.cuts){
        pred = pred + totalbkg.at(ibin);
        obs  = obs  + data.at(ibin);
        ++ibin;
      }
      cout << setw(30) << cat.name << "\t "
          << setw(30) << setprecision(2) << pred
          << setw(30) << setprecision(0) << obs.value << endl;
    }

  }


public:
  map<TString, vector<Quantity>> yields;  // stores yields of all bins in all categories for each sample

};



}

#endif /*ESTTOOLS_ESTIMATOR_HH_*/
