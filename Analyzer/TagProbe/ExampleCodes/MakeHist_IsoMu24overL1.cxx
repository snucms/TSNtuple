#include <TagProbe/TnPTool.h>
#include <memory>

class TnPPair: public TnPPairBase {
public:
  TnPPair( KPMuon tagCand, KPMuon probeCand, NtupleHandle* ntuple ): 
  TnPPairBase( tagCand, probeCand, ntuple )
  {

  }

  // -- user-defined tag condition -- //
  Bool_t IsTag() {
    // -- IsoMu24 final filter: hltL3crIsoL1sSingleMu22L1f0L2f10QL3f24QL3trkIsoFiltered0p07 -- //
    // -- IsoMu27 final filter: hltL3crIsoL1sMu22Or25L1f0L2f10QL3f27QL3trkIsoFiltered0p07 -- //
    // -- Mu50 final filter: hltL3fL1sMu22Or25L1f0L2f10QL3Filtered50Q -- //
    Bool_t flag = kFALSE;

    if( tag_.IsHLTFilterMatched( ntuple_, "hltL3crIsoL1sSingleMu22L1f0L2f10QL3f24QL3trkIsoFiltered0p07" ) &&
        tag_.Pt > 26 &&
        fabs(tag_.Eta) < 2.4 &&
        tag_.IsTight && 
        tag_.RelPFIso_dBeta < 0.15 )
      flag = kTRUE;

    // if( flag )
    //   cout << "Tag is found" << endl;

    return flag;
  }

  // -- user-defined probe condition -- //
  Bool_t IsProbe() {
    Bool_t flag = kFALSE;

    if( probe_.IsTight && 
        probe_.RelPFIso_dBeta < 0.15 && 
        probe_.IsL1Matched(ntuple_, 22) )
      flag = kTRUE;

    return flag;
  }

  // -- user-defined passing probe condition -- //
  Bool_t IsPassingProbe() {
    Bool_t flag = kFALSE;
    if( probe_.IsMYHLTFilterMatched( ntuple_, "hltL3crIsoL1sSingleMu22L1f0L2f10QL3f24QL3trkIsoFiltered0p07" ) )
      flag = kTRUE;

    return flag;
  }

};

class HistProducer {
 public:

  void SetOutputFileName( TString outputFileName ) {
    outputFileName_ = outputFileName;
  }

  void AddDataPath( TString dataPath ) {
    printf("[Add data path: %s]\n", dataPath.Data() );
    dataPaths_.push_back( dataPath );
  }

  void Set_minPt( Double_t value ) {
    minPt_ = value;
  }

  void Produce() {

    TChain* chain = new TChain("ntupler/ntuple");
    for(const auto& dataPath : dataPaths_ )
      chain->Add(dataPath);

    NtupleHandle* ntuple = new NtupleHandle( chain );

    std::unique_ptr<TnPHistProducer> tnpHist( new TnPHistProducer(minPt_) );

    Int_t nEvent = chain->GetEntries();
    std::cout << "[Total event: " << nEvent << "]" << std::endl;

    // nEvent = 1000000;
    for(Int_t i_ev=0; i_ev<nEvent; i_ev++) {
      loadBar(i_ev+1, nEvent, 100, 100);

      ntuple->GetEvent( i_ev );

      KPEvent event( ntuple );
      Double_t weight = event.IsRealData? 1.0 : event.GenEventWeight;


      // -- iteration over all muon pairs
      for(Int_t i_mu=0; i_mu<event.nMuon; i_mu++) {
        KPMuon mu_ith( ntuple, i_mu );

        // -- collection of all possible Tag&Probe pairs with same tag muon (mu_ith)
        vector<TnPPair*> tnpPairs_sameTag;

        for(Int_t j_mu=0; j_mu<event.nMuon; j_mu++) {
          if( i_mu == j_mu ) continue;

          KPMuon mu_jth( ntuple, j_mu );

          TnPPair *tnpPair_ij = new TnPPair( mu_ith, mu_jth, ntuple );
          if( tnpPair_ij->IsValid() )  { tnpPairs_sameTag.push_back( tnpPair_ij ); }
          else delete tnpPair_ij;
        } // -- end of j-th muon iteration

        // -- fill TnP histogram only when probeMultiplicity == 1
        if( (Int_t)tnpPairs_sameTag.size() == 1 ) {
          tnpHist->Fill( tnpPairs_sameTag[0], weight );
        }

        for( auto tnpPair : tnpPairs_sameTag )
          delete tnpPair;

      } // -- end of i-th muon iteration

    } // -- end of event iteration

    TFile *f_output = TFile::Open(outputFileName_, "RECREATE");
    tnpHist->Save( f_output );
    f_output->Close();

    cout << "finished" << endl;

    delete ntuple;
    delete chain;
  }

private:
  TString outputFileName_;
  vector<TString> dataPaths_;
  Double_t minPt_;

};

void MakeHist_IsoMu24overL1() {
  std::unique_ptr<HistProducer> histProducer( new HistProducer() );
  histProducer->SetOutputFileName("ROOTFile_TnPHist_example.root");
  histProducer->AddDataPath("/Users/KyeongPil_Lee/ServiceWorks/MuonHLT/v20180507_v01_UpdateTnPCode/TSNtuple/Analyzer/TagProbe/ExampleCodes/ntuple_9.root");
  // histProducer->AddDataPath("/home/kplee/data1/TSNtuple/v20180626_NonIsoMuEff_SingleMuon_Run2018Av2_Run316361to2_Menu2p0/ntuple_9.root");
  histProducer->Set_minPt( 26 ); // -- min pT applied for eta, phi and vtx

  histProducer->Produce();
}
